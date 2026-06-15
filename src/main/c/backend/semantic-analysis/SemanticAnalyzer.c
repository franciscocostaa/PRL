#include "SemanticAnalyzer.h"

#include "SymbolTable.h"

#include <string.h>

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownSemanticAnalyzerModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: SemanticAnalyzer...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeSemanticAnalyzerModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("SemanticAnalyzer");
	return _shutdownSemanticAnalyzerModule;
}

/* TYPE SYSTEM */

/**
 * The type of a PRL construction. TYPE_FIELD represents a (dynamically typed)
 * entity field whose concrete type is unknown because no schema was declared,
 * and TYPE_ERROR is the bottom type (⊥) of the "Sistema de Tipos" specification,
 * signalling a malformed/ill-typed construction.
 */
typedef enum {
	TYPE_INTEGER,
	TYPE_PERCENTAGE,
	TYPE_STRING,
	TYPE_BOOLEAN,
	TYPE_FIELD,
	TYPE_ERROR
} PrlType;

/** The state carried while validating one campaign or simulation. */
typedef struct {
	SymbolTable * table;
	/** Campaign whose entity schema field accesses are resolved against. */
	const CampaignNode * campaign;
	/** Enforce that field accesses reference a declared entity. */
	bool enforceEntityBinding;
	int errorCount;
} SemanticContext;

/* PRIVATE FUNCTIONS */

static const char * _typeName(PrlType type) {
	switch (type) {
		case TYPE_INTEGER: return "integer";
		case TYPE_PERCENTAGE: return "percentage";
		case TYPE_STRING: return "string";
		case TYPE_BOOLEAN: return "boolean";
		case TYPE_FIELD: return "field";
		default: return "⊥";
	}
}

static const char * _actionName(ActionKind kind) {
	switch (kind) {
		case ACTION_DISCOUNT: return "discount";
		case ACTION_DISCOUNT_FIXED: return "discount_fixed";
		case ACTION_SURCHARGE: return "surcharge";
		case ACTION_REJECT: return "reject";
		default: return "unknown";
	}
}

/** Maps a user-declared property type name to an internal PRL type. */
static PrlType _typeFromName(const char * typeName) {
	if (typeName == NULL) {
		return TYPE_FIELD;
	}
	if (strcmp(typeName, "integer") == 0) return TYPE_INTEGER;
	if (strcmp(typeName, "percentage") == 0) return TYPE_PERCENTAGE;
	if (strcmp(typeName, "string") == 0) return TYPE_STRING;
	if (strcmp(typeName, "boolean") == 0) return TYPE_BOOLEAN;
	/* Domain types (e.g. "Money") have no scalar mapping; treat as dynamic. */
	return TYPE_FIELD;
}

/** True if two literal/scalar types can be compared (integer↔percentage). */
static bool _literalsCompatible(PrlType left, PrlType right) {
	if (left == right) {
		return true;
	}
	if ((left == TYPE_INTEGER && right == TYPE_PERCENTAGE) ||
		(left == TYPE_PERCENTAGE && right == TYPE_INTEGER)) {
		return true;
	}
	return false;
}

static bool _isLiteralOfKind(const ExpressionNode * expression, LiteralKind kind) {
	return expression != NULL
		&& expression->kind == EXPRESSION_LITERAL
		&& expression->literal->kind == kind;
}

static const EntityDeclNode * _findEntity(const CampaignNode * campaign, const char * name) {
	if (campaign == NULL) {
		return NULL;
	}
	for (size_t i = 0; i < campaign->entityCount; i++) {
		if (strcmp(campaign->entities[i]->name, name) == 0) {
			return campaign->entities[i];
		}
	}
	return NULL;
}

static const PropertyNode * _findProperty(const EntityDeclNode * entity, const char * name) {
	for (size_t i = 0; i < entity->propertyCount; i++) {
		if (strcmp(entity->properties[i]->name, name) == 0) {
			return entity->properties[i];
		}
	}
	return NULL;
}

/** type(expression): literals map to their type; fields use the declared schema. */
static PrlType _expressionType(SemanticContext * context, const ExpressionNode * expression) {
	if (expression == NULL) {
		return TYPE_ERROR;
	}
	switch (expression->kind) {
		case EXPRESSION_LITERAL:
			switch (expression->literal->kind) {
				case LITERAL_INTEGER: return TYPE_INTEGER;
				case LITERAL_STRING: return TYPE_STRING;
				case LITERAL_PERCENTAGE: return TYPE_PERCENTAGE;
				default: return TYPE_ERROR;
			}
		case EXPRESSION_FIELD_ACCESS: {
			const FieldAccessNode * access = expression->fieldAccess;
			const EntityDeclNode * entity = _findEntity(context->campaign, access->entityName);
			if (entity == NULL) {
				if (context->enforceEntityBinding) {
					logError(_logger, "Undefined entity '%s' referenced in '%s.%s'.",
						access->entityName, access->entityName, access->fieldName);
					context->errorCount++;
				}
				return TYPE_FIELD;
			}
			/* With a declared schema, field accesses are statically typed. */
			if (entity->propertyCount > 0) {
				const PropertyNode * property = _findProperty(entity, access->fieldName);
				if (property == NULL) {
					logError(_logger, "Entity '%s' (type '%s') has no property '%s'.",
						entity->name, entity->typeName, access->fieldName);
					context->errorCount++;
					return TYPE_FIELD;
				}
				return _typeFromName(property->typeName);
			}
			return TYPE_FIELD;
		}
		case EXPRESSION_ARITHMETIC: {
			PrlType left = _expressionType(context, expression->leftOperand);
			PrlType right = _expressionType(context, expression->rightOperand);
			if (left == TYPE_STRING || right == TYPE_STRING || left == TYPE_BOOLEAN || right == TYPE_BOOLEAN) {
				logError(_logger, "Arithmetic operator applied to a non-numeric operand (%s, %s).",
					_typeName(left), _typeName(right));
				context->errorCount++;
				return TYPE_ERROR;
			}
			if (left == TYPE_ERROR || right == TYPE_ERROR) {
				return TYPE_ERROR;
			}
			if (left == TYPE_FIELD || right == TYPE_FIELD) {
				return TYPE_FIELD;
			}
			return TYPE_INTEGER;
		}
		default:
			return TYPE_ERROR;
	}
}

/** type(condition): always reduces to TYPE_BOOLEAN; ill-typed parts log errors. */
static PrlType _conditionType(SemanticContext * context, const ConditionNode * condition) {
	if (condition == NULL) {
		return TYPE_ERROR;
	}
	switch (condition->kind) {
		case CONDITION_AND:
		case CONDITION_OR:
			_conditionType(context, condition->left);
			_conditionType(context, condition->right);
			return TYPE_BOOLEAN;
		case CONDITION_NOT:
			_conditionType(context, condition->operand);
			return TYPE_BOOLEAN;
		case CONDITION_COMPARISON: {
			PrlType leftType = _expressionType(context, condition->leftExpression);
			PrlType rightType = _expressionType(context, condition->rightExpression);
			if (leftType != TYPE_FIELD && rightType != TYPE_FIELD
				&& leftType != TYPE_ERROR && rightType != TYPE_ERROR
				&& !_literalsCompatible(leftType, rightType)) {
				logError(_logger, "Incompatible types in comparison (%s vs %s).",
					_typeName(leftType), _typeName(rightType));
				context->errorCount++;
			}
			return TYPE_BOOLEAN;
		}
		case CONDITION_IN: {
			PrlType targetType = _expressionType(context, condition->inExpression);
			for (size_t i = 0; i < condition->optionCount; i++) {
				PrlType optionType = _expressionType(context, condition->options[i]);
				if (targetType != TYPE_FIELD && targetType != TYPE_ERROR
					&& optionType != TYPE_FIELD && optionType != TYPE_ERROR
					&& !_literalsCompatible(targetType, optionType)) {
					logError(_logger, "Incompatible type in 'in' list (%s vs %s).",
						_typeName(targetType), _typeName(optionType));
					context->errorCount++;
				}
			}
			return TYPE_BOOLEAN;
		}
		case CONDITION_EXPRESSION:
			_expressionType(context, condition->expression);
			return TYPE_BOOLEAN;
		default:
			return TYPE_ERROR;
	}
}

/** Type-checks a rule action: discount/surcharge → percentage, discount_fixed → integer. */
static void _checkAction(SemanticContext * context, const ActionNode * action) {
	if (action == NULL) {
		return;
	}
	switch (action->kind) {
		case ACTION_DISCOUNT:
		case ACTION_SURCHARGE:
			if (!_isLiteralOfKind(action->value, LITERAL_PERCENTAGE)) {
				logError(_logger, "Action '%s' expects a percentage value (e.g. 10%%).", _actionName(action->kind));
				context->errorCount++;
			}
			break;
		case ACTION_DISCOUNT_FIXED:
			if (!_isLiteralOfKind(action->value, LITERAL_INTEGER)) {
				logError(_logger, "Action 'discount_fixed' expects an integer value.");
				context->errorCount++;
			}
			break;
		case ACTION_REJECT:
			/* The grammar guarantees a message literal. */
			break;
	}
}

static void _validateCampaign(SemanticContext * context, const CampaignNode * campaign) {
	symbolTableEnterScope(context->table);

	for (size_t i = 0; i < campaign->entityCount; i++) {
		const EntityDeclNode * entity = campaign->entities[i];
		if (!symbolTableDeclare(context->table, entity->name, SYMBOL_ENTITY, entity->typeName)) {
			logError(_logger, "Duplicate entity '%s' in campaign '%s'.", entity->name, campaign->name);
			context->errorCount++;
		}
		/* Reject schemas with two properties of the same name. */
		for (size_t j = 0; j < entity->propertyCount; j++) {
			for (size_t k = 0; k < j; k++) {
				if (strcmp(entity->properties[j]->name, entity->properties[k]->name) == 0) {
					logError(_logger, "Duplicate property '%s' in entity '%s'.", entity->properties[j]->name, entity->name);
					context->errorCount++;
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < campaign->ruleCount; i++) {
		for (size_t j = 0; j < i; j++) {
			if (strcmp(campaign->rules[i]->name, campaign->rules[j]->name) == 0) {
				logError(_logger, "Duplicate rule '%s' in campaign '%s'.", campaign->rules[i]->name, campaign->name);
				context->errorCount++;
				break;
			}
		}
	}

	context->campaign = campaign;
	context->enforceEntityBinding = campaign->entityCount > 0;

	for (size_t i = 0; i < campaign->invariantCount; i++) {
		_conditionType(context, campaign->invariants[i]->condition);
	}
	for (size_t i = 0; i < campaign->ruleCount; i++) {
		_conditionType(context, campaign->rules[i]->condition);
		_checkAction(context, campaign->rules[i]->action);
	}

	context->campaign = NULL;
	context->enforceEntityBinding = false;
	symbolTableLeaveScope(context->table);
}

static const CampaignNode * _findCampaignNode(const ProgramNode * program, const char * name) {
	for (size_t i = 0; i < program->topLevelCount; i++) {
		const TopLevelNode * topLevel = program->topLevels[i];
		if (topLevel->kind == TOP_LEVEL_CAMPAIGN && strcmp(topLevel->campaign->name, name) == 0) {
			return topLevel->campaign;
		}
	}
	return NULL;
}

/**
 * Validates a simulation: its model must exist, and every `given`/`expect`
 * constraint must be well-typed against that model's entity schema. The
 * constraints describe a satisfaction problem; this phase guarantees the
 * problem is well-formed before a solver would attempt to answer it.
 */
static void _validateSimulation(SemanticContext * context, const ProgramNode * program, const SimulationNode * simulation) {
	const CampaignNode * campaign = _findCampaignNode(program, simulation->campaignName);
	if (campaign == NULL) {
		logError(_logger, "Simulation '%s' references undefined campaign '%s'.",
			simulation->name, simulation->campaignName);
		context->errorCount++;
		return;
	}

	context->campaign = campaign;
	context->enforceEntityBinding = campaign->entityCount > 0;
	for (size_t i = 0; i < simulation->givenCount; i++) {
		_conditionType(context, simulation->given[i]);
	}
	for (size_t i = 0; i < simulation->expectCount; i++) {
		_conditionType(context, simulation->expect[i]);
	}
	context->campaign = NULL;
	context->enforceEntityBinding = false;
}

/* PUBLIC FUNCTIONS */

CompilationStatus executeSemanticAnalysis(CompilerState * compilerState) {
	logDebugging(_logger, "Analyzing semantics...");
	ProgramNode * program = (ProgramNode *) compilerState->abstractSyntaxtTree;
	if (program == NULL) {
		return SUCCEEDED;
	}

	SymbolTable * table = symbolTableCreate();
	compilerState->symbolTable = table;
	SemanticContext context = {
		.table = table,
		.campaign = NULL,
		.enforceEntityBinding = false,
		.errorCount = 0
	};

	symbolTableEnterScope(table); /* Global scope. */

	/* Pass 1: declare every campaign so exports/simulations can reference them in any order. */
	for (size_t i = 0; i < program->topLevelCount; i++) {
		TopLevelNode * topLevel = program->topLevels[i];
		if (topLevel->kind == TOP_LEVEL_CAMPAIGN) {
			if (!symbolTableDeclare(table, topLevel->campaign->name, SYMBOL_CAMPAIGN, NULL)) {
				logError(_logger, "Duplicate campaign '%s'.", topLevel->campaign->name);
				context.errorCount++;
			}
		}
	}

	/* Pass 2: validate campaign bodies, export references and simulations. */
	for (size_t i = 0; i < program->topLevelCount; i++) {
		TopLevelNode * topLevel = program->topLevels[i];
		switch (topLevel->kind) {
			case TOP_LEVEL_CAMPAIGN:
				_validateCampaign(&context, topLevel->campaign);
				break;
			case TOP_LEVEL_EXPORT: {
				Symbol * symbol = symbolTableLookup(table, topLevel->exportNode->campaignName);
				if (symbol == NULL || symbol->kind != SYMBOL_CAMPAIGN) {
					logError(_logger, "Export references undefined campaign '%s'.", topLevel->exportNode->campaignName);
					context.errorCount++;
				}
				break;
			}
			case TOP_LEVEL_SIMULATION:
				_validateSimulation(&context, program, topLevel->simulation);
				break;
		}
	}

	symbolTableLeaveScope(table); /* Global scope. */
	symbolTableDestroy(table);
	compilerState->symbolTable = NULL;

	if (context.errorCount > 0) {
		logError(_logger, "Semantic analysis failed with %d error(s).", context.errorCount);
		return FAILED;
	}
	logDebugging(_logger, "Semantic analysis succeeded.");
	return SUCCEEDED;
}

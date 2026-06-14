#include "SemanticAnalyzer.h"

#include "SymbolTable.h"

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
 * entity field, and TYPE_ERROR is the bottom type (⊥) of the "Sistema de Tipos"
 * specification, signalling a malformed/ill-typed construction.
 */
typedef enum {
	TYPE_INTEGER,
	TYPE_PERCENTAGE,
	TYPE_STRING,
	TYPE_BOOLEAN,
	TYPE_FIELD,
	TYPE_ERROR
} PrlType;

/** The state carried while validating one campaign. */
typedef struct {
	SymbolTable * table;
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

/** True if two literal types can be compared (numeric coercion integer↔percentage). */
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

/** type(expression): literals map to their type; field accesses to TYPE_FIELD. */
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
		case EXPRESSION_FIELD_ACCESS:
			if (context->enforceEntityBinding) {
				Symbol * symbol = symbolTableLookup(context->table, expression->fieldAccess->entityName);
				if (symbol == NULL || symbol->kind != SYMBOL_ENTITY) {
					logError(_logger, "Undefined entity '%s' referenced in '%s.%s'.",
						expression->fieldAccess->entityName,
						expression->fieldAccess->entityName,
						expression->fieldAccess->fieldName);
					context->errorCount++;
				}
			}
			return TYPE_FIELD;
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

	context->enforceEntityBinding = campaign->entityCount > 0;

	for (size_t i = 0; i < campaign->invariantCount; i++) {
		_conditionType(context, campaign->invariants[i]->condition);
	}
	for (size_t i = 0; i < campaign->ruleCount; i++) {
		_conditionType(context, campaign->rules[i]->condition);
		_checkAction(context, campaign->rules[i]->action);
	}

	symbolTableLeaveScope(context->table);
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
		.enforceEntityBinding = false,
		.errorCount = 0
	};

	symbolTableEnterScope(table); /* Global scope. */

	/* Pass 1: declare every campaign so exports can reference them in any order. */
	for (size_t i = 0; i < program->topLevelCount; i++) {
		TopLevelNode * topLevel = program->topLevels[i];
		if (topLevel->kind == TOP_LEVEL_CAMPAIGN) {
			if (!symbolTableDeclare(table, topLevel->campaign->name, SYMBOL_CAMPAIGN, NULL)) {
				logError(_logger, "Duplicate campaign '%s'.", topLevel->campaign->name);
				context.errorCount++;
			}
		}
	}

	/* Pass 2: validate campaign bodies and export references. */
	for (size_t i = 0; i < program->topLevelCount; i++) {
		TopLevelNode * topLevel = program->topLevels[i];
		if (topLevel->kind == TOP_LEVEL_CAMPAIGN) {
			_validateCampaign(&context, topLevel->campaign);
		}
		else {
			Symbol * symbol = symbolTableLookup(table, topLevel->exportNode->campaignName);
			if (symbol == NULL || symbol->kind != SYMBOL_CAMPAIGN) {
				logError(_logger, "Export references undefined campaign '%s'.", topLevel->exportNode->campaignName);
				context.errorCount++;
			}
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

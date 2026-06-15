#include "AbstractSyntaxTree.h"

#include <string.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* PUBLIC FUNCTIONS */

static char * _copyString(const char * value) {
	if (value == NULL) {
		return NULL;
	}
	size_t length = strlen(value);
	char * copy = (char *) calloc(length + 1, sizeof(char));
	strncpy(copy, value, length);
	return copy;
}

static void _printIndent(FILE * output, unsigned int level) {
	for (unsigned int i = 0; i < level; i++) {
		fprintf(output, "  ");
	}
}

static void _printConditionNode(FILE * output, const ConditionNode * condition, unsigned int level);
static void _printExpressionNode(FILE * output, const ExpressionNode * expression, unsigned int level);

static const char * _actionKindName(ActionKind kind) {
	switch (kind) {
		case ACTION_DISCOUNT: return "discount";
		case ACTION_DISCOUNT_FIXED: return "discount_fixed";
		case ACTION_SURCHARGE: return "surcharge";
		case ACTION_REJECT: return "reject";
		default: return "unknown";
	}
}

static const char * _conditionKindName(ConditionKind kind) {
	switch (kind) {
		case CONDITION_OR: return "or";
		case CONDITION_AND: return "and";
		case CONDITION_NOT: return "not";
		case CONDITION_COMPARISON: return "comparison";
		case CONDITION_IN: return "in";
		case CONDITION_EXPRESSION: return "expression";
		default: return "unknown";
	}
}

static const char * _relOpKindName(RelOpKind kind) {
	switch (kind) {
		case REL_OP_EQ: return "==";
		case REL_OP_NEQ: return "!=";
		case REL_OP_GT: return ">";
		case REL_OP_LT: return "<";
		case REL_OP_GTE: return ">=";
		case REL_OP_LTE: return "<=";
		default: return "unknown";
	}
}

static const char * _arithOpKindName(ArithOpKind kind) {
	switch (kind) {
		case ARITH_ADD: return "+";
		case ARITH_SUB: return "-";
		case ARITH_MUL: return "*";
		case ARITH_DIV: return "/";
		default: return "?";
	}
}

static void _printActionNode(FILE * output, const ActionNode * action, unsigned int level) {
	if (action == NULL) {
		return;
	}
	_printIndent(output, level);
	fprintf(output, "Action(%s)\n", _actionKindName(action->kind));
	if (action->value != NULL) {
		_printExpressionNode(output, action->value, level + 1);
	}
	if (action->message != NULL) {
		_printIndent(output, level + 1);
		fprintf(output, "Message(%s)\n", action->message);
	}
}

static void _printExpressionNode(FILE * output, const ExpressionNode * expression, unsigned int level) {
	if (expression == NULL) {
		return;
	}
	_printIndent(output, level);
	switch (expression->kind) {
		case EXPRESSION_FIELD_ACCESS:
			fprintf(output, "FieldAccess(%s.%s)\n", expression->fieldAccess->entityName, expression->fieldAccess->fieldName);
			break;
		case EXPRESSION_LITERAL:
			switch (expression->literal->kind) {
				case LITERAL_INTEGER:
					fprintf(output, "Integer(%d)\n", expression->literal->integerValue);
					break;
				case LITERAL_STRING:
					fprintf(output, "String(%s)\n", expression->literal->stringValue);
					break;
				case LITERAL_PERCENTAGE:
					fprintf(output, "Percentage(%d%%)\n", expression->literal->percentageValue);
					break;
			}
			break;
		case EXPRESSION_ARITHMETIC:
			fprintf(output, "Arithmetic(%s)\n", _arithOpKindName(expression->arithOp));
			_printExpressionNode(output, expression->leftOperand, level + 1);
			_printExpressionNode(output, expression->rightOperand, level + 1);
			break;
	}
}

static void _printConditionNode(FILE * output, const ConditionNode * condition, unsigned int level) {
	if (condition == NULL) {
		return;
	}
	_printIndent(output, level);
	fprintf(output, "Condition(%s)\n", _conditionKindName(condition->kind));
	switch (condition->kind) {
		case CONDITION_OR:
		case CONDITION_AND:
			_printConditionNode(output, condition->left, level + 1);
			_printConditionNode(output, condition->right, level + 1);
			break;
		case CONDITION_NOT:
			_printConditionNode(output, condition->operand, level + 1);
			break;
		case CONDITION_COMPARISON:
			_printExpressionNode(output, condition->leftExpression, level + 1);
			_printIndent(output, level + 1);
			fprintf(output, "Operator(%s)\n", _relOpKindName(condition->relOp));
			_printExpressionNode(output, condition->rightExpression, level + 1);
			break;
		case CONDITION_IN:
			_printExpressionNode(output, condition->inExpression, level + 1);
			for (size_t i = 0; i < condition->optionCount; i++) {
				_printExpressionNode(output, condition->options[i], level + 1);
			}
			break;
		case CONDITION_EXPRESSION:
			_printExpressionNode(output, condition->expression, level + 1);
			break;
	}
}

void destroyConstant(Constant * constant) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (constant != NULL) {
		free(constant);
	}
}

void destroyExpression(Expression * expression) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (expression != NULL) {
		switch (expression->type) {
			case ADDITION:
			case DIVISION:
			case MULTIPLICATION:
			case SUBTRACTION:
				destroyExpression(expression->leftExpression);
				destroyExpression(expression->rightExpression);
				break;
			case FACTOR:
				destroyFactor(expression->factor);
				break;
		}
		free(expression);
	}
}

void destroyFactor(Factor * factor) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (factor != NULL) {
		switch (factor->type) {
			case CONSTANT:
				destroyConstant(factor->constant);
				break;
			case EXPRESSION:
				destroyExpression(factor->expression);
				break;
		}
		free(factor);
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		destroyExpression(program->expression);
		free(program);
	}
}

ProgramNode * createProgramNode(void) {
	return (ProgramNode *) calloc(1, sizeof(ProgramNode));
}

TopLevelNode * createTopLevelCampaignNode(CampaignNode * campaign) {
	TopLevelNode * topLevel = (TopLevelNode *) calloc(1, sizeof(TopLevelNode));
	topLevel->kind = TOP_LEVEL_CAMPAIGN;
	topLevel->campaign = campaign;
	return topLevel;
}

TopLevelNode * createTopLevelExportNode(ExportNode * exportNode) {
	TopLevelNode * topLevel = (TopLevelNode *) calloc(1, sizeof(TopLevelNode));
	topLevel->kind = TOP_LEVEL_EXPORT;
	topLevel->exportNode = exportNode;
	return topLevel;
}

TopLevelNode * createTopLevelSimulationNode(SimulationNode * simulation) {
	TopLevelNode * topLevel = (TopLevelNode *) calloc(1, sizeof(TopLevelNode));
	topLevel->kind = TOP_LEVEL_SIMULATION;
	topLevel->simulation = simulation;
	return topLevel;
}

CampaignNode * createCampaignNode(const char * name) {
	CampaignNode * campaign = (CampaignNode *) calloc(1, sizeof(CampaignNode));
	campaign->name = _copyString(name);
	return campaign;
}

EntityDeclNode * createEntityDeclNode(const char * name, const char * typeName) {
	EntityDeclNode * entity = (EntityDeclNode *) calloc(1, sizeof(EntityDeclNode));
	entity->name = _copyString(name);
	entity->typeName = _copyString(typeName);
	return entity;
}

PropertyNode * createPropertyNode(const char * name, const char * typeName) {
	PropertyNode * property = (PropertyNode *) calloc(1, sizeof(PropertyNode));
	property->name = _copyString(name);
	property->typeName = _copyString(typeName);
	return property;
}

InvariantNode * createInvariantNode(ConditionNode * condition) {
	InvariantNode * invariant = (InvariantNode *) calloc(1, sizeof(InvariantNode));
	invariant->condition = condition;
	return invariant;
}

RuleNode * createRuleNode(const char * name, int priority, ConditionNode * condition, ActionNode * action) {
	RuleNode * rule = (RuleNode *) calloc(1, sizeof(RuleNode));
	rule->name = _copyString(name);
	rule->priority = priority;
	rule->condition = condition;
	rule->action = action;
	return rule;
}

SimulationNode * createSimulationNode(const char * name, const char * campaignName) {
	SimulationNode * simulation = (SimulationNode *) calloc(1, sizeof(SimulationNode));
	simulation->name = _copyString(name);
	simulation->campaignName = _copyString(campaignName);
	simulation->satExpectation = SAT_UNSPECIFIED;
	return simulation;
}

ConditionNode * createBinaryConditionNode(ConditionKind kind, ConditionNode * left, ConditionNode * right) {
	ConditionNode * condition = (ConditionNode *) calloc(1, sizeof(ConditionNode));
	condition->kind = kind;
	condition->left = left;
	condition->right = right;
	return condition;
}

ConditionNode * createUnaryConditionNode(ConditionKind kind, ConditionNode * operand) {
	ConditionNode * condition = (ConditionNode *) calloc(1, sizeof(ConditionNode));
	condition->kind = kind;
	condition->operand = operand;
	return condition;
}

ConditionNode * createComparisonConditionNode(ExpressionNode * leftExpression, RelOpKind relOp, ExpressionNode * rightExpression) {
	ConditionNode * condition = (ConditionNode *) calloc(1, sizeof(ConditionNode));
	condition->kind = CONDITION_COMPARISON;
	condition->leftExpression = leftExpression;
	condition->rightExpression = rightExpression;
	condition->relOp = relOp;
	return condition;
}

ConditionNode * createInConditionNode(ExpressionNode * expression, ExpressionNode ** options, size_t optionCount) {
	ConditionNode * condition = (ConditionNode *) calloc(1, sizeof(ConditionNode));
	condition->kind = CONDITION_IN;
	condition->inExpression = expression;
	condition->options = options;
	condition->optionCount = optionCount;
	return condition;
}

ConditionNode * createExpressionConditionNode(ExpressionNode * expression) {
	ConditionNode * condition = (ConditionNode *) calloc(1, sizeof(ConditionNode));
	condition->kind = CONDITION_EXPRESSION;
	condition->expression = expression;
	return condition;
}

ExpressionNode * createFieldAccessExpressionNode(const char * entityName, const char * fieldName) {
	ExpressionNode * expression = (ExpressionNode *) calloc(1, sizeof(ExpressionNode));
	FieldAccessNode * fieldAccess = (FieldAccessNode *) calloc(1, sizeof(FieldAccessNode));
	fieldAccess->entityName = _copyString(entityName);
	fieldAccess->fieldName = _copyString(fieldName);
	expression->kind = EXPRESSION_FIELD_ACCESS;
	expression->fieldAccess = fieldAccess;
	return expression;
}

ExpressionNode * createIntegerLiteralNode(int value) {
	ExpressionNode * expression = (ExpressionNode *) calloc(1, sizeof(ExpressionNode));
	LiteralNode * literal = (LiteralNode *) calloc(1, sizeof(LiteralNode));
	literal->kind = LITERAL_INTEGER;
	literal->integerValue = value;
	expression->kind = EXPRESSION_LITERAL;
	expression->literal = literal;
	return expression;
}

ExpressionNode * createStringLiteralNode(const char * value) {
	ExpressionNode * expression = (ExpressionNode *) calloc(1, sizeof(ExpressionNode));
	LiteralNode * literal = (LiteralNode *) calloc(1, sizeof(LiteralNode));
	literal->kind = LITERAL_STRING;
	literal->stringValue = _copyString(value);
	expression->kind = EXPRESSION_LITERAL;
	expression->literal = literal;
	return expression;
}

ExpressionNode * createPercentageLiteralNode(int value) {
	ExpressionNode * expression = (ExpressionNode *) calloc(1, sizeof(ExpressionNode));
	LiteralNode * literal = (LiteralNode *) calloc(1, sizeof(LiteralNode));
	literal->kind = LITERAL_PERCENTAGE;
	literal->percentageValue = value;
	expression->kind = EXPRESSION_LITERAL;
	expression->literal = literal;
	return expression;
}

ExpressionNode * createArithmeticExpressionNode(ArithOpKind arithOp, ExpressionNode * leftOperand, ExpressionNode * rightOperand) {
	ExpressionNode * expression = (ExpressionNode *) calloc(1, sizeof(ExpressionNode));
	expression->kind = EXPRESSION_ARITHMETIC;
	expression->arithOp = arithOp;
	expression->leftOperand = leftOperand;
	expression->rightOperand = rightOperand;
	return expression;
}

ActionNode * createActionNode(ActionKind kind, ExpressionNode * value, const char * message) {
	ActionNode * action = (ActionNode *) calloc(1, sizeof(ActionNode));
	action->kind = kind;
	action->value = value;
	action->message = _copyString(message);
	return action;
}

ExportNode * createExportNode(const char * campaignName, const char * format) {
	ExportNode * exportNode = (ExportNode *) calloc(1, sizeof(ExportNode));
	exportNode->campaignName = _copyString(campaignName);
	exportNode->format = _copyString(format);
	return exportNode;
}

void addTopLevelNode(ProgramNode * program, TopLevelNode * topLevel) {
	program->topLevels = (TopLevelNode **) realloc(program->topLevels, sizeof(TopLevelNode *) * (program->topLevelCount + 1));
	program->topLevels[program->topLevelCount++] = topLevel;
}

void addCampaignEntityNode(CampaignNode * campaign, EntityDeclNode * entity) {
	campaign->entities = (EntityDeclNode **) realloc(campaign->entities, sizeof(EntityDeclNode *) * (campaign->entityCount + 1));
	campaign->entities[campaign->entityCount++] = entity;
}

void addEntityPropertyNode(EntityDeclNode * entity, PropertyNode * property) {
	entity->properties = (PropertyNode **) realloc(entity->properties, sizeof(PropertyNode *) * (entity->propertyCount + 1));
	entity->properties[entity->propertyCount++] = property;
}

void addCampaignInvariantNode(CampaignNode * campaign, InvariantNode * invariant) {
	campaign->invariants = (InvariantNode **) realloc(campaign->invariants, sizeof(InvariantNode *) * (campaign->invariantCount + 1));
	campaign->invariants[campaign->invariantCount++] = invariant;
}

void addCampaignRuleNode(CampaignNode * campaign, RuleNode * rule) {
	campaign->rules = (RuleNode **) realloc(campaign->rules, sizeof(RuleNode *) * (campaign->ruleCount + 1));
	campaign->rules[campaign->ruleCount++] = rule;
}

void addSimulationGivenNode(SimulationNode * simulation, ConditionNode * condition) {
	simulation->given = (ConditionNode **) realloc(simulation->given, sizeof(ConditionNode *) * (simulation->givenCount + 1));
	simulation->given[simulation->givenCount++] = condition;
}

void addSimulationExpectNode(SimulationNode * simulation, ConditionNode * condition) {
	simulation->expect = (ConditionNode **) realloc(simulation->expect, sizeof(ConditionNode *) * (simulation->expectCount + 1));
	simulation->expect[simulation->expectCount++] = condition;
}

void destroyActionNode(ActionNode * action) {
	if (action != NULL) {
		destroyExpressionNode(action->value);
		free(action->message);
		free(action);
	}
}

void destroyCampaignNode(CampaignNode * campaign) {
	if (campaign != NULL) {
		for (size_t i = 0; i < campaign->entityCount; i++) {
			destroyEntityDeclNode(campaign->entities[i]);
		}
		for (size_t i = 0; i < campaign->invariantCount; i++) {
			destroyInvariantNode(campaign->invariants[i]);
		}
		for (size_t i = 0; i < campaign->ruleCount; i++) {
			destroyRuleNode(campaign->rules[i]);
		}
		free(campaign->entities);
		free(campaign->invariants);
		free(campaign->rules);
		free(campaign->name);
		free(campaign);
	}
}

void destroyConditionNode(ConditionNode * condition) {
	if (condition != NULL) {
		switch (condition->kind) {
			case CONDITION_OR:
			case CONDITION_AND:
				destroyConditionNode(condition->left);
				destroyConditionNode(condition->right);
				break;
			case CONDITION_NOT:
				destroyConditionNode(condition->operand);
				break;
			case CONDITION_COMPARISON:
				destroyExpressionNode(condition->leftExpression);
				destroyExpressionNode(condition->rightExpression);
				break;
			case CONDITION_IN:
				destroyExpressionNode(condition->inExpression);
				for (size_t i = 0; i < condition->optionCount; i++) {
					destroyExpressionNode(condition->options[i]);
				}
				free(condition->options);
				break;
			case CONDITION_EXPRESSION:
				destroyExpressionNode(condition->expression);
				break;
		}
		free(condition);
	}
}

void destroyEntityDeclNode(EntityDeclNode * entity) {
	if (entity != NULL) {
		for (size_t i = 0; i < entity->propertyCount; i++) {
			destroyPropertyNode(entity->properties[i]);
		}
		free(entity->properties);
		free(entity->name);
		free(entity->typeName);
		free(entity);
	}
}

void destroyPropertyNode(PropertyNode * property) {
	if (property != NULL) {
		free(property->name);
		free(property->typeName);
		free(property);
	}
}

void destroyExportNode(ExportNode * exportNode) {
	if (exportNode != NULL) {
		free(exportNode->campaignName);
		free(exportNode->format);
		free(exportNode);
	}
}

void destroyExpressionNode(ExpressionNode * expression) {
	if (expression != NULL) {
		switch (expression->kind) {
			case EXPRESSION_FIELD_ACCESS:
				destroyFieldAccessNode(expression->fieldAccess);
				break;
			case EXPRESSION_LITERAL:
				destroyLiteralNode(expression->literal);
				break;
			case EXPRESSION_ARITHMETIC:
				destroyExpressionNode(expression->leftOperand);
				destroyExpressionNode(expression->rightOperand);
				break;
		}
		free(expression);
	}
}

void destroyFieldAccessNode(FieldAccessNode * fieldAccess) {
	if (fieldAccess != NULL) {
		free(fieldAccess->entityName);
		free(fieldAccess->fieldName);
		free(fieldAccess);
	}
}

void destroyInvariantNode(InvariantNode * invariant) {
	if (invariant != NULL) {
		destroyConditionNode(invariant->condition);
		free(invariant);
	}
}

void destroyLiteralNode(LiteralNode * literal) {
	if (literal != NULL) {
		if (literal->kind == LITERAL_STRING) {
			free(literal->stringValue);
		}
		free(literal);
	}
}

void destroyProgramNode(ProgramNode * program) {
	if (program != NULL) {
		for (size_t i = 0; i < program->topLevelCount; i++) {
			destroyTopLevelNode(program->topLevels[i]);
		}
		free(program->topLevels);
		free(program);
	}
}

void destroyRuleNode(RuleNode * rule) {
	if (rule != NULL) {
		free(rule->name);
		destroyConditionNode(rule->condition);
		destroyActionNode(rule->action);
		free(rule);
	}
}

void destroySimulationNode(SimulationNode * simulation) {
	if (simulation != NULL) {
		for (size_t i = 0; i < simulation->givenCount; i++) {
			destroyConditionNode(simulation->given[i]);
		}
		for (size_t i = 0; i < simulation->expectCount; i++) {
			destroyConditionNode(simulation->expect[i]);
		}
		free(simulation->given);
		free(simulation->expect);
		free(simulation->name);
		free(simulation->campaignName);
		free(simulation);
	}
}

void destroyTopLevelNode(TopLevelNode * topLevel) {
	if (topLevel != NULL) {
		switch (topLevel->kind) {
			case TOP_LEVEL_CAMPAIGN:
				destroyCampaignNode(topLevel->campaign);
				break;
			case TOP_LEVEL_EXPORT:
				destroyExportNode(topLevel->exportNode);
				break;
			case TOP_LEVEL_SIMULATION:
				destroySimulationNode(topLevel->simulation);
				break;
		}
		free(topLevel);
	}
}

static const char * _satExpectationName(SatExpectation expectation) {
	switch (expectation) {
		case SAT_SATISFIABLE: return "satisfiable";
		case SAT_UNSATISFIABLE: return "unsatisfiable";
		default: return "unspecified";
	}
}

void printProgramNode(FILE * output, const ProgramNode * program) {
	if (output == NULL || program == NULL) {
		return;
	}
	fprintf(output, "Program\n");
	for (size_t i = 0; i < program->topLevelCount; i++) {
		TopLevelNode * topLevel = program->topLevels[i];
		if (topLevel->kind == TOP_LEVEL_CAMPAIGN) {
			CampaignNode * campaign = topLevel->campaign;
			_printIndent(output, 1);
			fprintf(output, "Campaign(%s)\n", campaign->name);
			for (size_t j = 0; j < campaign->entityCount; j++) {
				const EntityDeclNode * entity = campaign->entities[j];
				_printIndent(output, 2);
				fprintf(output, "Entity(%s: %s)\n", entity->name, entity->typeName);
				for (size_t k = 0; k < entity->propertyCount; k++) {
					_printIndent(output, 3);
					fprintf(output, "Property(%s: %s)\n", entity->properties[k]->name, entity->properties[k]->typeName);
				}
			}
			for (size_t j = 0; j < campaign->invariantCount; j++) {
				_printIndent(output, 2);
				fprintf(output, "Invariant\n");
				_printConditionNode(output, campaign->invariants[j]->condition, 3);
			}
			for (size_t j = 0; j < campaign->ruleCount; j++) {
				RuleNode * rule = campaign->rules[j];
				_printIndent(output, 2);
				fprintf(output, "Rule(%s, priority=%d)\n", rule->name, rule->priority);
				_printConditionNode(output, rule->condition, 3);
				_printActionNode(output, rule->action, 3);
			}
		}
		else if (topLevel->kind == TOP_LEVEL_SIMULATION) {
			SimulationNode * simulation = topLevel->simulation;
			_printIndent(output, 1);
			fprintf(output, "Simulation(%s on %s, expect=%s)\n", simulation->name, simulation->campaignName, _satExpectationName(simulation->satExpectation));
			for (size_t j = 0; j < simulation->givenCount; j++) {
				_printIndent(output, 2);
				fprintf(output, "Given\n");
				_printConditionNode(output, simulation->given[j], 3);
			}
			for (size_t j = 0; j < simulation->expectCount; j++) {
				_printIndent(output, 2);
				fprintf(output, "Expect\n");
				_printConditionNode(output, simulation->expect[j], 3);
			}
		}
		else {
			_printIndent(output, 1);
			fprintf(output, "Export(%s to %s)\n", topLevel->exportNode->campaignName, topLevel->exportNode->format);
		}
	}
}

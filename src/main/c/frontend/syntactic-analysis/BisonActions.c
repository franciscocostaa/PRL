#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;
static ProgramNode * _buildingProgram = NULL;
static CampaignNode * _buildingCampaign = NULL;

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	if (_buildingProgram != NULL) {
		destroyProgramNode(_buildingProgram);
		_buildingProgram = NULL;
	}
	if (_buildingCampaign != NULL) {
		destroyCampaignNode(_buildingCampaign);
		_buildingCampaign = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

/* IMPORTED FUNCTIONS */

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

Constant * IntegerConstantSemanticAction(const int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = calloc(1, sizeof(Constant));
	constant->value = value;
	return constant;
}

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = leftExpression;
	expression->rightExpression = rightExpression;
	expression->type = type;
	return expression;
}

Expression * FactorExpressionSemanticAction(Factor * factor) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->factor = factor;
	expression->type = FACTOR;
	return expression;
}

Factor * ConstantFactorSemanticAction(Constant * constant) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->constant = constant;
	factor->type = CONSTANT;
	return factor;
}

Factor * ExpressionFactorSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->expression = expression;
	factor->type = EXPRESSION;
	return factor;
}

Program * ExpressionProgramSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->expression = expression;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

/* PRL SEMANTIC ACTIONS */

void BeginProgramSemanticAction(void) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	_buildingProgram = createProgramNode();
}

ProgramNode * FinalizeProgramSemanticAction(void) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	_compilerState->abstractSyntaxtTree = _buildingProgram;
	ProgramNode * result = _buildingProgram;
	_buildingProgram = NULL;
	return result;
}

void AppendTopLevelCampaignSemanticAction(CampaignNode * campaign) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	addTopLevelNode(_buildingProgram, createTopLevelCampaignNode(campaign));
}

void AppendTopLevelExportSemanticAction(ExportNode * exportNode) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	addTopLevelNode(_buildingProgram, createTopLevelExportNode(exportNode));
}

void BeginCampaignSemanticAction(const char * name) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	_buildingCampaign = createCampaignNode(name);
}

CampaignNode * EndCampaignSemanticAction(void) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	CampaignNode * result = _buildingCampaign;
	_buildingCampaign = NULL;
	return result;
}

void AddEntityDeclSemanticAction(const char * name, const char * typeName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	addCampaignEntityNode(_buildingCampaign, createEntityDeclNode(name, typeName));
}

void AddInvariantSemanticAction(ConditionNode * condition) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	addCampaignInvariantNode(_buildingCampaign, createInvariantNode(condition));
}

void AddRuleSemanticAction(const char * name, int priority, ConditionNode * condition, ActionNode * action) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	addCampaignRuleNode(_buildingCampaign, createRuleNode(name, priority, condition, action));
}

ExpressionList * MakeExpressionListSemanticAction(ExpressionNode * first) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ExpressionList * list = calloc(1, sizeof(ExpressionList));
	list->items = malloc(sizeof(ExpressionNode *));
	list->items[0] = first;
	list->count = 1;
	return list;
}

ExpressionList * AppendExpressionListSemanticAction(ExpressionList * list, ExpressionNode * expr) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	list->items = realloc(list->items, (list->count + 1) * sizeof(ExpressionNode *));
	list->items[list->count++] = expr;
	return list;
}

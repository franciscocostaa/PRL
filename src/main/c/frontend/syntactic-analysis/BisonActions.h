#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState);

/**
 * Bison semantic actions.
 */

Constant * IntegerConstantSemanticAction(const int value);
Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type);
Expression * FactorExpressionSemanticAction(Factor * factor);
Factor * ConstantFactorSemanticAction(Constant * constant);
Factor * ExpressionFactorSemanticAction(Expression * expression);
Program * ExpressionProgramSemanticAction(Expression * expression);

/** PRL semantic actions */

void BeginProgramSemanticAction(void);
ProgramNode * FinalizeProgramSemanticAction(void);
void AppendTopLevelCampaignSemanticAction(CampaignNode * campaign);
void AppendTopLevelExportSemanticAction(ExportNode * exportNode);
void AppendTopLevelSimulationSemanticAction(SimulationNode * simulation);
void BeginCampaignSemanticAction(const char * name);
CampaignNode * EndCampaignSemanticAction(void);
void AddEntityDeclSemanticAction(const char * name, const char * typeName, PropertyList * properties);
void AddInvariantSemanticAction(ConditionNode * condition);
void AddRuleSemanticAction(const char * name, int priority, ConditionNode * condition, ActionNode * action);
void BeginSimulationSemanticAction(const char * name, const char * campaignName);
void AddSimulationGivenSemanticAction(ConditionNode * condition);
void AddSimulationExpectSemanticAction(ConditionNode * condition);
void SetSimulationSatExpectationSemanticAction(SatExpectation expectation);
SimulationNode * EndSimulationSemanticAction(void);
ExpressionList * MakeExpressionListSemanticAction(ExpressionNode * first);
ExpressionList * AppendExpressionListSemanticAction(ExpressionList * list, ExpressionNode * expr);
PropertyList * MakePropertyListSemanticAction(PropertyNode * first);
PropertyList * AppendPropertyListSemanticAction(PropertyList * list, PropertyNode * property);

#endif

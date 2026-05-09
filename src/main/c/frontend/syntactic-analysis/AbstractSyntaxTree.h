#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

/**
 * This type definitions allows self-referencing types (e.g., an expression
 * that is made of another expressions, such as talking about you in 3rd
 * person, but without the madness).
 */

typedef enum ExpressionType ExpressionType;
typedef enum FactorType FactorType;
typedef enum ActionKind ActionKind;
typedef enum ConditionKind ConditionKind;
typedef enum ExpressionKind ExpressionKind;
typedef enum LiteralKind LiteralKind;
typedef enum RelOpKind RelOpKind;
typedef enum TopLevelKind TopLevelKind;

typedef struct Constant Constant;
typedef struct Expression Expression;
typedef struct Factor Factor;
typedef struct Program Program;
typedef struct ActionNode ActionNode;
typedef struct CampaignNode CampaignNode;
typedef struct ConditionNode ConditionNode;
typedef struct EntityDeclNode EntityDeclNode;
typedef struct ExportNode ExportNode;
typedef struct ExpressionNode ExpressionNode;
typedef struct FieldAccessNode FieldAccessNode;
typedef struct InvariantNode InvariantNode;
typedef struct LiteralNode LiteralNode;
typedef struct ProgramNode ProgramNode;
typedef struct RuleNode RuleNode;
typedef struct TopLevelNode TopLevelNode;

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

enum ExpressionType {
	ADDITION,
	DIVISION,
	FACTOR,
	MULTIPLICATION,
	SUBTRACTION
};

enum FactorType {
	CONSTANT,
	EXPRESSION
};

enum TopLevelKind {
	TOP_LEVEL_CAMPAIGN,
	TOP_LEVEL_EXPORT
};

enum ConditionKind {
	CONDITION_OR,
	CONDITION_AND,
	CONDITION_NOT,
	CONDITION_COMPARISON,
	CONDITION_IN,
	CONDITION_EXPRESSION
};

enum ExpressionKind {
	EXPRESSION_FIELD_ACCESS,
	EXPRESSION_LITERAL
};

enum LiteralKind {
	LITERAL_INTEGER,
	LITERAL_STRING,
	LITERAL_PERCENTAGE
};

enum ActionKind {
	ACTION_DISCOUNT,
	ACTION_DISCOUNT_FIXED,
	ACTION_SURCHARGE,
	ACTION_REJECT
};

enum RelOpKind {
	REL_OP_EQ,
	REL_OP_NEQ,
	REL_OP_GT,
	REL_OP_LT,
	REL_OP_GTE,
	REL_OP_LTE
};

struct Constant {
	int value;
};

struct Factor {
	union {
		Constant * constant;
		Expression * expression;
	};
	FactorType type;
};

struct Expression {
	union {
		Factor * factor;
		struct {
			Expression * leftExpression;
			Expression * rightExpression;
		};
	};
	ExpressionType type;
};

struct Program {
	Expression * expression;
};

struct LiteralNode {
	LiteralKind kind;
	union {
		int integerValue;
		char * stringValue;
		int percentageValue;
	};
};

struct FieldAccessNode {
	char * entityName;
	char * fieldName;
};

struct ExpressionNode {
	ExpressionKind kind;
	union {
		FieldAccessNode * fieldAccess;
		LiteralNode * literal;
	};
};

struct ConditionNode {
	ConditionKind kind;
	union {
		struct {
			ConditionNode * left;
			ConditionNode * right;
		};
		ConditionNode * operand;
		struct {
			ExpressionNode * leftExpression;
			ExpressionNode * rightExpression;
			RelOpKind relOp;
		};
		struct {
			ExpressionNode * inExpression;
			ExpressionNode ** options;
			size_t optionCount;
		};
		ExpressionNode * expression;
	};
};

struct ActionNode {
	ActionKind kind;
	ExpressionNode * value;
	char * message;
};

struct EntityDeclNode {
	char * name;
	char * typeName;
};

struct InvariantNode {
	ConditionNode * condition;
};

struct RuleNode {
	char * name;
	int priority;
	ConditionNode * condition;
	ActionNode * action;
};

struct CampaignNode {
	char * name;
	EntityDeclNode ** entities;
	size_t entityCount;
	InvariantNode ** invariants;
	size_t invariantCount;
	RuleNode ** rules;
	size_t ruleCount;
};

struct ExportNode {
	char * campaignName;
	char * format;
};

struct TopLevelNode {
	TopLevelKind kind;
	union {
		CampaignNode * campaign;
		ExportNode * exportNode;
	};
};

struct ProgramNode {
	TopLevelNode ** topLevels;
	size_t topLevelCount;
};

/**
 * Node recursive super-duper-trambolik-destructors.
 */

void destroyConstant(Constant * constant);
void destroyExpression(Expression * expression);
void destroyFactor(Factor * factor);
void destroyProgram(Program * program);

ProgramNode * createProgramNode(void);
TopLevelNode * createTopLevelCampaignNode(CampaignNode * campaign);
TopLevelNode * createTopLevelExportNode(ExportNode * exportNode);
CampaignNode * createCampaignNode(const char * name);
EntityDeclNode * createEntityDeclNode(const char * name, const char * typeName);
InvariantNode * createInvariantNode(ConditionNode * condition);
RuleNode * createRuleNode(const char * name, int priority, ConditionNode * condition, ActionNode * action);
ConditionNode * createBinaryConditionNode(ConditionKind kind, ConditionNode * left, ConditionNode * right);
ConditionNode * createUnaryConditionNode(ConditionKind kind, ConditionNode * operand);
ConditionNode * createComparisonConditionNode(ExpressionNode * leftExpression, RelOpKind relOp, ExpressionNode * rightExpression);
ConditionNode * createInConditionNode(ExpressionNode * expression, ExpressionNode ** options, size_t optionCount);
ConditionNode * createExpressionConditionNode(ExpressionNode * expression);
ExpressionNode * createFieldAccessExpressionNode(const char * entityName, const char * fieldName);
ExpressionNode * createIntegerLiteralNode(int value);
ExpressionNode * createStringLiteralNode(const char * value);
ExpressionNode * createPercentageLiteralNode(int value);
ActionNode * createActionNode(ActionKind kind, ExpressionNode * value, const char * message);
ExportNode * createExportNode(const char * campaignName, const char * format);

void addTopLevelNode(ProgramNode * program, TopLevelNode * topLevel);
void addCampaignEntityNode(CampaignNode * campaign, EntityDeclNode * entity);
void addCampaignInvariantNode(CampaignNode * campaign, InvariantNode * invariant);
void addCampaignRuleNode(CampaignNode * campaign, RuleNode * rule);

void destroyActionNode(ActionNode * action);
void destroyCampaignNode(CampaignNode * campaign);
void destroyConditionNode(ConditionNode * condition);
void destroyEntityDeclNode(EntityDeclNode * entity);
void destroyExportNode(ExportNode * exportNode);
void destroyExpressionNode(ExpressionNode * expression);
void destroyFieldAccessNode(FieldAccessNode * fieldAccess);
void destroyInvariantNode(InvariantNode * invariant);
void destroyLiteralNode(LiteralNode * literal);
void destroyProgramNode(ProgramNode * program);
void destroyRuleNode(RuleNode * rule);
void destroyTopLevelNode(TopLevelNode * topLevel);

void printProgramNode(FILE * output, const ProgramNode * program);

/** Temporary list type used during parsing of IN conditions. */
typedef struct {
	ExpressionNode ** items;
	size_t count;
} ExpressionList;

#endif

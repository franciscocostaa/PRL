%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"
#include <stdio.h>

/**
 * The error reporting function for Bison parser.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Tracking-Locations.html
 */
void yyerror(const YYLTYPE * location, const char * message) {
	fprintf(stderr, "Syntax error at line %d: %s\n", location->first_line, message);
}

%}

// Placed in the generated BisonParser.h so the union types are visible to all
// files that include that header (e.g. SemanticValue.h).
%code requires {
#include "AbstractSyntaxTree.h"
}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */

	signed int integer;
	char * string;
	TokenLabel token;

	/** Non-terminals (calculator — kept so Calculator.c still compiles). */

	Constant * constant;
	Expression * expression;
	Factor * factor;
	Program * program;

	/** Non-terminals (PRL). */

	ActionNode * actionNode;
	CampaignNode * campaignNode;
	ConditionNode * conditionNode;
	EntityDeclNode * entityDeclNode;
	ExportNode * exportNode;
	ExpressionNode * expressionNode;
	FieldAccessNode * fieldAccessNode;
	InvariantNode * invariantNode;
	LiteralNode * literalNode;
	ProgramNode * programNode;
	RuleNode * ruleNode;
	TopLevelNode * topLevelNode;
	RelOpKind relOp;
	ExpressionList * expressionList;
}

/**
 * Destructors for heap-owned grammar symbols. Called during error recovery
 * when symbols are popped off the parse stack.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { free($$); } <string>

%destructor { destroyCampaignNode($$);   } <campaignNode>
%destructor { destroyConditionNode($$);  } <conditionNode>
%destructor { destroyExpressionNode($$); } <expressionNode>
%destructor { destroyActionNode($$);     } <actionNode>
%destructor { destroyExportNode($$);     } <exportNode>
%destructor {
	if ($$) {
		for (size_t i = 0; i < $$->count; i++) destroyExpressionNode($$->items[i]);
		free($$->items);
		free($$);
	}
} <expressionList>

/** Terminals. */
%token <integer> INTEGER
%token <string> ID
%token <string> STRING_LITERAL
%token <token> ADD
%token <token> AND
%token <token> ASSERT
%token <token> CAMPAIGN
%token <token> CLOSE_BRACE
%token <token> CLOSE_BRACKET
%token <token> CLOSE_COMMENT
%token <token> CLOSE_PARENTHESIS
%token <token> COLON
%token <token> COMMA
%token <token> DIV
%token <token> DISCOUNT
%token <token> DISCOUNT_FIXED
%token <token> DOT
%token <token> ENTITIES
%token <token> EQ
%token <token> EXPORT
%token <token> GT
%token <token> GTE
%token <token> IF
%token <token> IN
%token <token> INVARIANTS
%token <token> LT
%token <token> LTE
%token <token> MUL
%token <token> NEQ
%token <token> NOT
%token <token> OPEN_BRACE
%token <token> OPEN_BRACKET
%token <token> OPEN_COMMENT
%token <token> OPEN_PARENTHESIS
%token <token> OR
%token <token> PERCENT
%token <token> PRIORITY
%token <token> REJECT_ACTION
%token <token> RULE
%token <token> RULES
%token <token> SUB
%token <token> SURCHARGE
%token <token> THEN
%token <token> TO

%token <token> IGNORED
%token <token> UNKNOWN

/** Non-terminals (PRL). */
%type <programNode>    program
%type <campaignNode>   campaign
%type <exportNode>     export_stmt
%type <conditionNode>  condition
%type <expressionNode> expression
%type <actionNode>     action
%type <relOp>          rel_op
%type <expressionList> expression_list

/**
 * Logical operator precedence: not > and > or.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%left OR
%left AND
%right NOT

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

program: program_start top_level_list
	{ $$ = FinalizeProgramSemanticAction(); }
	;

program_start: %empty
	{ BeginProgramSemanticAction(); }
	;

top_level_list: %empty
	| top_level_list campaign
		{ AppendTopLevelCampaignSemanticAction($2); }
	| top_level_list export_stmt
		{ AppendTopLevelExportSemanticAction($2); }
	;

campaign: campaign_header OPEN_BRACE campaign_body CLOSE_BRACE
	{ $$ = EndCampaignSemanticAction(); }
	;

campaign_header: CAMPAIGN ID
	{ BeginCampaignSemanticAction($2); free($2); $2 = NULL; }
	;

campaign_body: %empty
	| campaign_body entities_section
	| campaign_body invariants_section
	| campaign_body rules_section
	;

entities_section: ENTITIES COLON entity_decl_list
	;

invariants_section: INVARIANTS COLON invariant_list
	;

rules_section: RULES COLON rule_list
	;

entity_decl_list: %empty
	| entity_decl_list entity_decl
	;

invariant_list: %empty
	| invariant_list invariant
	;

rule_list: %empty
	| rule_list rule
	;

entity_decl: ID COLON ID
	{ AddEntityDeclSemanticAction($1, $3); free($1); free($3); $1 = NULL; $3 = NULL; }
	;

invariant: ASSERT condition
	{ AddInvariantSemanticAction($2); }
	;

rule: RULE STRING_LITERAL PRIORITY INTEGER OPEN_BRACE IF condition THEN action CLOSE_BRACE
	{ AddRuleSemanticAction($2, $4, $7, $9); free($2); $2 = NULL; }
	;

condition: condition OR condition
		{ $$ = createBinaryConditionNode(CONDITION_OR, $1, $3); }
	| condition AND condition
		{ $$ = createBinaryConditionNode(CONDITION_AND, $1, $3); }
	| NOT condition
		{ $$ = createUnaryConditionNode(CONDITION_NOT, $2); }
	| expression rel_op expression
		{ $$ = createComparisonConditionNode($1, $2, $3); }
	| expression IN OPEN_BRACKET expression_list CLOSE_BRACKET
		{ $$ = createInConditionNode($1, $4->items, $4->count); free($4->items); free($4); }
	| OPEN_PARENTHESIS condition CLOSE_PARENTHESIS
		{ $$ = $2; }
	| expression
		{ $$ = createExpressionConditionNode($1); }
	;

rel_op: EQ	{ $$ = REL_OP_EQ;  }
	| NEQ	{ $$ = REL_OP_NEQ; }
	| GT	{ $$ = REL_OP_GT;  }
	| LT	{ $$ = REL_OP_LT;  }
	| GTE	{ $$ = REL_OP_GTE; }
	| LTE	{ $$ = REL_OP_LTE; }
	;

expression: ID DOT ID
		{ $$ = createFieldAccessExpressionNode($1, $3); free($1); free($3); $1 = NULL; $3 = NULL; }
	| INTEGER
		{ $$ = createIntegerLiteralNode($1); }
	| STRING_LITERAL
		{ $$ = createStringLiteralNode($1); free($1); $1 = NULL; }
	| INTEGER PERCENT
		{ $$ = createPercentageLiteralNode($1); }
	;

expression_list: expression
		{ $$ = MakeExpressionListSemanticAction($1); }
	| expression_list COMMA expression
		{ $$ = AppendExpressionListSemanticAction($1, $3); }
	;

action: DISCOUNT OPEN_PARENTHESIS expression CLOSE_PARENTHESIS
		{ $$ = createActionNode(ACTION_DISCOUNT, $3, NULL); }
	| DISCOUNT_FIXED OPEN_PARENTHESIS expression CLOSE_PARENTHESIS
		{ $$ = createActionNode(ACTION_DISCOUNT_FIXED, $3, NULL); }
	| SURCHARGE OPEN_PARENTHESIS expression CLOSE_PARENTHESIS
		{ $$ = createActionNode(ACTION_SURCHARGE, $3, NULL); }
	| REJECT_ACTION OPEN_PARENTHESIS STRING_LITERAL CLOSE_PARENTHESIS
		{ $$ = createActionNode(ACTION_REJECT, NULL, $3); free($3); $3 = NULL; }
	;

export_stmt: EXPORT ID TO ID
	{ $$ = createExportNode($2, $4); free($2); free($4); $2 = NULL; $4 = NULL; }
	;

%%

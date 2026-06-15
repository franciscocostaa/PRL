%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus StringLiteralLexemeAction();
extern CompilationStatus TokenLexemeAction(TokenLabel label);
extern CompilationStatus UnknownLexemeAction();

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace          [[:space:]]+
id                 [[:alpha:]_][[:alnum:]_]*
integer            [[:digit:]]+
string_literal     \"([^\"\\]|\\.)*\"

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"//"[^\n]*                         { return IgnoredLexemeAction(); }
"/*"                               { BEGIN(MULTILINE_COMMENT); return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>"*/"            { BEGIN(INITIAL); return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>[^*\n]+          { return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>\n               { return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.                { return IgnoredLexemeAction(); }

">="                               { return TokenLexemeAction(GTE); }
"<="                               { return TokenLexemeAction(LTE); }
"=="                               { return TokenLexemeAction(EQ); }
"!="                               { return TokenLexemeAction(NEQ); }
">"                                { return TokenLexemeAction(GT); }
"<"                                { return TokenLexemeAction(LT); }

"campaign"                         { return TokenLexemeAction(CAMPAIGN); }
"entities"                         { return TokenLexemeAction(ENTITIES); }
"invariants"                       { return TokenLexemeAction(INVARIANTS); }
"rules"                            { return TokenLexemeAction(RULES); }
"rule"                             { return TokenLexemeAction(RULE); }
"priority"                         { return TokenLexemeAction(PRIORITY); }
"if"                               { return TokenLexemeAction(IF); }
"then"                             { return TokenLexemeAction(THEN); }
"assert"                           { return TokenLexemeAction(ASSERT); }
"export"                           { return TokenLexemeAction(EXPORT); }
"simulation"                       { return TokenLexemeAction(SIMULATION); }
"given"                            { return TokenLexemeAction(GIVEN); }
"expect"                           { return TokenLexemeAction(EXPECT); }
"unsatisfiable"                    { return TokenLexemeAction(UNSATISFIABLE); }
"satisfiable"                      { return TokenLexemeAction(SATISFIABLE); }
"to"                               { return TokenLexemeAction(TO); }
"on"                               { return TokenLexemeAction(ON); }
"and"                              { return TokenLexemeAction(AND); }
"or"                               { return TokenLexemeAction(OR); }
"not"                              { return TokenLexemeAction(NOT); }
"in"                               { return TokenLexemeAction(IN); }

"discount_fixed"                   { return TokenLexemeAction(DISCOUNT_FIXED); }
"discount"                         { return TokenLexemeAction(DISCOUNT); }
"surcharge"                        { return TokenLexemeAction(SURCHARGE); }
"reject"                           { return TokenLexemeAction(REJECT_ACTION); }

"{"                                { return TokenLexemeAction(OPEN_BRACE); }
"}"                                { return TokenLexemeAction(CLOSE_BRACE); }
"("                                { return TokenLexemeAction(OPEN_PARENTHESIS); }
")"                                { return TokenLexemeAction(CLOSE_PARENTHESIS); }
"["                                { return TokenLexemeAction(OPEN_BRACKET); }
"]"                                { return TokenLexemeAction(CLOSE_BRACKET); }
":"                                { return TokenLexemeAction(COLON); }
","                                { return TokenLexemeAction(COMMA); }
"."                                { return TokenLexemeAction(DOT); }
"%"                                { return TokenLexemeAction(PERCENT); }
"+"                                { return TokenLexemeAction(ADD); }
"-"                                { return TokenLexemeAction(SUB); }
"*"                                { return TokenLexemeAction(MUL); }
"/"                                { return TokenLexemeAction(DIV); }

{string_literal}                    { return StringLiteralLexemeAction(); }
{integer}                           { return IntegerLexemeAction(); }
{id}                                { return IdentifierLexemeAction(); }

{whitespace}                        { return IgnoredLexemeAction(); }
.                                   { return UnknownLexemeAction(); }

<<EOF>>                             { return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"

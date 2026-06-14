#include "Generator.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownGeneratorModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: Generator...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeGeneratorModule() {
	_logger = createLogger("Generator");
	return _shutdownGeneratorModule;
}

/* PRIVATE FUNCTIONS */

static const char * _relationalOperatorSymbol(RelOpKind kind) {
	switch (kind) {
		case REL_OP_EQ: return "==";
		case REL_OP_NEQ: return "!=";
		case REL_OP_GT: return ">";
		case REL_OP_LT: return "<";
		case REL_OP_GTE: return ">=";
		case REL_OP_LTE: return "<=";
		default: return "?";
	}
}

/**
 * Writes a string as the contents of a JSON string value, escaping the
 * characters that JSON requires (quotes, backslash and control characters). The
 * surrounding quotes are NOT emitted by this function.
 */
static void _emitJsonEscaped(FILE * output, const char * string) {
	if (string == NULL) {
		return;
	}
	for (const char * c = string; *c != '\0'; c++) {
		switch (*c) {
			case '"': fputs("\\\"", output); break;
			case '\\': fputs("\\\\", output); break;
			case '\n': fputs("\\n", output); break;
			case '\t': fputs("\\t", output); break;
			case '\r': fputs("\\r", output); break;
			default:
				if ((unsigned char) *c < 0x20) {
					fprintf(output, "\\u%04x", (unsigned char) *c);
				}
				else {
					fputc(*c, output);
				}
		}
	}
}

/**
 * Renders an expression inline as a JSON-safe fragment (used inside the "if"
 * string value). String literals are wrapped in escaped quotes.
 */
static void _emitExpressionInline(FILE * output, const ExpressionNode * expression) {
	if (expression == NULL) {
		return;
	}
	switch (expression->kind) {
		case EXPRESSION_FIELD_ACCESS:
			fprintf(output, "%s.%s", expression->fieldAccess->entityName, expression->fieldAccess->fieldName);
			break;
		case EXPRESSION_LITERAL:
			switch (expression->literal->kind) {
				case LITERAL_INTEGER:
					fprintf(output, "%d", expression->literal->integerValue);
					break;
				case LITERAL_PERCENTAGE:
					fprintf(output, "%d%%", expression->literal->percentageValue);
					break;
				case LITERAL_STRING:
					fputs("\\\"", output);
					_emitJsonEscaped(output, expression->literal->stringValue);
					fputs("\\\"", output);
					break;
			}
			break;
	}
}

/** Renders a condition inline as a JSON-safe fragment for the "if" field. */
static void _emitConditionInline(FILE * output, const ConditionNode * condition) {
	if (condition == NULL) {
		return;
	}
	switch (condition->kind) {
		case CONDITION_OR:
			_emitConditionInline(output, condition->left);
			fputs(" or ", output);
			_emitConditionInline(output, condition->right);
			break;
		case CONDITION_AND:
			_emitConditionInline(output, condition->left);
			fputs(" and ", output);
			_emitConditionInline(output, condition->right);
			break;
		case CONDITION_NOT:
			fputs("not ", output);
			_emitConditionInline(output, condition->operand);
			break;
		case CONDITION_COMPARISON:
			_emitExpressionInline(output, condition->leftExpression);
			fprintf(output, " %s ", _relationalOperatorSymbol(condition->relOp));
			_emitExpressionInline(output, condition->rightExpression);
			break;
		case CONDITION_IN:
			_emitExpressionInline(output, condition->inExpression);
			fputs(" in [", output);
			for (size_t i = 0; i < condition->optionCount; i++) {
				if (i > 0) {
					fputs(", ", output);
				}
				_emitExpressionInline(output, condition->options[i]);
			}
			fputc(']', output);
			break;
		case CONDITION_EXPRESSION:
			_emitExpressionInline(output, condition->expression);
			break;
	}
}

/** Emits the "then" action object of a rule. */
static void _emitActionJson(FILE * output, const ActionNode * action) {
	if (action == NULL) {
		fputs("null", output);
		return;
	}
	switch (action->kind) {
		case ACTION_DISCOUNT:
			fprintf(output, "{ \"action\": \"discount\", \"value\": \"%d%%\" }", action->value->literal->percentageValue);
			break;
		case ACTION_SURCHARGE:
			fprintf(output, "{ \"action\": \"surcharge\", \"value\": \"%d%%\" }", action->value->literal->percentageValue);
			break;
		case ACTION_DISCOUNT_FIXED:
			fprintf(output, "{ \"action\": \"discount_fixed\", \"value\": %d }", action->value->literal->integerValue);
			break;
		case ACTION_REJECT:
			fputs("{ \"action\": \"reject\", \"message\": \"", output);
			_emitJsonEscaped(output, action->message);
			fputs("\" }", output);
			break;
	}
}

static void _generateCampaignJson(FILE * output, const CampaignNode * campaign, const char * format) {
	fputs("  {\n", output);
	fputs("    \"campaign\": \"", output);
	_emitJsonEscaped(output, campaign->name);
	fputs("\",\n", output);
	fputs("    \"format\": \"", output);
	_emitJsonEscaped(output, format);
	fputs("\",\n", output);

	fputs("    \"entities\": [", output);
	for (size_t i = 0; i < campaign->entityCount; i++) {
		fputs(i == 0 ? " " : ", ", output);
		fputs("{ \"name\": \"", output);
		_emitJsonEscaped(output, campaign->entities[i]->name);
		fputs("\", \"type\": \"", output);
		_emitJsonEscaped(output, campaign->entities[i]->typeName);
		fputs("\" }", output);
	}
	fputs(campaign->entityCount == 0 ? "],\n" : " ],\n", output);

	fputs("    \"invariants\": [", output);
	for (size_t i = 0; i < campaign->invariantCount; i++) {
		fputs(i == 0 ? " \"" : ", \"", output);
		_emitConditionInline(output, campaign->invariants[i]->condition);
		fputc('"', output);
	}
	fputs(campaign->invariantCount == 0 ? "],\n" : " ],\n", output);

	fputs("    \"rules\": [", output);
	for (size_t i = 0; i < campaign->ruleCount; i++) {
		const RuleNode * rule = campaign->rules[i];
		fputs(i == 0 ? "\n" : ",\n", output);
		fputs("      { \"name\": \"", output);
		_emitJsonEscaped(output, rule->name);
		fprintf(output, "\", \"priority\": %d, \"if\": \"", rule->priority);
		_emitConditionInline(output, rule->condition);
		fputs("\", \"then\": ", output);
		_emitActionJson(output, rule->action);
		fputs(" }", output);
	}
	fputs(campaign->ruleCount == 0 ? "]\n" : "\n    ]\n", output);

	fputs("  }", output);
}

static const CampaignNode * _findCampaign(const ProgramNode * program, const char * name) {
	for (size_t i = 0; i < program->topLevelCount; i++) {
		const TopLevelNode * topLevel = program->topLevels[i];
		if (topLevel->kind == TOP_LEVEL_CAMPAIGN && strcmp(topLevel->campaign->name, name) == 0) {
			return topLevel->campaign;
		}
	}
	return NULL;
}

/* PUBLIC FUNCTIONS */

void executeGenerator(CompilerState * compilerState) {
	logDebugging(_logger, "Generating final output...");
	const ProgramNode * program = (const ProgramNode *) compilerState->abstractSyntaxtTree;
	fputs("[\n", stdout);
	bool first = true;
	if (program != NULL) {
		for (size_t i = 0; i < program->topLevelCount; i++) {
			const TopLevelNode * topLevel = program->topLevels[i];
			if (topLevel->kind != TOP_LEVEL_EXPORT) {
				continue;
			}
			const CampaignNode * campaign = _findCampaign(program, topLevel->exportNode->campaignName);
			if (campaign == NULL) {
				continue;
			}
			if (!first) {
				fputs(",\n", stdout);
			}
			first = false;
			_generateCampaignJson(stdout, campaign, topLevel->exportNode->format);
		}
	}
	fputs("\n]\n", stdout);
	fflush(stdout);
	logDebugging(_logger, "Generation is done.");
}

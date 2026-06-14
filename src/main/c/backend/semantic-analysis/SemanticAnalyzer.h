#ifndef SEMANTIC_ANALYZER_HEADER
#define SEMANTIC_ANALYZER_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"

/** Initialize module's internal state. */
ModuleDestructor initializeSemanticAnalyzerModule(CompilerState * compilerState);

/**
 * Runs the semantic-analysis phase over the AST stored in the compiler state.
 * Builds the symbol table, resolves references, and type-checks the program.
 * Returns SUCCEEDED if the program is semantically valid, FAILED otherwise.
 */
CompilationStatus executeSemanticAnalysis(CompilerState * compilerState);

#endif

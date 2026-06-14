#ifndef COMPILER_STATE_HEADER
#define COMPILER_STATE_HEADER

/**
 * The global state of the compiler. Should transport every data structure
 * needed across the different phases of a compilation.
 */
typedef struct {
	/**
	 * The root node of the AST.
	 */
	void * abstractSyntaxtTree;

	/**
	 * The symbol table (a stack of scopes) used by the semantic-analysis phase.
	 * It is created and owned by the semantic analyzer for the duration of that
	 * phase, and is NULL outside of it.
	 */
	void * symbolTable;
} CompilerState;

#endif

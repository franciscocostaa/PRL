#ifndef SYMBOL_TABLE_HEADER
#define SYMBOL_TABLE_HEADER

#include <stdbool.h>
#include <stddef.h>

/**
 * The compiler's symbol table, implemented as a stack of scopes as described in
 * the "Tabla de Símbolos" and "Scopes" specifications. Each scope holds the
 * symbols (campaigns, entities, rules) declared within a block; lookups walk the
 * stack from the top (innermost scope) down to the bottom (global scope), so
 * inner declarations shadow outer ones.
 */

typedef enum {
	SYMBOL_CAMPAIGN,
	SYMBOL_ENTITY,
	SYMBOL_RULE
} SymbolKind;

/**
 * A single entry in the symbol table.
 */
typedef struct {
	char * name;
	SymbolKind kind;
	/** Entity type name (e.g. "Client") for SYMBOL_ENTITY; NULL otherwise. */
	char * typeName;
} Symbol;

typedef struct SymbolTable SymbolTable;

/** Creates an empty symbol table (no scopes pushed yet). */
SymbolTable * symbolTableCreate(void);

/** Destroys the symbol table and every scope/symbol it owns. */
void symbolTableDestroy(SymbolTable * table);

/** Pushes a new (empty) scope onto the stack. */
void symbolTableEnterScope(SymbolTable * table);

/** Pops and destroys the top scope. No-op if the stack is empty. */
void symbolTableLeaveScope(SymbolTable * table);

/**
 * Declares a symbol in the current (top) scope. The name and typeName are
 * copied. Returns false (and declares nothing) if a symbol with the same name
 * already exists in the current scope.
 */
bool symbolTableDeclare(SymbolTable * table, const char * name, SymbolKind kind, const char * typeName);

/**
 * Looks up a symbol by name walking the scope stack top-down. Returns a pointer
 * to the matching symbol, or NULL if it is not visible from any active scope.
 */
Symbol * symbolTableLookup(const SymbolTable * table, const char * name);

/** Looks up a symbol by name in the current (top) scope only. */
Symbol * symbolTableLookupCurrent(const SymbolTable * table, const char * name);

#endif

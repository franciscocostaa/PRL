#include "SymbolTable.h"

#include <stdlib.h>
#include <string.h>

/* A scope is a growable array of symbols. */
typedef struct {
	Symbol * symbols;
	size_t count;
	size_t capacity;
} Scope;

/* The symbol table is a growable stack of scopes. */
struct SymbolTable {
	Scope * scopes;
	size_t count;
	size_t capacity;
};

/* PRIVATE FUNCTIONS */

/** Heap-copies a string (NULL-safe). */
static char * _copyString(const char * value) {
	if (value == NULL) {
		return NULL;
	}
	size_t length = strlen(value);
	char * copy = (char *) calloc(length + 1, sizeof(char));
	memcpy(copy, value, length);
	return copy;
}

/** Returns the current (top) scope, or NULL if the stack is empty. */
static Scope * _currentScope(const SymbolTable * table) {
	if (table == NULL || table->count == 0) {
		return NULL;
	}
	return &table->scopes[table->count - 1];
}

/** Finds a symbol by name within a single scope, or NULL. */
static Symbol * _findInScope(Scope * scope, const char * name) {
	if (scope == NULL || name == NULL) {
		return NULL;
	}
	for (size_t i = 0; i < scope->count; i++) {
		if (strcmp(scope->symbols[i].name, name) == 0) {
			return &scope->symbols[i];
		}
	}
	return NULL;
}

/* PUBLIC FUNCTIONS */

SymbolTable * symbolTableCreate(void) {
	return (SymbolTable *) calloc(1, sizeof(SymbolTable));
}

void symbolTableDestroy(SymbolTable * table) {
	if (table == NULL) {
		return;
	}
	while (table->count > 0) {
		symbolTableLeaveScope(table);
	}
	free(table->scopes);
	free(table);
}

void symbolTableEnterScope(SymbolTable * table) {
	if (table == NULL) {
		return;
	}
	if (table->count == table->capacity) {
		size_t newCapacity = table->capacity == 0 ? 4 : table->capacity * 2;
		table->scopes = (Scope *) realloc(table->scopes, newCapacity * sizeof(Scope));
		table->capacity = newCapacity;
	}
	Scope * scope = &table->scopes[table->count++];
	scope->symbols = NULL;
	scope->count = 0;
	scope->capacity = 0;
}

void symbolTableLeaveScope(SymbolTable * table) {
	Scope * scope = _currentScope(table);
	if (scope == NULL) {
		return;
	}
	for (size_t i = 0; i < scope->count; i++) {
		free(scope->symbols[i].name);
		free(scope->symbols[i].typeName);
	}
	free(scope->symbols);
	table->count--;
}

bool symbolTableDeclare(SymbolTable * table, const char * name, SymbolKind kind, const char * typeName) {
	Scope * scope = _currentScope(table);
	if (scope == NULL || name == NULL) {
		return false;
	}
	if (_findInScope(scope, name) != NULL) {
		return false;
	}
	if (scope->count == scope->capacity) {
		size_t newCapacity = scope->capacity == 0 ? 4 : scope->capacity * 2;
		scope->symbols = (Symbol *) realloc(scope->symbols, newCapacity * sizeof(Symbol));
		scope->capacity = newCapacity;
	}
	Symbol * symbol = &scope->symbols[scope->count++];
	symbol->name = _copyString(name);
	symbol->kind = kind;
	symbol->typeName = _copyString(typeName);
	return true;
}

Symbol * symbolTableLookup(const SymbolTable * table, const char * name) {
	if (table == NULL || name == NULL) {
		return NULL;
	}
	for (size_t i = table->count; i > 0; i--) {
		Symbol * found = _findInScope(&table->scopes[i - 1], name);
		if (found != NULL) {
			return found;
		}
	}
	return NULL;
}

Symbol * symbolTableLookupCurrent(const SymbolTable * table, const char * name) {
	return _findInScope(_currentScope(table), name);
}

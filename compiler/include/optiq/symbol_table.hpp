#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "optiq/ast.hpp"

namespace optiq {

// One symbol table entry, per the project manual: name, type, scope, and the
// line where the identifier was declared (for error reporting).
struct Symbol {
    std::string name;
    ValueType type;
    int scopeId;
    int lineDeclared;
};

// Nested-scope symbol table. Each block pushes a new scope; lookups walk from
// the innermost scope outward, so a variable declared inside an if/while block
// is not visible outside that block.
class SymbolTable {
public:
    void enterScope();
    void exitScope();

    // Declares a symbol in the current scope. Returns false (redeclaration)
    // when the name already exists in the current scope.
    bool declare(const std::string& name, ValueType type, int lineDeclared);

    // Finds a symbol visible from the current scope, or nullptr.
    const Symbol* lookup(const std::string& name) const;

    int currentScopeId() const { return nextScopeId - 1; }
    std::size_t scopeCount() const { return scopes.size(); }

private:
    using Scope = std::unordered_map<std::string, Symbol>;
    std::vector<Scope> scopes;
    std::vector<int> scopeIds;
    int nextScopeId = 0;
};

} // namespace optiq

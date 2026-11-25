#include "optiq/symbol_table.hpp"

namespace optiq {

void SymbolTable::enterScope() {
    scopes.emplace_back();
    scopeIds.push_back(nextScopeId++);
}

void SymbolTable::exitScope() {
    scopes.pop_back();
    scopeIds.pop_back();
}

bool SymbolTable::declare(const std::string& name, ValueType type, int lineDeclared) {
    Scope& scope = scopes.back();
    if (scope.find(name) != scope.end()) return false;
    scope.emplace(name, Symbol{name, type, scopeIds.back(), lineDeclared});
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

} // namespace optiq

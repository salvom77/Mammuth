#ifndef MAMMUTH_SCOPE_H
#define MAMMUTH_SCOPE_H

#include <unordered_map>
#include <string>
#include <memory>
#include "value.h"

// Forward declaration
struct ASTNode;

struct StoredVar {
    Value value;
    bool isDynamic = false;  // Per array dinamici
    bool isFixed = false;    // Per variabili fixed (immutabili)
};

class Scope {
public:
    std::unordered_map<std::string, StoredVar> vars;
    std::unordered_map<std::string, std::shared_ptr<ASTNode>> localFunctions;  // ← NUOVO!
    Scope* parent = nullptr;

    Scope(Scope* p = nullptr) : parent(p) {}

    bool existsLocal(const std::string& n) const {
        return vars.count(n) > 0;
    }

    StoredVar* lookup(const std::string& n) {
        auto it = vars.find(n);
        if (it != vars.end()) return &it->second;
        if (parent) return parent->lookup(n);
        return nullptr;
    }

    void define(const std::string& n, const StoredVar& v) {
        vars[n] = v;
    }

    void set(const std::string& n, const Value& val) {
        StoredVar* sv = lookup(n);
        if (!sv) return;
        sv->value = val;
    }
    
    // ============================================
    // NUOVO: Gestione funzioni locali (nested)
    // ============================================
    void defineLocalFunction(const std::string& name, std::shared_ptr<ASTNode> funcNode) {
        localFunctions[name] = funcNode;
    }
    
    std::shared_ptr<ASTNode> lookupLocalFunction(const std::string& name) {
        auto it = localFunctions.find(name);
        if (it != localFunctions.end()) return it->second;
        if (parent) return parent->lookupLocalFunction(name);
        return nullptr;
    }
};

#endif // MAMMUTH_SCOPE_H

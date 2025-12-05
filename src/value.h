#ifndef MAMMUTH_VALUE_H
#define MAMMUTH_VALUE_H

#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward
struct ASTNode;
class Scope;

// ------------------------------
// Forward di Value (wrapper)
// ------------------------------
struct Value; // definito dopo

// ------------------------------
// Array Mammuth
// ------------------------------
struct ArrayValue {
    std::vector<std::shared_ptr<Value>> elements;

    bool empty() const { return elements.empty(); }
    size_t size() const { return elements.size(); }

    void push_back(const std::shared_ptr<Value>& v) { elements.push_back(v); }

    const std::shared_ptr<Value>& operator[](size_t i) const { return elements[i]; }
    std::shared_ptr<Value>& operator[](size_t i) { return elements[i]; }
};

// ------------------------------
// Funzione utente
// ------------------------------
struct FunctionValue {
    std::vector<std::string> params;
    std::shared_ptr<ASTNode> body;
    Scope* closureScope = nullptr;  // Deprecato, uso capturedVars
    
    // Variabili catturate dalla closure (copia dei valori)
    std::unordered_map<std::string, Value> capturedVars;
    
    // Per composizione: f $ g
    std::vector<std::shared_ptr<FunctionValue>> composedFuncs;
    std::unordered_map<std::string, std::string> extra;  // Metadati
};

// ------------------------------
// Wrapper Value vero e proprio
// ------------------------------
struct Value {
    using Variant = std::variant<
        int,
        double,
        std::string,
        FunctionValue,
        ArrayValue
    >;

    Variant data;

    // costruttori comodi
    Value() : data(0) {}
    Value(int v) : data(v) {}
    Value(double v) : data(v) {}
    Value(const std::string& v) : data(v) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(const FunctionValue& f) : data(f) {}
    Value(const ArrayValue& a) : data(a) {}
    Value(const Variant& v) : data(v) {}
};

// ------------------------------
// Helper generici per Value
// ------------------------------
template<typename T>
inline bool isType(const Value& v) {
    return std::holds_alternative<T>(v.data);
}

template<typename T>
inline T& as(Value& v) {
    return std::get<T>(v.data);
}

template<typename T>
inline const T& as(const Value& v) {
    return std::get<T>(v.data);
}

#endif

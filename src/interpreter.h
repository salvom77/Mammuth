#ifndef MAMMUTH_INTERPRETER_H
#define MAMMUTH_INTERPRETER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "ast.h"
#include "value.h"
#include "scope.h"
#include "range.h"

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    Value eval(const std::shared_ptr<ASTNode>& node);

private:
    // Scopes
    std::vector<Scope*> scopes;
    Scope& currentScope();
    void pushScope();
    void popScope();

    // Variabili
    Value lookup(const std::string& name);
    void defineVar(const std::string& name, const Value& v, bool isDynamic = false, bool isFixed = false);
    void setVar(const std::string& name, const Value& v);

    // Semantica
    bool isTruthy(const Value& v) const;
    std::string toString(const Value& v) const;

    // ⭐ Range e slicing
    RangeInfo parseRangeNode(const std::shared_ptr<ASTNode>& node);
    Value sliceString(const std::string& s,
                      const RangeInfo& range,
                      const ASTNode* node);
    Value sliceArray(const ArrayValue& arr,
                     const RangeInfo& range,
                     const ASTNode* node);

    // Assignment
    Value evalAssignment(const std::shared_ptr<ASTNode>& node);

    // Operatori
    Value evalBinaryOp(const std::string& op,
                       Value left,
                       Value right,
                       const ASTNode* node);

    Value evalUnaryOp(const std::string& op,
                      const Value& val,
                      const ASTNode* node);

    // CondChain / Elvis / Filter
    Value evalCondChain(const std::shared_ptr<ASTNode>& node);
    Value evalElvis(const std::shared_ptr<ASTNode>& node);
    Value evalFilter(const std::shared_ptr<ASTNode>& node);

    // Funzioni utente
    Value callUserFunction(const std::shared_ptr<ASTNode>& funcNode,
                           const ArrayValue& args,
                           const ASTNode* callSite);

    // Utility
    void runtimeError(const ASTNode* node, const std::string& msg) const;
    void printValue(const Value& v) const;

    // Tabella funzioni
    std::unordered_map<std::string, std::shared_ptr<ASTNode>> functions;
};

#endif // MAMMUTH_INTERPRETER_H

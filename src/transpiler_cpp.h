#ifndef TRANSPILER_CPP_H
#define TRANSPILER_CPP_H

#include "ast.h"
#include <string>
#include <memory>
#include <sstream>
#include <array>

class CPPTranspiler {
public:
    // Entry Point
    std::string transpile(const std::shared_ptr<ASTNode>& ast);

    // Core Dispatcher
    std::string generateCode(const std::shared_ptr<ASTNode>& node);

private:
    std::unordered_map<std::string, std::string> varTypes;
    // Generators per tipo di nodo
    // Literals & Basic
    std::string generateLiteral(const std::shared_ptr<ASTNode>& node);
    std::string generateIdentifier(const std::shared_ptr<ASTNode>& node);

    // Declarations
    std::string generateVarDecl(const std::shared_ptr<ASTNode>& node);
    std::string generateFunctionDef(const std::shared_ptr<ASTNode>& node);
    std::string generateFunctionCall(const std::shared_ptr<ASTNode>& node);
    std::string generateSlice(const std::string& array, const std::shared_ptr<ASTNode>& rangeNode);

    // Expressions
    std::string generateBinaryOp(const std::shared_ptr<ASTNode>& node);
    std::string generateUnaryOp(const std::shared_ptr<ASTNode>& node);
    std::string generateIfExpression(const std::shared_ptr<ASTNode>& node);
    std::string generateIfStatement(const std::shared_ptr<ASTNode>& node);

    // Statements
    std::string generateEcho(const std::shared_ptr<ASTNode>& node);
    std::string generateAssignment(const std::shared_ptr<ASTNode>& node);

    // Control Flow
    std::string generateWhileLoop(const std::shared_ptr<ASTNode>& node);
    std::string generateForLoop(const std::shared_ptr<ASTNode>& node);

    // Advanced
    std::string generateCondChain(const std::shared_ptr<ASTNode>& node);
    std::string generateFilter(const std::shared_ptr<ASTNode>& node);
    std::string generateCommaList(const std::shared_ptr<ASTNode>& node);

    // Array
    std::string generateArrayInit(const std::shared_ptr<ASTNode>& node);
    std::string generateArrayDecl(const std::shared_ptr<ASTNode>& node);
    size_t countArraySize(const std::shared_ptr<ASTNode>& node);
    std::string generateArrayAccess(const std::shared_ptr<ASTNode>& node);

    // Utilities
    std::string indent(int level);
    std::string mapMammuthTypeToCpp(const std::string& mammuthType);
};

#endif

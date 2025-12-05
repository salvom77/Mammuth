#ifndef MAMMUTH_PARSER_H
#define MAMMUTH_PARSER_H

#include "lexer.h"
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>

#include "ast.h"

// Parser principale
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    std::shared_ptr<ASTNode> parseProgram();
    void printAST(const std::shared_ptr<ASTNode>& node, int indent = 0);

    std::shared_ptr<ASTNode> parseFunctionCall();
    std::shared_ptr<ASTNode> parsePrimary();
    std::shared_ptr<ASTNode> parseFunctionDef();

private:
    const std::vector<Token>& tokens;

    // Tipo elemento array (int, double, string, zero)
    std::unordered_map<std::string, std::string> arrayTypes;
    // Mutabilità degli array
    std::unordered_map<std::string, bool> arrayMutable;

    size_t pos = 0;

    // --- primitive di base ---
    const Token& peek() const;
    const Token& advance();
    bool match(TokenType type);
    bool check(TokenType type) const;

    // -------------------------------------
    // Line continuation intelligente
    // -------------------------------------
    bool isExpressionOpen() const;
    void skipContinuationNewlines();

    // --- parsing ---
    std::shared_ptr<ASTNode> parseStatement();
    std::shared_ptr<ASTNode> parseAssignment();
    std::shared_ptr<ASTNode> parseEcho();
    std::shared_ptr<ASTNode> parseIfExpr();  // v3.5: if/elif/else
    std::shared_ptr<ASTNode> parseFilter(std::shared_ptr<ASTNode> left);
    std::shared_ptr<ASTNode> parseElvis(std::shared_ptr<ASTNode> left = nullptr);
    std::shared_ptr<ASTNode> parseArrayDecl(bool isMutable);
    std::shared_ptr<ASTNode> parseArrayInitializer();
    std::shared_ptr<ASTNode> parseRange();

    // Espressione “alta”
    std::shared_ptr<ASTNode> parseExpression(int precedence = 0);

    // Espressione base
    std::shared_ptr<ASTNode> parseBaseExpression(int precedence = 0);

    // CondChain Mammuth
    std::shared_ptr<ASTNode> parseCondChain();
    std::shared_ptr<ASTNode> parseSimpleCond();

    std::shared_ptr<ASTNode> makeLiteral(const std::string& v);

    int getPrecedence(TokenType type);
    bool expectBlockStart();
};

#endif // MAMMUTH_PARSER_H

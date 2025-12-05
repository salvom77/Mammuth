#ifndef MAMMUTH_AST_H
#define MAMMUTH_AST_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "lexer.h"

struct ASTNode {
    std::string type;   // es. "Literal", "VarDecl", "BinaryOp", ...
    std::string value;  // lessico principale (es. nome variabile, operatore, ecc.)
    std::vector<std::shared_ptr<ASTNode>> children;
    std::unordered_map<std::string, std::string> extra;

    TokenType tokenType = TokenType::END_OF_FILE; // tipo token originario (opzionale)
    int line   = 0;
    int column = 0;
    bool condIncomplete = false;
};

#endif // MAMMUTH_AST_H

#include "parser.h"
#include "debug.h"

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens)
{
    arrayTypes.clear();
    DEBUG_PARSER_LOG("Parser creato, tokens=" << tokens.size());
}

/* ============================================================
   Primitive
   ============================================================ */

const Token& Parser::peek() const {
    static Token eof{TokenType::END_OF_FILE, "EOF", 0, 0};
    if (pos >= tokens.size())
        return eof;
    return tokens[pos];
}

const Token& Parser::advance() {
    if (pos < tokens.size())
        pos++;
    return peek();
}

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

/* ============================================================
   Line continuation: "espressione aperta"
   ============================================================ */

bool Parser::isExpressionOpen() const {
    if (pos == 0) return false;

    TokenType t = tokens[pos-1].type;

    switch (t) {
        case TokenType::PLUS:
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::MOD:
        case TokenType::CONCAT:
        case TokenType::AND:
        case TokenType::OR:
        case TokenType::BAND:
        case TokenType::BOR:
        case TokenType::BXOR:
        case TokenType::SHL:
        case TokenType::SHR:
        case TokenType::EQ:
        case TokenType::NEQ:
        case TokenType::GT:
        case TokenType::GE:
        case TokenType::LT:
        case TokenType::LE:
        case TokenType::ELVIS:
        case TokenType::FAT_ARROW:
        case TokenType::ASSIGN:
        case TokenType::LPAREN:
        case TokenType::LBRACKET:
        case TokenType::DOUBLE_QUESTION:  // Fix: Multi-line CondChain
        case TokenType::QUESTION:         // Fix: Multi-line SimpleCond
            return true;
        default:
            return false;
    }
}

void Parser::skipContinuationNewlines() {
    while (check(TokenType::NEWLINE) && isExpressionOpen())
        advance();
}

std::shared_ptr<ASTNode> Parser::makeLiteral(const std::string& v) {
    auto n = std::make_shared<ASTNode>();
    n->type = "Literal";
    n->value = v;
    return n;
}


/* ============================================================
   Program
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseProgram() {
    auto prog = std::make_shared<ASTNode>();
    prog->type = "Program";

    auto body = std::make_shared<ASTNode>();
    body->type = "Body";

    while (!check(TokenType::END_OF_FILE)) {

        while (check(TokenType::NEWLINE) && !isExpressionOpen())
            advance();

        if (check(TokenType::END_OF_FILE))
            break;

        auto stmt = parseStatement();

        if (stmt)
            body->children.push_back(stmt);
        else
            advance();

        while (check(TokenType::NEWLINE) && !isExpressionOpen())
            advance();
    }

    prog->children.push_back(body);
    return prog;
}

/* ============================================================
   STATEMENTS
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseStatement() {

    // ============================================
    // DEF: Named function o Lambda?
    // ============================================
    if (check(TokenType::KW_DEF)) {
        // Guarda il prossimo token dopo 'def'
        // Se è IDENT → named function
        // Se è LPAREN → lambda (expression statement)
        
        size_t saved = pos;
        advance(); // consuma 'def'
        
        if (check(TokenType::IDENT)) {
            // Named function: def funcName(...)
            pos = saved; // ripristina
            return parseFunctionDef();
        } else {
            // Lambda: def(...) → expression statement
            pos = saved; // ripristina
            auto expr = parseExpression();
            auto stmt = std::make_shared<ASTNode>();
            stmt->type = "ExprStmt";
            stmt->children.push_back(expr);
            return stmt;
        }
    }

    /* ======================================================
       ECHO
       ====================================================== */
    if (match(TokenType::KW_ECHO)) {
        auto node = std::make_shared<ASTNode>();
        node->type = "Echo";

        // echo senza parametri → stampa newline
        if (check(TokenType::NEWLINE) || check(TokenType::END_OF_FILE)) {
            node->children.push_back(makeLiteral(""));
            return node;
        }

        auto e = parseExpression();

        if (e && e->type == "CondChain" && e->condIncomplete) {
            std::cerr << "Errore: CondChain senza fallback non valida in echo\n";
        }

        node->children.push_back(e);
        return node;
    }


    /* ======================================================
       WHILE: while (cond) [-> var] stmt/block
       ====================================================== */
    if (match(TokenType::KW_WHILE)) {
        auto whileNode = std::make_shared<ASTNode>();
        whileNode->type = "While";
        
        if (!match(TokenType::LPAREN)) {
            std::cerr << "Errore while: atteso (\n";
            return nullptr;
        }
        
        auto cond = parseExpression();
        whileNode->children.push_back(cond);
        
        if (!match(TokenType::RPAREN)) {
            std::cerr << "Errore while: atteso )\n";
            return nullptr;
        }
        
        // Opzionale: -> variable
        if (match(TokenType::ARROW)) {
            if (!check(TokenType::IDENT)) {
                std::cerr << "Errore while: atteso nome variabile dopo ->\n";
                return nullptr;
            }
            whileNode->extra["returnVar"] = peek().lexeme;
            advance();
        }
        
        // Body: blocco :: ... end o statement inline
        if (match(TokenType::DOUBLE_COLON)) {
            auto body = std::make_shared<ASTNode>();
            body->type = "Body";
            
            skipContinuationNewlines();
            
            while (!check(TokenType::KW_END) && !check(TokenType::END_OF_FILE)) {
                if (check(TokenType::NEWLINE)) {
                    advance();
                    continue;
                }
                auto stmt = parseStatement();
                if (stmt) body->children.push_back(stmt);
            }
            
            if (!match(TokenType::KW_END)) {
                std::cerr << "Errore while: atteso 'end'\n";
            }
            
            whileNode->children.push_back(body);
        } else {
            // Statement inline
            auto stmt = parseStatement();
            whileNode->children.push_back(stmt);
        }
        
        return whileNode;
    }


    /* ======================================================
       FOR-IN: for var in collection [-> var] stmt/block
       ====================================================== */
    if (match(TokenType::KW_FOR)) {
        auto forNode = std::make_shared<ASTNode>();
        forNode->type = "ForIn";
        
        if (!check(TokenType::IDENT)) {
            std::cerr << "Errore for: atteso nome variabile\n";
            return nullptr;
        }
        
        std::string iterVar = peek().lexeme;
        forNode->value = iterVar;
        advance();
        
        if (!match(TokenType::KW_IN)) {
            std::cerr << "Errore for: atteso 'in'\n";
            return nullptr;
        }
        
        auto collection = parseExpression();
        forNode->children.push_back(collection);
        
        // Opzionale: -> variable
        if (match(TokenType::ARROW)) {
            if (!check(TokenType::IDENT)) {
                std::cerr << "Errore for: atteso nome variabile dopo ->\n";
                return nullptr;
            }
            forNode->extra["returnVar"] = peek().lexeme;
            advance();
        }
        
        // Body: blocco :: ... end o statement inline
        if (match(TokenType::DOUBLE_COLON)) {
            auto body = std::make_shared<ASTNode>();
            body->type = "Body";
            
            skipContinuationNewlines();
            
            while (!check(TokenType::KW_END) && !check(TokenType::END_OF_FILE)) {
                if (check(TokenType::NEWLINE)) {
                    advance();
                    continue;
                }
                auto stmt = parseStatement();
                if (stmt) body->children.push_back(stmt);
            }
            
            if (!match(TokenType::KW_END)) {
                std::cerr << "Errore for: atteso 'end'\n";
            }
            
            forNode->children.push_back(body);
        } else {
            // Statement inline
            auto stmt = parseStatement();
            forNode->children.push_back(stmt);
        }
        
        return forNode;
    }


    /* ======================================================
       ARRAY ASSIGN: arr[x] = expr
       ====================================================== */
    if (check(TokenType::IDENT)) {
        size_t saved = pos;

        if (pos + 1 < tokens.size() &&
            tokens[pos + 1].type == TokenType::LBRACKET)
        {
            auto target = parsePrimary();

            if (match(TokenType::ASSIGN)) {
                auto node = std::make_shared<ASTNode>();
                node->type = "ArrayAssign";

                skipContinuationNewlines();
                node->children.push_back(target);

                auto val = parseExpression();

                if (val && val->type == "CondChain" && val->condIncomplete) {
                    std::cerr << "Errore: CondChain senza fallback in assegnazione\n";
                }

                node->children.push_back(val);
                return node;
            }

            pos = saved;
        }
    }


    /* ======================================================
       ASSEGN. SEMPLICE: x = expr
       (messa PRIMA delle dichiarazioni)
       ====================================================== */
    {
        size_t saved = pos;
        auto a = parseAssignment();
        if (a) return a;
        pos = saved;
    }


    /* ======================================================
       DECLARAZIONI VARIABILI E ARRAY
       ====================================================== */
    bool isFixed = false;
    if (match(TokenType::KW_FIXED))
        isFixed = true;

    bool isDynamic = false;
    if (match(TokenType::KW_DYNAMIC)) {
        isDynamic = true;
        
        // Controlla contraddizione
        if (isFixed) {
            std::cerr << "Errore: 'fixed' e 'dynamic' sono mutuamente esclusivi\n";
            return nullptr;
        }
    }

    /* ======================================================
       TIPO FUNZIONE: <(type, type, ...)> name = lambda
       ====================================================== */
    if (match(TokenType::LT)) {
        if (!match(TokenType::LPAREN)) {
            std::cerr << "Errore: atteso '(' dopo '<' in tipo funzione\n";
            return nullptr;
        }

        // Parse param types
        std::vector<std::string> paramTypes;
        
        if (!check(TokenType::RPAREN)) {
            while (true) {
                if (match(TokenType::KW_INT)) {
                    paramTypes.push_back("int");
                } else if (match(TokenType::KW_DOUBLE)) {
                    paramTypes.push_back("double");
                } else if (match(TokenType::KW_STRING)) {
                    paramTypes.push_back("string");
                } else {
                    std::cerr << "Errore: tipo parametro non valido in signature funzione\n";
                    return nullptr;
                }
                
                if (!match(TokenType::COMMA))
                    break;
            }
        }

        if (!match(TokenType::RPAREN)) {
            std::cerr << "Errore: atteso ')' in tipo funzione\n";
            return nullptr;
        }

        if (!match(TokenType::GT)) {
            std::cerr << "Errore: atteso '>' dopo tipo funzione\n";
            return nullptr;
        }

        // Ora dovrebbe esserci il nome della variabile
        if (!match(TokenType::IDENT)) {
            std::cerr << "Errore: atteso nome variabile dopo tipo funzione\n";
            return nullptr;
        }

        std::string name = tokens[pos-1].lexeme;

        // Deve esserci =
        if (!match(TokenType::ASSIGN)) {
            std::cerr << "Errore: variabile funzione deve essere inizializzata\n";
            return nullptr;
        }

        // Parse espressione (dovrebbe essere lambda)
        auto expr = parseExpression();

        auto var = std::make_shared<ASTNode>();
        var->type = "VarDecl";
        var->value = name;
        var->extra["type"] = "function";
        var->extra["fixed"] = "true";  // SEMPRE immutabile!
        var->extra["isFunctionVar"] = "true";
        
        // Salva signature per type checking futuro
        std::string signature = "";
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (i > 0) signature += ",";
            signature += paramTypes[i];
        }
        var->extra["signature"] = signature;
        
        var->children.push_back(expr);
        return var;
    }

    if (match(TokenType::KW_INT) ||
        match(TokenType::KW_DOUBLE) ||
        match(TokenType::KW_STRING) ||
        match(TokenType::KW_ZERO))
    {
        TokenType typeToken = tokens[pos-1].type;
        std::string typeLex = tokens[pos-1].lexeme;

        if (!match(TokenType::IDENT)) {
            std::cerr << "Errore: atteso nome variabile\n";
            return nullptr;
        }

        std::string name = tokens[pos-1].lexeme;

        /* ===== ARRAY ===== */
        if (match(TokenType::LBRACKET)) {

            // array statico: int arr[10]
            if (match(TokenType::NUMBER_INT)) {
                int sizeVal = std::stoi(tokens[pos-1].lexeme);

                if (!match(TokenType::RBRACKET))
                    std::cerr << "Errore: atteso ']'\n";

                auto node = std::make_shared<ASTNode>();
                node->type = "ArrayDecl";
                node->value = name;
                node->extra["size"] = std::to_string(sizeVal);
                node->extra["dynamic"] = isDynamic ? "true" : "false";
                node->extra["fixed"] = isFixed ? "true" : "false";

                switch (typeToken) {
                    case TokenType::KW_INT: node->extra["type"]="int"; break;
                    case TokenType::KW_DOUBLE: node->extra["type"]="double"; break;
                    case TokenType::KW_STRING: node->extra["type"]="string"; break;
                    // zero NON è un tipo per variabili/array
                    default: node->extra["type"]="int";
                }

                arrayTypes[name] = node->extra["type"];
                arrayMutable[name] = isDynamic;
                return node;
            }

            // array dinamico / inizializzatore
            if (match(TokenType::RBRACKET)) {

                auto node = std::make_shared<ASTNode>();
                node->type = "ArrayDecl";
                node->value = name;
                node->extra["dynamic"] = isDynamic ? "true" : "false";
                node->extra["fixed"] = isFixed ? "true" : "false";

                switch (typeToken) {
                    case TokenType::KW_INT: node->extra["type"]="int"; break;
                    case TokenType::KW_DOUBLE: node->extra["type"]="double"; break;
                    case TokenType::KW_STRING: node->extra["type"]="string"; break;
                    // zero NON è un tipo per variabili/array
                    default: node->extra["type"]="int";
                }

                // immutabili richiedono inizializzatore
                if (!isDynamic && !check(TokenType::ASSIGN)) {
                    std::cerr << "Errore: array immutabile '" << name
                              << "' deve avere dimensione o inizializzatore\n";
                    return nullptr;
                }

                if (match(TokenType::ASSIGN)) {
                    skipContinuationNewlines();

                    TokenType tt = peek().type;

                    // caso array vuoto int arr[] =
                    if (tt == TokenType::NEWLINE ||
                        tt == TokenType::END_OF_FILE ||
                        (tt != TokenType::NUMBER_INT &&
                         tt != TokenType::NUMBER_DBL &&
                         tt != TokenType::STRING &&
                         tt != TokenType::IDENT &&
                         tt != TokenType::LPAREN &&
                         tt != TokenType::LBRACKET &&
                         tt != TokenType::MINUS))
                    {
                        auto empty = std::make_shared<ASTNode>();
                        empty->type = "ArrayInit";
                        node->children.push_back(empty);
                    }
                    else {
                        auto init = parseArrayInitializer();
                        node->children.push_back(init);
                    }
                }

                arrayTypes[name] = node->extra["type"];
                arrayMutable[name] = isDynamic;
                return node;
            }

            std::cerr << "Errore: array malformato\n";
            return nullptr;
        }

        /* ===== VAR semplice ===== */
        auto var = std::make_shared<ASTNode>();
        var->type      = "VarDecl";
        var->value     = name;
        var->extra["dynamic"] = isDynamic ? "true" : "false";
        var->extra["fixed"] = isFixed ? "true" : "false";

        switch (typeToken) {
            case TokenType::KW_INT: var->extra["type"]="int"; break;
            case TokenType::KW_DOUBLE: var->extra["type"]="double"; break;
            case TokenType::KW_STRING: var->extra["type"]="string"; break;
            // zero NON è un tipo per variabili
            default: var->extra["type"]="int";
        }

        if (match(TokenType::ASSIGN)) {
            skipContinuationNewlines();
            var->children.push_back(parseExpression());
        }

        return var;
    }

    /* ======================================================
       STATEMENT: espressione standalone
       ====================================================== */
    auto expr = parseExpression();

    auto stmt = std::make_shared<ASTNode>();
    stmt->type = "ExprStmt";
    stmt->children.push_back(expr);
    return stmt;
}



std::shared_ptr<ASTNode> Parser::parseAssignment() {
    // Se non c'è IDENT, non è un'assegnazione
    if (!check(TokenType::IDENT))
        return nullptr;

    size_t saved = pos;
    Token identTok = peek();          // salviamo il token per nome + posizione
    std::string name = identTok.lexeme;
    advance();                        // consumiamo l'identificatore

    // Se il token successivo NON è "=", non è un'assegnazione → rollback
    if (!match(TokenType::ASSIGN)) {
        pos = saved;
        return nullptr;
    }

    // c'è un "=", quindi è un assignment
    skipContinuationNewlines();
    auto rhs = parseExpression();
    if (!rhs) rhs = makeLiteral("0");

    // Costruiamo il nodo Identifier per il LHS
    auto lhs = std::make_shared<ASTNode>();
    lhs->type  = "Identifier";
    lhs->value = name;
    lhs->line   = identTok.line;
    lhs->column = identTok.column;

    // Nodo Assign con due figli: [0] = lhs, [1] = rhs
    auto node = std::make_shared<ASTNode>();
    node->type   = "Assign";
    node->value  = name;          // opzionale, ma può tornare utile
    node->line   = identTok.line;
    node->column = identTok.column;

    node->children.push_back(lhs);
    node->children.push_back(rhs);

    return node;
}




/* ============================================================
   Expression = CondChain → Elvis → Filter
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseExpression(int) {
    skipContinuationNewlines();
    auto expr = parseCondChain();
    expr = parseElvis(expr);
    expr = parseFilter(expr);
    // ★ Se la CondChain è incompleta, vieta l’uso dentro espressioni
    if (expr && expr->type == "CondChain" && expr->condIncomplete) {
        std::cerr << "Errore: CondChain senza fallback in contesto che richiede un valore\n";
    }

    return expr;
}

/* ============================================================
   CondChain
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseCondChain() {
    auto first = parseSimpleCond();
    if (!first) return nullptr;

    // FIX CRITICO: Skip TUTTI i newline (non condizionale!)
    // Dopo SimpleCond il token precedente è NUMBER, non in isExpressionOpen()
    while (check(TokenType::NEWLINE)) {
        advance();
    }

    if (!check(TokenType::DOUBLE_QUESTION) &&
        !check(TokenType::COLON))
        return first;

    auto chain = std::make_shared<ASTNode>();
    chain->type = "CondChain";
    chain->children.push_back(first);
    
    while (match(TokenType::DOUBLE_QUESTION)) {
        // Skip newline dopo ??
        while (check(TokenType::NEWLINE)) {
            advance();
        }
        chain->children.push_back(parseSimpleCond());
        // Skip newline dopo SimpleCond
        while (check(TokenType::NEWLINE)) {
            advance();
        }
    }
    
    if (match(TokenType::COLON)) {
        // Skip newline dopo :
        while (check(TokenType::NEWLINE)) {
            advance();
        }
        auto fallback = parseCondChain();
        chain->extra["hasFallback"] = "1";
        chain->children.push_back(fallback);
    }
    else {
        chain->extra["hasFallback"] = "0";
        chain->condIncomplete = true;
    }

    return chain;
}

/* ============================================================
   SimpleCond
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseSimpleCond() {
    auto cond = parseBaseExpression();
    if (!match(TokenType::QUESTION))
        return cond;

    skipContinuationNewlines();
    auto expr = parseBaseExpression();
    if (!expr) expr = makeLiteral("0");

    auto node = std::make_shared<ASTNode>();
    node->type = "SimpleCond";
    node->children.push_back(cond);
    node->children.push_back(expr);
    return node;
}

/* ============================================================
   BaseExpression
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseBaseExpression(int precedence) {
    skipContinuationNewlines();
    auto left = parsePrimary();
    if (!left) left = makeLiteral("0");

    while (true) {
        skipContinuationNewlines();

        TokenType t = peek().type;

        // ============================================
        // POSTFIX OPERATORS: Call e Array Access
        // Gestiti qui per supportare chiamate multiple
        // Es: func()(10) o arr[0][1]
        // ============================================
        
        // CALL: expr(args)
        if (t == TokenType::LPAREN) {
            advance(); // consuma (
            
            auto call = std::make_shared<ASTNode>();
            
            // Se left è Identifier, usa Call normale
            // Altrimenti usa CallExpr
            if (left->type == "Identifier") {
                call->type = "Call";
                call->value = left->value;
            } else {
                call->type = "CallExpr";
                call->children.push_back(left);
            }
            
            skipContinuationNewlines();
            
            // Parse argomenti
            while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
                call->children.push_back(parseBaseExpression(1));
                if (!match(TokenType::COMMA)) break;
                skipContinuationNewlines();
            }
            
            if (!match(TokenType::RPAREN))
                std::cerr << "Errore: ) mancante nella call\n";
            
            left = call;
            continue; // Check for more postfix operators
        }
        
        // ARRAY ACCESS: expr[index]
        if (t == TokenType::LBRACKET) {
            advance(); // consuma [
            
            size_t saved = pos;
            skipContinuationNewlines();
            auto rangeNode = parseRange();
            
            auto acc = std::make_shared<ASTNode>();
            acc->type = "ArrayAccess";
            
            if (rangeNode) {
                // Slice: arr[start..end]
                if (left->type == "Identifier") {
                    acc->value = left->value;
                    
                    auto& name = left->value;
                    if (arrayTypes.count(name))
                        acc->extra["elemType"] = arrayTypes[name];
                    if (arrayMutable.count(name))
                        acc->extra["dynamic"] = arrayMutable[name] ? "true" : "false";
                } else {
                    acc->value = "";
                    acc->children.push_back(left);
                }
                acc->children.push_back(rangeNode);
            } else {
                // Single index: arr[i]
                pos = saved;
                skipContinuationNewlines();
                auto idx = parseBaseExpression(1);  // Use precedence 1 come per call args
                
                if (!match(TokenType::RBRACKET)) {
                    std::cerr << "Errore: atteso ]\n";
                }
                
                if (left->type == "Identifier") {
                    acc->value = left->value;
                    
                    auto& name = left->value;
                    if (arrayTypes.count(name))
                        acc->extra["elemType"] = arrayTypes[name];
                    if (arrayMutable.count(name))
                        acc->extra["dynamic"] = arrayMutable[name] ? "true" : "false";
                } else {
                    acc->value = "";
                    acc->children.push_back(left);
                }
                acc->children.push_back(idx);
            }
            
            left = acc;
            continue; // Check for more postfix operators
        }

        if (t == TokenType::QUESTION ||
            t == TokenType::DOUBLE_QUESTION ||
            t == TokenType::COLON ||
            t == TokenType::FAT_ARROW)
            break;

        int prec = getPrecedence(t);
        if (prec < 0 || prec < precedence)
            break;

        std::string op = peek().lexeme;
        advance();

        skipContinuationNewlines();

        // ============================================
        // SLICE SHORTHAND: var $[...] → var $ var[...]
        // ============================================
        if (op == "$" && check(TokenType::LBRACKET)) {
            advance(); // consuma [
            
            // Parse slice o single index
            std::shared_ptr<ASTNode> indexOrSlice = nullptr;
            
            if (match(TokenType::COLON)) {
                // [:end] o [:]
                auto slice = std::make_shared<ASTNode>();
                slice->type = "Slice";
                slice->extra["start"] = "";  // Empty = from beginning
                
                if (!check(TokenType::RBRACKET)) {
                    auto end = parseBaseExpression();
                    slice->children.push_back(end);
                } else {
                    slice->extra["end"] = "";  // Empty = to end
                }
                indexOrSlice = slice;
                
            } else {
                auto first = parseBaseExpression();
                
                if (match(TokenType::DOUBLE_COLON)) {
                    // [start..]
                    auto slice = std::make_shared<ASTNode>();
                    slice->type = "Slice";
                    slice->children.push_back(first);
                    slice->extra["end"] = "";  // To end
                    indexOrSlice = slice;
                    
                } else if (match(TokenType::COLON)) {
                    // [start:end]
                    auto slice = std::make_shared<ASTNode>();
                    slice->type = "Slice";
                    slice->children.push_back(first);
                    
                    if (!check(TokenType::RBRACKET)) {
                        auto end = parseBaseExpression();
                        slice->children.push_back(end);
                    } else {
                        slice->extra["end"] = "";
                    }
                    indexOrSlice = slice;
                    
                } else {
                    // Single index [i]
                    indexOrSlice = first;
                }
            }
            
            if (!match(TokenType::RBRACKET)) {
                std::cerr << "Errore: atteso ] in slice shorthand\n";
                return nullptr;
            }
            
            // Crea: left $ left[...]
            auto access = std::make_shared<ASTNode>();
            access->type = "ArrayAccess";
            access->children.push_back(left);  // array
            access->children.push_back(indexOrSlice);  // index/slice
            
            auto concat = std::make_shared<ASTNode>();
            concat->type = "BinaryOp";
            concat->value = "$";
            concat->children.push_back(left);
            concat->children.push_back(access);
            
            left = concat;
            continue;
        }

        int nextPrec = prec + ((op == "**") ? 0 : 1);

        auto right = parseBaseExpression(nextPrec);
        if (!right) right = makeLiteral("0");

        auto node = std::make_shared<ASTNode>();
        node->type = (op == "and" || op == "or") ? "LogicalOp" : "BinaryOp";

        if (op == ",") {
            auto list = std::make_shared<ASTNode>();
            list->type = "CommaList";

            if (left->type == "CommaList")
                list->children = left->children;
            else
                list->children.push_back(left);

            list->children.push_back(right);
            left = list;
            continue;
        }

        node->value = op;
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }

    return left;
}

/* ============================================================
   parsePrimary
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parsePrimary() {
    skipContinuationNewlines();
    const auto& tok = peek();

    // ============================================
    // IF EXPRESSION: if cond:: body elif cond:: body else:: body end
    // ============================================
    if (tok.type == TokenType::KW_IF) {
        return parseIfExpr();
    }

    // ============================================
    // LAMBDA ANONIMA: def(params) -> type expr/block
    // ============================================
    if (tok.type == TokenType::KW_DEF) {
        advance(); // consuma 'def'
        
        if (!match(TokenType::LPAREN)) {
            std::cerr << "Errore lambda: atteso (\n";
            return nullptr;
        }
        
        // Parsing parametri
        std::vector<std::pair<std::string,std::string>> params;
        
        if (!check(TokenType::RPAREN)) {
            while (true) {
                if (!check(TokenType::IDENT)) {
                    std::cerr << "Errore lambda: atteso parametro\n";
                    return nullptr;
                }
                
                std::string pname = peek().lexeme;
                advance();
                
                if (!match(TokenType::COLON)) {
                    std::cerr << "Errore lambda: atteso :\n";
                    return nullptr;
                }
                
                std::string ptype;
                
                // Tipo funzione: <(type, type, ...)>
                if (check(TokenType::LT)) {
                    advance(); // <
                    
                    if (!match(TokenType::LPAREN)) {
                        std::cerr << "Errore lambda: atteso '(' in tipo funzione\n";
                        return nullptr;
                    }
                    
                    std::vector<std::string> funcParamTypes;
                    if (!check(TokenType::RPAREN)) {
                        while (true) {
                            if (match(TokenType::KW_INT)) {
                                funcParamTypes.push_back("int");
                            } else if (match(TokenType::KW_DOUBLE)) {
                                funcParamTypes.push_back("double");
                            } else if (match(TokenType::KW_STRING)) {
                                funcParamTypes.push_back("string");
                            } else {
                                std::cerr << "Errore lambda: tipo non valido\n";
                                return nullptr;
                            }
                            
                            if (!match(TokenType::COMMA))
                                break;
                        }
                    }
                    
                    if (!match(TokenType::RPAREN)) {
                        std::cerr << "Errore lambda: atteso ')'\n";
                        return nullptr;
                    }
                    
                    if (!match(TokenType::GT)) {
                        std::cerr << "Errore lambda: atteso '>'\n";
                        return nullptr;
                    }
                    
                    ptype = "<(";
                    for (size_t i = 0; i < funcParamTypes.size(); ++i) {
                        if (i > 0) ptype += ",";
                        ptype += funcParamTypes[i];
                    }
                    ptype += ")>";
                    
                } else if (!(check(TokenType::KW_INT) ||
                      check(TokenType::KW_DOUBLE) ||
                      check(TokenType::KW_STRING)))
                {
                    std::cerr << "Errore lambda: atteso tipo parametro\n";
                    return nullptr;
                } else {
                    ptype = peek().lexeme;
                    advance();
                }
                
                params.emplace_back(ptype, pname);
                
                if (!match(TokenType::COMMA))
                    break;
            }
        }
        
        if (!match(TokenType::RPAREN)) {
            std::cerr << "Errore lambda: atteso )\n";
            return nullptr;
        }
        
        if (!match(TokenType::ARROW)) {
            std::cerr << "Errore lambda: atteso ->\n";
            return nullptr;
        }
        
        // Tipo di ritorno
        std::string retType;
        if (!(check(TokenType::KW_INT) ||
              check(TokenType::KW_DOUBLE) ||
              check(TokenType::KW_STRING) ||
              check(TokenType::KW_ZERO)))
        {
            std::cerr << "Errore lambda: atteso tipo di ritorno\n";
            return nullptr;
        }
        
        retType = peek().lexeme;
        advance();
        
        auto lambda = std::make_shared<ASTNode>();
        lambda->type = "Lambda";
        lambda->value = "<anonymous>";
        lambda->extra["returnType"] = retType;
        
        for (auto& p : params) {
            auto pn = std::make_shared<ASTNode>();
            pn->type = "Param";
            pn->value = p.second;
            pn->extra["paramType"] = p.first;
            lambda->children.push_back(pn);
        }
        
        // Body: espressione singola o blocco ::
        if (match(TokenType::DOUBLE_COLON)) {
            // Blocco: def(...) -> tipo:: ... end
            auto body = std::make_shared<ASTNode>();
            body->type = "Body";
            
            skipContinuationNewlines();
            
            while (!check(TokenType::KW_END) && !check(TokenType::END_OF_FILE)) {
                if (check(TokenType::NEWLINE)) {
                    advance();
                    continue;
                }
                auto stmt = parseStatement();
                if (stmt) body->children.push_back(stmt);
            }
            
            if (!match(TokenType::KW_END)) {
                std::cerr << "Errore lambda: atteso 'end'\n";
            }
            
            lambda->children.push_back(body);
        } else {
            // Espressione singola: def(...) -> tipo expr
            auto expr = parseExpression();
            auto body = std::make_shared<ASTNode>();
            body->type = "Body";
            
            auto exprStmt = std::make_shared<ASTNode>();
            exprStmt->type = "ExprStmt";
            exprStmt->children.push_back(expr);
            body->children.push_back(exprStmt);
            
            lambda->children.push_back(body);
        }
        
        return lambda;
    }

    // Unari
    if (tok.type == TokenType::MINUS ||
        tok.type == TokenType::BNOT ||
        tok.type == TokenType::NOT)
    {
        std::string op = tok.lexeme;
        advance();
        skipContinuationNewlines();
        auto expr = parsePrimary();

        auto u = std::make_shared<ASTNode>();
        u->type = "UnaryOp";
        u->value = op;
        u->children.push_back(expr);
        return u;
    }

    // Identificatore
    if (tok.type == TokenType::IDENT) {
        std::string name = tok.lexeme;
        advance();

        auto id = std::make_shared<ASTNode>();
        id->type = "Identifier";
        id->value = name;

        // NOTE: Call e array access gestiti in parseBaseExpression
        // come operatori postfix, così supportano chiamate multiple
        // Es: func()(10) o arr[0][1]

        return id;
    }

    // Range standalone
    if (tok.type == TokenType::LBRACKET) {
        advance();
        skipContinuationNewlines();
        auto r = parseRange();
        if (!r) {
            std::cerr << "Errore: atteso range dopo [\n";
            return makeLiteral("0");
        }
        return r;
    }

    // Literal
    if (tok.type == TokenType::NUMBER_INT ||
        tok.type == TokenType::NUMBER_DBL ||
        tok.type == TokenType::STRING) {
        auto lit = std::make_shared<ASTNode>();
        lit->type = "Literal";
        lit->value = tok.lexeme;
        lit->tokenType = tok.type;
        advance();
        return lit;
    }

    // (expr)
    if (tok.type == TokenType::LPAREN) {
        advance();
        skipContinuationNewlines();
        auto expr = parseExpression();
        skipContinuationNewlines();
        if (!match(TokenType::RPAREN))
            std::cerr << "Errore: ) mancante\n";
        
        // ============================================
        // Chiamata su espressione: (expr)(args)
        // Es: (doubler $ addFive)(10)
        // ============================================
        if (check(TokenType::LPAREN)) {
            advance(); // consuma (
            
            auto call = std::make_shared<ASTNode>();
            call->type = "CallExpr";  // Nuovo tipo per distinguere da Call normale
            call->children.push_back(expr);  // Espressione da chiamare
            
            skipContinuationNewlines();
            
            // Parse argomenti
            while (!check(TokenType::RPAREN) && !check(TokenType::END_OF_FILE)) {
                call->children.push_back(parseBaseExpression(1));
                if (!match(TokenType::COMMA)) break;
                skipContinuationNewlines();
            }
            
            if (!match(TokenType::RPAREN))
                std::cerr << "Errore: ) mancante nella chiamata su espressione\n";
            
            return call;
        }
        
        return expr;
    }

    if (tok.type == TokenType::NEWLINE) {
        advance();
        skipContinuationNewlines();
        return makeLiteral("0");
    }

    if (tok.type == TokenType::END_OF_FILE)
        return nullptr;

    // Fix: ASSIGN è gestito in parseStatement (dichiarazioni variabili funzione)
    // Non è un errore, semplicemente non va parsato qui
    if (tok.type == TokenType::ASSIGN) {
        return nullptr;
    }

    std::cerr << "Token inatteso in parsePrimary: " << tok.lexeme << "\n";
    advance();
    return makeLiteral("0");
}

/* ============================================================
   Elvis
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseElvis(std::shared_ptr<ASTNode> left) {
    if (!left) return nullptr;

    while (match(TokenType::ELVIS)) {
        skipContinuationNewlines();
        auto right = parseCondChain();
        if (!right) right = makeLiteral("0");

        auto node = std::make_shared<ASTNode>();
        node->type = "Elvis";
        node->children.push_back(left);
        node->children.push_back(right);
        left = node;
    }

    return left;
}

/* ============================================================
   Filter
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseFilter(std::shared_ptr<ASTNode> left) {
    if (!left) return nullptr;

    while (match(TokenType::FAT_ARROW)) {
        skipContinuationNewlines();
        auto cond = parseCondChain();
        if (!cond) cond = makeLiteral("0");

        auto node = std::make_shared<ASTNode>();
        node->type = "Filter";
        node->children.push_back(left);
        node->children.push_back(cond);
        left = node;
    }

    return left;
}

/* ============================================================
   Array initializer
   ============================================================ */
std::shared_ptr<ASTNode> Parser::parseArrayInitializer() {
    auto list = std::make_shared<ASTNode>();
    list->type = "ArrayInit";
    list->children.push_back(parseExpression());  // ✅ FIX: parseExpression invece di parseBaseExpression
    while (match(TokenType::COMMA)) {
        skipContinuationNewlines();
        list->children.push_back(parseExpression());  // ✅ FIX: parseExpression invece di parseBaseExpression
    }
    return list;
}

/* ============================================================
   Range parsing
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseRange() {
    size_t startPos = pos;

    // [..]
    if (match(TokenType::RANGE)) {

        auto node = std::make_shared<ASTNode>();
        node->type = "RangeExpr";
        node->extra["hasStart"] = "false";

        skipContinuationNewlines();

        if (check(TokenType::RBRACKET)) {
            advance();
            node->extra["hasEnd"] = "false";
            return node;
        }

        auto end = parseExpression();
        node->children.push_back(end);

        if (!match(TokenType::RBRACKET))
            std::cerr << "Errore: atteso ]\n";

        node->extra["hasEnd"] = "true";
        return node;
    }

    // [a..]
    size_t saved = pos;
    auto startExpr = parseExpression();

    if (match(TokenType::RANGE)) {
        auto node = std::make_shared<ASTNode>();
        node->type = "RangeExpr";
        node->children.push_back(startExpr);
        node->extra["hasStart"] = "true";

        skipContinuationNewlines();

        if (check(TokenType::RBRACKET)) {
            advance();
            node->extra["hasEnd"] = "false";
            return node;
        }

        auto end = parseExpression();
        node->children.push_back(end);
        node->extra["hasEnd"] = "true";

        if (!match(TokenType::RBRACKET))
            std::cerr << "Errore: atteso ]\n";

        return node;
    }

    pos = startPos;
    return nullptr;
}

/* ============================================================
   Precedenze operatori
   ============================================================ */

int Parser::getPrecedence(TokenType type) {
    switch (type) {
        case TokenType::FAT_ARROW: return 14;
        case TokenType::POW: return 13;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::MOD: return 12;
        case TokenType::PLUS:
        case TokenType::MINUS: return 11;
        case TokenType::CONCAT: return 10;
        case TokenType::SHL:
        case TokenType::SHR: return 9;
        case TokenType::LT:
        case TokenType::LE:
        case TokenType::GT:
        case TokenType::GE: return 8;
        case TokenType::EQ:
        case TokenType::NEQ: return 7;
        case TokenType::BAND: return 6;
        case TokenType::BXOR: return 5;
        case TokenType::BOR: return 4;
        case TokenType::AND: return 3;
        case TokenType::OR: return 2;
        case TokenType::ELVIS: return 1;
        case TokenType::COMMA: return 0;
        default: return -1;
    }
}

/* ============================================================
   If/Elif/Else Expression (v3.5)
   Sintassi:
     if condition:: body [elif condition:: body]* [else:: body] end
     if condition:: expr else:: expr (inline, no end)
   ============================================================ */

std::shared_ptr<ASTNode> Parser::parseIfExpr() {
    if (!match(TokenType::KW_IF)) {
        std::cerr << "Errore: atteso 'if'\n";
        return nullptr;
    }
    
    auto ifNode = std::make_shared<ASTNode>();
    ifNode->type = "IfExpr";
    
    // Parse condition
    skipContinuationNewlines();
    auto condition = parseExpression();
    if (!condition) {
        std::cerr << "Errore: attesa condizione dopo 'if'\n";
        return nullptr;
    }
    
    if (!match(TokenType::DOUBLE_COLON)) {
        std::cerr << "Errore: atteso '::' dopo condizione if\n";
        return nullptr;
    }
    
    // Check se è multi-line (newline dopo ::) o inline (expr diretto)
    // NON skippiamo newline prima del check!
    bool isMultiline = check(TokenType::NEWLINE);
    if (isMultiline) {
        advance(); // consuma newline
        skipContinuationNewlines();
    }
    
    // Parse then body
    auto thenBody = std::make_shared<ASTNode>();
    thenBody->type = "Body";
    
    if (isMultiline) {
        // Multi-line: parse statements fino a elif/else/end
        while (!check(TokenType::KW_ELIF) && 
               !check(TokenType::KW_ELSE) && 
               !check(TokenType::KW_END) && 
               !check(TokenType::END_OF_FILE)) {
            // Skip ALL newlines in multiline body (not just continuation)
            while (check(TokenType::NEWLINE)) advance();
            
            if (check(TokenType::KW_ELIF) || check(TokenType::KW_ELSE) || check(TokenType::KW_END))
                break;
            auto stmt = parseStatement();
            if (stmt) thenBody->children.push_back(stmt);
        }
    } else {
        // Inline: single expression
        auto expr = parseExpression();
        if (!expr) {
            std::cerr << "Errore: attesa espressione in then branch\n";
            return nullptr;
        }
        auto exprStmt = std::make_shared<ASTNode>();
        exprStmt->type = "ExprStmt";
        exprStmt->children.push_back(expr);
        thenBody->children.push_back(exprStmt);
    }
    
    // Add condition and body to ifNode
    ifNode->children.push_back(condition);
    ifNode->children.push_back(thenBody);
    
    int elifCount = 0;
    
    // Parse elif branches
    while (check(TokenType::KW_ELIF)) {
        advance(); // consuma 'elif'
        skipContinuationNewlines();
        
        auto elifCondition = parseExpression();
        if (!elifCondition) {
            std::cerr << "Errore: attesa condizione dopo 'elif'\n";
            return nullptr;
        }
        
        if (!match(TokenType::DOUBLE_COLON)) {
            std::cerr << "Errore: atteso '::' dopo condizione elif\n";
            return nullptr;
        }
        
        // Check multiline senza skippare newline prima!
        bool elifMultiline = check(TokenType::NEWLINE);
        if (elifMultiline) {
            advance();
            skipContinuationNewlines();
        }
        
        auto elifBody = std::make_shared<ASTNode>();
        elifBody->type = "Body";
        
        if (elifMultiline) {
            while (!check(TokenType::KW_ELIF) && 
                   !check(TokenType::KW_ELSE) && 
                   !check(TokenType::KW_END) && 
                   !check(TokenType::END_OF_FILE)) {
                // Skip ALL newlines in multiline body (not just continuation)
                while (check(TokenType::NEWLINE)) advance();
                
                if (check(TokenType::KW_ELIF) || check(TokenType::KW_ELSE) || check(TokenType::KW_END))
                    break;
                auto stmt = parseStatement();
                if (stmt) elifBody->children.push_back(stmt);
            }
        } else {
            auto expr = parseExpression();
            if (!expr) {
                std::cerr << "Errore: attesa espressione in elif branch\n";
                return nullptr;
            }
            auto exprStmt = std::make_shared<ASTNode>();
            exprStmt->type = "ExprStmt";
            exprStmt->children.push_back(expr);
            elifBody->children.push_back(exprStmt);
        }
        
        ifNode->children.push_back(elifCondition);
        ifNode->children.push_back(elifBody);
        elifCount++;
    }
    
    // Parse optional else branch
    bool hasElse = false;
    if (check(TokenType::KW_ELSE)) {
        advance(); // consuma 'else'
        skipContinuationNewlines();
        
        if (!match(TokenType::DOUBLE_COLON)) {
            std::cerr << "Errore: atteso '::' dopo 'else'\n";
            return nullptr;
        }
        
        // Check multiline senza skippare newline prima!
        bool elseMultiline = check(TokenType::NEWLINE);
        if (elseMultiline) {
            advance();
            skipContinuationNewlines();
        }
        
        auto elseBody = std::make_shared<ASTNode>();
        elseBody->type = "Body";
        
        if (elseMultiline) {
            while (!check(TokenType::KW_END) && !check(TokenType::END_OF_FILE)) {
                // Skip ALL newlines in multiline body (not just continuation)
                while (check(TokenType::NEWLINE)) advance();
                
                if (check(TokenType::KW_END))
                    break;
                auto stmt = parseStatement();
                if (stmt) elseBody->children.push_back(stmt);
            }
        } else {
            auto expr = parseExpression();
            if (!expr) {
                std::cerr << "Errore: attesa espressione in else branch\n";
                return nullptr;
            }
            auto exprStmt = std::make_shared<ASTNode>();
            exprStmt->type = "ExprStmt";
            exprStmt->children.push_back(expr);
            elseBody->children.push_back(exprStmt);
        }
        
        ifNode->children.push_back(elseBody);
        hasElse = true;
    }
    
    // Consume 'end' if multi-line
    if (isMultiline) {
        if (!match(TokenType::KW_END)) {
            std::cerr << "Errore: atteso 'end' per chiudere if multi-line\n";
            return nullptr;
        }
    }
    
    ifNode->extra["elifCount"] = std::to_string(elifCount);
    ifNode->extra["hasElse"] = hasElse ? "true" : "false";
    ifNode->extra["multiline"] = isMultiline ? "true" : "false";
    
    return ifNode;
}

/* ============================================================
   Function definition
   ============================================================ */

bool Parser::expectBlockStart() {
    if (match(TokenType::DOUBLE_COLON)) return true;
    std::cerr << "Errore: atteso '::'\n";
    return false;
}

std::shared_ptr<ASTNode> Parser::parseFunctionDef() {
    match(TokenType::KW_DEF);

    if (!check(TokenType::IDENT)) {
        std::cerr << "Errore: atteso nome funzione\n";
        return nullptr;
    }

    std::string fname = peek().lexeme;
    advance();

    if (!match(TokenType::LPAREN)) {
        std::cerr << "Errore: atteso (\n"; return nullptr;
    }

    std::vector<std::pair<std::string,std::string>> params;

    if (!check(TokenType::RPAREN)) {
        while (true) {

            if (!check(TokenType::IDENT)) {
                std::cerr << "Errore: atteso parametro\n";
                return nullptr;
            }

            std::string pname = peek().lexeme;
            advance();

            if (!match(TokenType::COLON)) {
                std::cerr << "Errore: atteso :\n";
                return nullptr;
            }

            std::string ptype;
            
            // Tipo funzione: <(type, type, ...)>
            if (check(TokenType::LT)) {
                advance(); // <
                
                if (!match(TokenType::LPAREN)) {
                    std::cerr << "Errore: atteso '(' in tipo funzione parametro\n";
                    return nullptr;
                }
                
                std::vector<std::string> funcParamTypes;
                if (!check(TokenType::RPAREN)) {
                    while (true) {
                        if (match(TokenType::KW_INT)) {
                            funcParamTypes.push_back("int");
                        } else if (match(TokenType::KW_DOUBLE)) {
                            funcParamTypes.push_back("double");
                        } else if (match(TokenType::KW_STRING)) {
                            funcParamTypes.push_back("string");
                        } else {
                            std::cerr << "Errore: tipo non valido in signature\n";
                            return nullptr;
                        }
                        
                        if (!match(TokenType::COMMA))
                            break;
                    }
                }
                
                if (!match(TokenType::RPAREN)) {
                    std::cerr << "Errore: atteso ')' in tipo funzione\n";
                    return nullptr;
                }
                
                if (!match(TokenType::GT)) {
                    std::cerr << "Errore: atteso '>'\n";
                    return nullptr;
                }
                
                // Costruisci stringa tipo
                ptype = "<(";
                for (size_t i = 0; i < funcParamTypes.size(); ++i) {
                    if (i > 0) ptype += ",";
                    ptype += funcParamTypes[i];
                }
                ptype += ")>";
                
            } else if (!(check(TokenType::KW_INT) ||
                  check(TokenType::KW_DOUBLE) ||
                  check(TokenType::KW_STRING)))
            {
                std::cerr << "Errore: atteso tipo parametro (int, double, string, <(...)>)\n";
                return nullptr;
            } else {
                ptype = peek().lexeme;
                advance();
            }

            params.emplace_back(ptype, pname);

            if (!match(TokenType::COMMA))
                break;
        }
    }

    if (!match(TokenType::RPAREN)) {
        std::cerr << "Errore: atteso )\n";
        return nullptr;
    }

    if (!match(TokenType::ARROW)) {
        std::cerr << "Errore: atteso ->\n";
        return nullptr;
    }

    std::string retType;
    
    // ============================================
    // FUNCTION RETURN TYPE: -> <(params)>
    // ============================================
    if (check(TokenType::LT)) {
        advance(); // <
        
        if (!match(TokenType::LPAREN)) {
            std::cerr << "Errore: atteso '(' in tipo funzione return\n";
            return nullptr;
        }
        
        std::vector<std::string> funcRetTypes;
        if (!check(TokenType::RPAREN)) {
            while (true) {
                if (match(TokenType::KW_INT)) {
                    funcRetTypes.push_back("int");
                } else if (match(TokenType::KW_DOUBLE)) {
                    funcRetTypes.push_back("double");
                } else if (match(TokenType::KW_STRING)) {
                    funcRetTypes.push_back("string");
                } else {
                    std::cerr << "Errore: tipo non valido in function return type\n";
                    return nullptr;
                }
                
                if (!match(TokenType::COMMA))
                    break;
            }
        }
        
        if (!match(TokenType::RPAREN)) {
            std::cerr << "Errore: atteso ')' in tipo funzione return\n";
            return nullptr;
        }
        
        if (!match(TokenType::GT)) {
            std::cerr << "Errore: atteso '>' in tipo funzione return\n";
            return nullptr;
        }
        
        // Costruisci stringa tipo
        retType = "<(";
        for (size_t i = 0; i < funcRetTypes.size(); ++i) {
            if (i > 0) retType += ",";
            retType += funcRetTypes[i];
        }
        retType += ")>";
        
    } else if (!(check(TokenType::KW_INT) ||
          check(TokenType::KW_DOUBLE) ||
          check(TokenType::KW_STRING) ||
          check(TokenType::KW_ZERO)))
    {
        std::cerr << "Errore: atteso tipo di ritorno (int, double, string, <(...)>)\n";
        return nullptr;
    } else {
        retType = peek().lexeme;
        advance();
    }

    if (!match(TokenType::DOUBLE_COLON)) {
        std::cerr << "Errore: atteso ::\n";
        return nullptr;
    }

    auto func = std::make_shared<ASTNode>();
    func->type = "FunctionDef";
    func->value = fname;
    func->extra["returnType"] = retType;

    for (auto& p : params) {
        auto pn = std::make_shared<ASTNode>();
        pn->type = "Param";
        pn->value = p.second;
        pn->extra["paramType"] = p.first;
        func->children.push_back(pn);
    }

    auto body = std::make_shared<ASTNode>();
    body->type = "Body";

    while (match(TokenType::NEWLINE));

    while (!check(TokenType::KW_END) && !check(TokenType::END_OF_FILE)) {
        skipContinuationNewlines();
        auto stmt = parseStatement();
        if (stmt) body->children.push_back(stmt);
        while (match(TokenType::NEWLINE));
    }

    if (!match(TokenType::KW_END)) {
        std::cerr << "Errore: atteso end\n";
        return nullptr;
    }

    func->children.push_back(body);
    return func;
}

/* ============================================================
   Debug AST
   ============================================================ */

void Parser::printAST(const std::shared_ptr<ASTNode>& node, int indent) {
    for (int i=0; i<indent; i++)
        std::cout << "  ";

    std::cout << node->type;
    if (!node->value.empty())
        std::cout << " (" << node->value << ")";
    std::cout << "\n";

    for (auto& c : node->children)
        printAST(c, indent+1);
}

#include "lexer.h"
#include "debug.h"
#include <cctype>
#include <iostream>
#include <sstream>

/* ============================================================
   MAMMUTH LEXER
   Versione completamente commentata e patchata con:
   - Range ".." robusto
   - NumberLiteral sicuro
   - Nessun conflitto tra numeri e operatori range
   ============================================================ */


// ============================================================
// Funzione debug token
// ============================================================
static void debugToken(const Token& t) {
#if TOKEN_DUMP
    std::cout << "[TOKEN] "
              << t.lexeme
              << "  (type=" << (int)t.type
              << ", line=" << t.line
              << ", col=" << t.column
              << ")\n";
#endif
}

// ============================================================
// Funzione errore: formattazione coerente
// ============================================================
static void lexerError(int line, int column, const std::string& msg) {
    std::cerr << "Errore di analisi (riga " << line
              << ", colonna " << column
              << "): " << msg << "\n";
}

// ============================================================
// Constructor
// ============================================================
Lexer::Lexer(const std::string& src) : source(src) {}


// ============================================================
// Primitive utility
// ============================================================
char Lexer::peek() const {
    if (pos >= source.size()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    char c = peek();
    if (c == '\n') { line++; column = 1; }
    else column++;
    pos++;
    return c;
}

bool Lexer::match(char expected) {
    if (peek() != expected) return false;
    pos++; column++;
    return true;
}

void Lexer::skipWhitespace() {
    while (isspace(peek()) && peek() != '\n')
        advance();
}

void Lexer::skipComment() {
    // Single-line comment: # resto della linea
    while (peek() != '\n' && peek() != '\0')
        advance();
}

void Lexer::skipMultiLineComment() {
    // Multi-line comment: #[ ... ]#
    // Già consumato '#['
    int startLine = line;
    int startColumn = column - 2;
    
    while (true) {
        if (peek() == '\0') {
            lexerError(startLine, startColumn, 
                "Commento multi-line non chiuso (manca ]#)");
            return;
        }
        
        if (peek() == ']') {
            advance();  // consuma ']'
            if (peek() == '#') {
                advance();  // consuma '#'
                return;  // Fine commento multi-line
            }
        } else {
            advance();
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::string& lexeme) {
    return {type, lexeme, line, column};
}


// ============================================================
// STRINGHE
// ============================================================
Token Lexer::stringLiteral() {
    std::string value;
    int startLine = line, startColumn = column;

    while (peek() != '"' && peek() != '\0') {

        if (peek() == '\\') {
            advance(); // salta '\'
            char e = peek();
            switch (e) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                default:
                    lexerError(line, column,
                        std::string("Sequenza di escape sconosciuta: \\") + e);
                    value += e;
                    break;
            }
        } else {
            value += peek();
        }

        advance();
    }

    if (peek() == '"') {
        advance(); // chiude stringa
        return makeToken(TokenType::STRING, value);
    }

    lexerError(startLine, startColumn,
               "Stringa non chiusa prima della fine del file.");
    return makeToken(TokenType::STRING, value);
}


// ============================================================
// NUMERI (INT / DOUBLE) — versione patchata per range
// ============================================================
/*
   ⭐ PATCH RANGE
   La vecchia versione accettava "2." e poi mangiava ".." come double malformato.

   Qui facciamo:
   - Consumiamo solo le cifre iniziali
   - Se dopo le cifre c’è '.', controlliamo:
        se è ".." → STOP: non fa parte del numero
        se è "." + cifra → double valido
        se è "." + NON-cifra → double malformato
*/
Token Lexer::numberLiteral() {
    std::string num;
    int startLine   = line;
    int startColumn = column;

    // 1) Consuma tutte le cifre iniziali
    while (isdigit(peek())) {
        num += advance();
    }

    // 2) Se il prossimo è '.', può essere:
    //    - un double: "3.14"
    //    - un range: ".."
    //    - un errore: "3." seguito da lettera/vuoto
    if (peek() == '.') {

        // ⭐ Se è "..", NON fa parte del numero
        if (pos + 1 < source.size() && source[pos + 1] == '.') {
            // Il numero termina qui: INT
            return makeToken(TokenType::NUMBER_INT, num);
        }

        // Caso double: numero + "." + cifra?
        advance();
        num += '.';

        if (!isdigit(peek())) {
            // Caso "3." isolato → errore ma accettato come DOUBLE
            lexerError(startLine, startColumn,
                       "Numero malformato: termina con punto decimale.");
            return makeToken(TokenType::NUMBER_DBL, num);
        }

        // Consuma le cifre dopo il punto
        while (isdigit(peek())) {
            num += advance();
        }

        return makeToken(TokenType::NUMBER_DBL, num);
    }

    // Nessun punto → INT
    return makeToken(TokenType::NUMBER_INT, num);
}


// ============================================================
// IDENTIFICATORI + KEYWORDS
// ============================================================
Token Lexer::identifier() {
    std::string id;

    while (isalnum(peek()) || peek() == '_')
        id += advance();

    // parole chiave Mammuth
    if (id == "fixed")     return makeToken(TokenType::KW_FIXED, id);
    if (id == "dynamic")   return makeToken(TokenType::KW_DYNAMIC, id);
    if (id == "int")       return makeToken(TokenType::KW_INT, id);
    if (id == "double")    return makeToken(TokenType::KW_DOUBLE, id);
    if (id == "string")    return makeToken(TokenType::KW_STRING, id);
    if (id == "zero")      return makeToken(TokenType::KW_ZERO, id);
    if (id == "def")       return makeToken(TokenType::KW_DEF, id);
    if (id == "if")        return makeToken(TokenType::KW_IF, id);
    if (id == "elif")      return makeToken(TokenType::KW_ELIF, id);
    if (id == "else")      return makeToken(TokenType::KW_ELSE, id);
    if (id == "for")       return makeToken(TokenType::KW_FOR, id);
    if (id == "in")        return makeToken(TokenType::KW_IN, id);
    if (id == "while")     return makeToken(TokenType::KW_WHILE, id);
    if (id == "do")        return makeToken(TokenType::KW_DO, id);
    if (id == "end")       return makeToken(TokenType::KW_END, id);
    if (id == "echo")      return makeToken(TokenType::KW_ECHO, id);
    if (id == "err")       return makeToken(TokenType::KW_ERR, id);
    if (id == "break")     return makeToken(TokenType::KW_BREAK, id);
    if (id == "continue")  return makeToken(TokenType::KW_CONTINUE, id);
    if (id == "and")       return makeToken(TokenType::AND, id);
    if (id == "or")        return makeToken(TokenType::OR, id);
    if (id == "not")       return makeToken(TokenType::NOT, id);

    return makeToken(TokenType::IDENT, id);
}


// ============================================================
// TOKENIZE: ciclo principale
// ============================================================
std::vector<Token> Lexer::tokenize() {

    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();

        // ====================================================
        // COMMENTI
        // ====================================================
        if (peek() == '#') {
            advance();  // consuma '#'
            
            // Multi-line comment: #[ ... ]#
            if (peek() == '[') {
                advance();  // consuma '['
                skipMultiLineComment();
                continue;
            }
            
            // Single-line comment: # resto linea
            // Già consumato '#', skippa resto linea
            while (peek() != '\n' && peek() != '\0')
                advance();
            continue;
        }

        if (peek() == '\0')
            break;

        char c = advance();

        auto PUSH = [&](Token t) {
            tokens.push_back(t);
#if TOKEN_DUMP
            std::cout << "[TOKEN] "
                      << t.lexeme
                      << " (" << (int)t.type
                      << ") line=" << t.line
                      << " col=" << t.column << "\n";
#endif
        };


        // ====================================================
        // SWITCH PRINCIPALE CARATTERI SINGOLI
        // ====================================================
        switch (c) {

            // Arith
            case '+': PUSH(makeToken(TokenType::PLUS, "+")); break;

            case '-':
                if (match('>')) PUSH(makeToken(TokenType::ARROW, "->"));
                else PUSH(makeToken(TokenType::MINUS, "-"));
                break;

            case '*':
                if (match('*')) PUSH(makeToken(TokenType::POW, "**"));
                else PUSH(makeToken(TokenType::STAR, "*"));
                break;

            case '/': PUSH(makeToken(TokenType::SLASH, "/")); break;
            case '%': PUSH(makeToken(TokenType::MOD, "%")); break;

            // Concatenazione Mammuth
            case '$': PUSH(makeToken(TokenType::CONCAT, "$")); break;

            // Delimitatori
            case '(': PUSH(makeToken(TokenType::LPAREN, "(")); break;
            case ')': PUSH(makeToken(TokenType::RPAREN, ")")); break;
            case '[': PUSH(makeToken(TokenType::LBRACKET, "[")); break;
            case ']': PUSH(makeToken(TokenType::RBRACKET, "]")); break;
            case '{': PUSH(makeToken(TokenType::LBRACE, "{")); break;
            case '}': PUSH(makeToken(TokenType::RBRACE, "}")); break;
            case ',': PUSH(makeToken(TokenType::COMMA, ",")); break;

            // Logici e relazionali
            case '!':
                if (match('=')) PUSH(makeToken(TokenType::NEQ, "!="));
                else PUSH(makeToken(TokenType::NOT, "!"));
                break;

            case '<':
                if (match('=')) PUSH(makeToken(TokenType::LE, "<="));
                else if (match('<')) PUSH(makeToken(TokenType::SHL, "<<"));
                else PUSH(makeToken(TokenType::LT, "<"));
                break;

            case '>':
                if (match('=')) PUSH(makeToken(TokenType::GE, ">="));
                else if (match('>')) PUSH(makeToken(TokenType::SHR, ">>"));
                else PUSH(makeToken(TokenType::GT, ">"));
                break;

            case '&': PUSH(makeToken(TokenType::BAND, "&")); break;
            case '|': PUSH(makeToken(TokenType::BOR, "|")); break;
            case '^': PUSH(makeToken(TokenType::BXOR, "^")); break;
            case '~': PUSH(makeToken(TokenType::BNOT, "~")); break;

            // Condizionali Mammuth
            case '?':
                if (match('?')) PUSH(makeToken(TokenType::DOUBLE_QUESTION, "??"));
                else if (match(':')) PUSH(makeToken(TokenType::ELVIS, "?:"));
                else PUSH(makeToken(TokenType::QUESTION, "?"));
                break;

            case ':':
                if (peek() == ':') {
                    advance();
                    PUSH(makeToken(TokenType::DOUBLE_COLON, "::"));
                } else {
                    PUSH(makeToken(TokenType::COLON, ":"));
                }
                break;

            // ⭐ PATCH RANGE: "." e RANGE ".."
            case '.':
                if (match('.')) {
                    PUSH(makeToken(TokenType::RANGE, ".."));
                } else {
                    lexerError(line, column, "Carattere '.' inatteso.");
                }
                break;

            case '@':
                if (match('@')) PUSH(makeToken(TokenType::DOUBLE_AT, "@@"));
                else PUSH(makeToken(TokenType::AT, "@"));
                break;


            // Stringhe
            case '"':
                PUSH(stringLiteral());
                break;

            // Newline
            case '\n':
                PUSH(makeToken(TokenType::NEWLINE, "\\n"));
                break;

            // '=' o '==', '=>'
            case '=':
                if (match('=')) PUSH(makeToken(TokenType::EQ, "=="));
                else if (match('>')) PUSH(makeToken(TokenType::FAT_ARROW, "=>"));
                else PUSH(makeToken(TokenType::ASSIGN, "="));
                break;

            // Default: numeri o ident oppure errore
            default:

                if (isdigit(c)) {
                    // rimettiamo indietro per analizzare il numero completo
                    pos--; column--;
                    PUSH(numberLiteral());
                }
                else if (isalpha(c) || c == '_') {
                    pos--; column--;
                    PUSH(identifier());
                }
                else {
                    std::ostringstream msg;
                    msg << "Carattere sconosciuto: '" << c << "'";
                    lexerError(line, column, msg.str());
                }

                break;
        }
    }

    // EOF
    Token eof = makeToken(TokenType::END_OF_FILE, "EOF");
    tokens.push_back(eof);

#if TOKEN_DUMP
    std::cout << "[TOKEN] EOF\n";
#endif

    return tokens;
}

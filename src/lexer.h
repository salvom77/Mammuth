#ifndef MAMMUTH_LEXER_H
#define MAMMUTH_LEXER_H

#include <string>
#include <vector>

enum class TokenType {
    // Fine file
    END_OF_FILE,

    // Identificatori e letterali
    IDENT, NUMBER_INT, NUMBER_DBL, STRING,

    // Parole chiave principali
    KW_INT, KW_DOUBLE, KW_STRING, KW_ZERO,  // 🔹 tipi base Mammuth (incluso zero)
    KW_DEF,                                 // definizione funzione
    KW_IF, KW_ELIF, KW_ELSE,                // if/elif/else (v3.5)
    KW_FOR, KW_IN, KW_WHILE, KW_DO, KW_END, // controllo flusso (for future use)
    KW_ECHO, KW_INPUT, KW_ERR,              // I/O e diagnostica
    KW_BREAK, KW_CONTINUE,                  // loop control

    KW_DYNAMIC, // array dinamici (mutabili)
    KW_FIXED,   // variabili immutabili

    // Operatori aritmetici
    PLUS, MINUS, STAR, SLASH, MOD, POW,

    // Confronto
    EQ, NEQ, LT, GT, LE, GE,

    // Logici
    AND, OR, NOT,

    // Bitwise
    BAND, BOR, BXOR, BNOT, SHL, SHR,

    // Speciali Mammuth
    CONCAT,        // $
    ELVIS,         // ?:
    DOUBLE_QUESTION, // ??
    QUESTION,      // ?
    COLON,         // :
    DOUBLE_COLON,  // ::
    RANGE,         // ..
    ASSIGN,        // =
    ARROW,         // ->
    FAT_ARROW,     // =>

    // Delimitatori
    LPAREN, RPAREN,
    LBRACKET, RBRACKET,
    LBRACE, RBRACE,
    COMMA,

    // Speciali errori/debug
    AT, DOUBLE_AT,

    // Altro
    NEWLINE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

class Lexer {
public:
    explicit Lexer(const std::string& src);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    char peek() const;
    char advance();
    bool match(char expected);
    void skipWhitespace();
    void skipComment();
    void skipMultiLineComment();
    Token makeToken(TokenType type, const std::string& lexeme);
    Token stringLiteral();
    Token numberLiteral();
    Token identifier();
};

#endif // MAMMUTH_LEXER_H

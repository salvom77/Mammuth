#pragma once
#include <iostream>

// =======================================================
// 🔧 CONFIGURAZIONE DEBUG CENTRALE
//    Metti a 0/1 qui per attivare/disattivare i vari log
// =======================================================

// Master switch generale
#define DEBUG_MODE   1   // 1 = debug abilitato (sottocanali permettendo), 0 = tutto spento

// Dump dei token dal lexer (già usato)
#define TOKEN_DUMP   0   // 1 = stampa token, 0 = no

// Sottocanali mirati
#define DEBUG_PARSER 0   // Parser (struttura AST, dichiarazioni, ecc.)
#define DEBUG_INTERP 0   // Interpreter (esecuzione, eval, call funzioni)
#define DEBUG_ARRAY  0   // Tutto ciò che riguarda gli array (decl, accesso, assign)
#define DEBUG_SCOPE  0   // Gestione degli scope (push/pop, lookup, assegnazioni)

// =======================================================
// Macro base
// =======================================================

#if DEBUG_MODE
    #define DEBUG_LOG(msg) \
        do { std::cout << "[DEBUG] " << msg << std::endl; } while(0)
#else
    #define DEBUG_LOG(msg) \
        do {} while(0)
#endif

// Parser
#if DEBUG_MODE && DEBUG_PARSER
    #define DEBUG_PARSER_LOG(msg) \
        do { std::cout << "[PARSER] " << msg << std::endl; } while(0)
#else
    #define DEBUG_PARSER_LOG(msg) \
        do {} while(0)
#endif

// Interpreter
#if DEBUG_MODE && DEBUG_INTERP
    #define DEBUG_INTERP_LOG(msg) \
        do { std::cout << "[INTERP] " << msg << std::endl; } while(0)
#else
    #define DEBUG_INTERP_LOG(msg) \
        do {} while(0)
#endif

// Array (parser + interpreter)
#if DEBUG_MODE && DEBUG_ARRAY
    #define DEBUG_ARRAY_LOG(msg) \
        do { std::cout << "[ARRAY] " << msg << std::endl; } while(0)
#else
    #define DEBUG_ARRAY_LOG(msg) \
        do {} while(0)
#endif

// Scope (environment, lookup, assign)
#if DEBUG_MODE && DEBUG_SCOPE
    #define DEBUG_SCOPE_LOG(msg) \
        do { std::cout << "[SCOPE] " << msg << std::endl; } while(0)
#else
    #define DEBUG_SCOPE_LOG(msg) \
        do {} while(0)
#endif

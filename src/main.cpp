#include "driver.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "transpiler_cpp.h"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    Driver driver;

    if (!driver.parseArguments(argc, argv))
        return 0;  // help o errore già stampato

    std::string source;
    if (!driver.loadSource(source))
        return 1;

    std::cout << "File caricato: " << driver.opts.input_file << "\n";

    if (driver.opts.show_tokens) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        for (const auto& t : tokens)
            std::cout << t.line << ":" << t.column << "  " << t.lexeme << "\n";
        return 0;
    }

    if (driver.opts.show_ast) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parseProgram();

        std::cout << "AST:\n";
        parser.printAST(ast);
        return 0;
    }

    if (driver.opts.check_only) {
        std::cout << "[Stub] Controllo sintattico...\n";
        return 0;
    }

    if (driver.opts.compile) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parseProgram();
        CPPTranspiler cpptranspiler;
        std::string cpp_code = cpptranspiler.transpile(ast);

        // Scrivi su file
        std::ofstream out(driver.opts.output_file);
        out << cpp_code;
        out.close();

        std::cout << "C++ generato: " << driver.opts.output_file << "\n";
        return 0;
    }

    if (driver.opts.run) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parseProgram();

        Interpreter interp;
        interp.eval(ast);

        return 0;
    }

    return 0;
}


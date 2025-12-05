 #include "driver.h"
#include <iostream>
#include <fstream>

bool Driver::parseArguments(int argc, char* argv[]) {
    if (argc < 2) {
        printHelp();
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") opts.show_help = true;
        else if (arg == "-v" || arg == "--version") opts.show_version = true;
        else if (arg == "--tokens") opts.show_tokens = true;
        else if (arg == "--ast") opts.show_ast = true;
        else if (arg == "--check") opts.check_only = true;
        else if (arg == "--time") opts.time_exec = true;
        else if (arg == "--dump-errors") opts.dump_errors = true;
        else if (arg == "--keep-temp") opts.keep_temp = true;
        else if (arg == "--no-run") opts.no_run = true;
        else if (arg == "--run") opts.run = true;
        else if (arg == "--compile") opts.compile = true;
        else if (arg == "--backend" && i + 1 < argc) opts.backend = argv[++i];
        else if (arg == "--out" && i + 1 < argc) opts.output_file = argv[++i];
        else if (arg == "--errors" && i + 1 < argc) opts.errors_module = argv[++i];
        else if (arg[0] != '-') opts.input_file = arg;
        else {
            std::cerr << "Opzione sconosciuta: " << arg << "\n";
            return false;
        }
    }

    if (opts.show_help) {
        printHelp();
        return false;
    }
    if (opts.show_version) {
        printVersion();
        return false;
    }
    if (opts.input_file.empty()) {
        std::cerr << "Nessun file sorgente specificato.\n";
        return false;
    }
    return true;
}

void Driver::printHelp() const {
    std::cout <<
        "Mammuth Compiler/Interpreter (mammuthc)\n"
        "Uso: mammuthc [opzioni] file.mmt\n\n"
        "Opzioni principali:\n"
        "  --run              Esegue il programma (default)\n"
        "  --check            Controlla sintassi e tipi\n"
        "  --tokens           Mostra token\n"
        "  --ast              Mostra AST\n"
        "  --errors <mod>     Usa <mod>.err per la gestione errori\n"
        "  --dump-errors      Elenca gestori errori caricati\n"
        "  --compile          Genera codice C++ e compila\n"
        "  --backend <comp>   Seleziona backend (gcc, clang, msvc)\n"
        "  --out <file>       Nome file eseguibile\n"
        "  --keep-temp        Mantiene file temporanei\n"
        "  --time             Mostra tempi di esecuzione\n"
        "  -h, --help         Mostra questo aiuto\n"
        "  -v, --version      Mostra versione del compilatore\n";
}

void Driver::printVersion() const {
    std::cout << "Mammuth Compiler v1.0 (alpha)\n";
}

bool Driver::loadSource(std::string& code) const {
    std::ifstream file(opts.input_file);
    if (!file) {
        std::cerr << "Impossibile aprire file: " << opts.input_file << "\n";
        return false;
    }
    code.assign((std::istreambuf_iterator<char>(file)),
                (std::istreambuf_iterator<char>()));
    return true;
}


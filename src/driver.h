
#ifndef MAMMUTH_DRIVER_H
#define MAMMUTH_DRIVER_H

#include <string>
#include <vector>

struct Options {
    bool show_help = false;
    bool show_version = false;
    bool show_tokens = false;
    bool show_ast = false;
    bool check_only = false;
    bool run = true;
    bool compile = false;
    bool time_exec = false;
    bool dump_errors = false;
    bool keep_temp = false;
    bool no_run = false;

    std::string backend = "gcc";
    std::string errors_module;
    std::string output_file = "a.out";
    std::string input_file;
};

class Driver {
public:
    Options opts;

    bool parseArguments(int argc, char* argv[]);
    void printHelp() const;
    void printVersion() const;
    bool loadSource(std::string& code) const;
};

#endif // MAMMUTH_DRIVER_H

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "optiq/ast_printer.hpp"
#include "optiq/parser.hpp"

namespace {

void printUsage(std::ostream& output) {
    output << "Usage: optiq_cli [--ast] [SOURCE | FILE]\n"
              "       Compile an Optiq program and print its typed AST as JSON.\n"
              "       Source is read from a file path, inline arguments, or stdin\n"
              "       when no argument is supplied.\n\n"
              "Exit codes: 0 = valid input, 2 = lexical or syntax error, 1 = internal error.\n";
}

bool isSourceFile(const std::string& argument) {
    std::error_code status;
    return std::filesystem::is_regular_file(argument, status);
}

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.good()) {
        throw std::runtime_error("Unable to read source file: " + path);
    }
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

int reportErrors(const std::vector<optiq::Diagnostic>& errors) {
    for (const auto& error : errors) {
        std::cerr << "error at line " << error.location.line
                  << ", column " << error.location.column << ": "
                  << error.message << '\n';
    }
    return 2;
}

} // namespace

int main(int argc, char* argv[]) {
    std::string source;

    try {
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--help" || argument == "-h") {
                printUsage(std::cout);
                return 0;
            }
            if (argument == "--ast") continue;
            if (source.empty() && isSourceFile(argument)) {
                source = readFile(argument);
                continue;
            }
            if (!source.empty()) source.push_back(' ');
            source += argument;
        }

        if (source.empty()) {
            source.assign(std::istreambuf_iterator<char>(std::cin),
                          std::istreambuf_iterator<char>());
        }

        auto result = optiq::parseProgram(source);
        if (!result.success()) return reportErrors(result.errors);
        optiq::writeProgramJson(std::cout, *result.program);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Internal compiler error: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Internal compiler error: unknown failure\n";
        return 1;
    }
}

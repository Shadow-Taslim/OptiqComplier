#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include "optiq/cpp/lexer.hpp"

namespace {

void printUsage(std::ostream& output) {
    output << "Usage: optiq_cpp [--tokens] <source-file>\n"
              "       optiq_cpp [--tokens] < source-file\n"
              "\n"
              "Tokenize the documented OptiQ C/C++ common subset.\n"
              "No preprocessing, parsing, semantic analysis, or code generation is performed.\n"
              "\n"
              "Options:\n"
              "  --tokens  Display the token table (the default output mode)\n"
              "  --help    Display this help text\n";
}

void printTokens(const std::vector<optiq::cpp::Token>& tokens) {
    std::cout << std::left << std::setw(22) << "TOKEN TYPE"
              << std::setw(32) << "LEXEME"
              << std::setw(10) << "LINE" << "COLUMN\n";

    for (const auto& token : tokens) {
        std::cout << std::left << std::setw(22) << optiq::cpp::tokenTypeName(token.type)
                  << std::setw(32) << token.lexeme
                  << std::setw(10) << token.location.line << token.location.column << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::string path;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--help" || argument == "-h") {
                printUsage(std::cout);
                return 0;
            }
            if (argument == "--tokens") continue;
            if (!path.empty()) {
                std::cerr << "Error: more than one source file was provided\n";
                printUsage(std::cerr);
                return 1;
            }
            path = argument;
        }

        std::string source;
        if (path.empty()) {
            source.assign(std::istreambuf_iterator<char>(std::cin),
                          std::istreambuf_iterator<char>());
        } else {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                std::cerr << "Error: unable to open source file '" << path << "'\n";
                return 1;
            }
            source.assign(std::istreambuf_iterator<char>(input),
                          std::istreambuf_iterator<char>());
        }

        const auto result = optiq::cpp::tokenize(source);
        printTokens(result.tokens);
        for (const auto& diagnostic : result.diagnostics) {
            std::cerr << "Lexical error at line " << diagnostic.line << ", column "
                      << diagnostic.column << ": " << diagnostic.message << '\n';
        }
        return result.success() ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "Internal error: " << error.what() << '\n';
        return 1;
    }
}

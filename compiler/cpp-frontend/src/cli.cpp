#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include "optiq/cpp/ast_printer.hpp"
#include "optiq/cpp/lexer.hpp"
#include "optiq/cpp/parser.hpp"

namespace {

enum class Mode { Tokens, Parse, Ast };

void printUsage(std::ostream& output) {
    output << "Usage: optiq_cpp [--tokens|--parse|--ast] <source-file>\n"
              "       optiq_cpp [--tokens|--parse|--ast] < source-file\n"
              "\n"
              "Process the documented OptiQ C/C++ common subset.\n"
              "Parsing builds a preliminary AST; no semantic analysis or code generation is performed.\n"
              "\n"
              "Options:\n"
              "  --tokens  Display the token table (the default output mode)\n"
              "  --parse   Validate syntax and report lexer/parser status\n"
              "  --ast     Parse and print the preliminary AST\n"
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

void printDiagnostic(const std::string& sourceName, const char* category,
                     const optiq::cpp::Diagnostic& diagnostic) {
    std::cerr << sourceName << ':' << diagnostic.line << ':' << diagnostic.column
              << ": " << category << ": " << diagnostic.message << '\n';
}

int runParser(const std::string& source, const std::string& sourceName, Mode mode) {
    const auto result = optiq::cpp::parse(source);
    for (const auto& diagnostic : result.lexicalDiagnostics) {
        printDiagnostic(sourceName, "lexical error", diagnostic);
    }
    for (const auto& diagnostic : result.syntaxDiagnostics) {
        printDiagnostic(sourceName, "syntax error", diagnostic);
    }

    if (!result.success()) {
        std::cout << "OptiQ Compiler\n\n"
                  << "Source: " << sourceName << '\n'
                  << "Lexer: " << (result.lexicalDiagnostics.empty() ? "SUCCESS" : "FAILED")
                  << '\n'
                  << "Parser: "
                  << (result.lexicalDiagnostics.empty() ? "FAILED" : "NOT RUN") << '\n'
                  << "AST: NOT GENERATED\n";
        return 2;
    }

    if (mode == Mode::Ast) {
        optiq::cpp::printAst(*result.program, std::cout);
        return 0;
    }

    std::cout << "OptiQ Compiler\n\n"
              << "Source: " << sourceName << '\n'
              << "Lexer: SUCCESS\n"
              << "Parser: SUCCESS\n"
              << "AST: GENERATED\n\n"
              << "Syntax analysis completed successfully.\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        Mode mode = Mode::Tokens;
        bool modeSelected = false;
        std::string path;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--help" || argument == "-h") {
                printUsage(std::cout);
                return 0;
            }

            Mode requestedMode = mode;
            bool isMode = true;
            if (argument == "--tokens") requestedMode = Mode::Tokens;
            else if (argument == "--parse") requestedMode = Mode::Parse;
            else if (argument == "--ast") requestedMode = Mode::Ast;
            else isMode = false;

            if (isMode) {
                if (modeSelected && requestedMode != mode) {
                    std::cerr << "Error: --tokens, --parse, and --ast are mutually exclusive\n";
                    return 1;
                }
                mode = requestedMode;
                modeSelected = true;
                continue;
            }

            if (!argument.empty() && argument.front() == '-') {
                std::cerr << "Error: unknown option '" << argument << "'\n";
                return 1;
            }
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

        if (mode != Mode::Tokens) {
            return runParser(source, path.empty() ? "<stdin>" : path, mode);
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

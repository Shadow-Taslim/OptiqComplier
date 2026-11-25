#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "optiq/ast_printer.hpp"
#include "optiq/lexer.hpp"
#include "optiq/parser.hpp"
#include "optiq/semantic.hpp"
#include "optiq/tac.hpp"

namespace {

void printUsage(std::ostream& output) {
    output << "Usage: optiq_cli [MODE] [SOURCE | FILE]\n"
              "       Compile a .mc program through lex, parse, semantic analysis, and TAC.\n"
              "       (none)   Emit Three Address Code (the final compiler output).\n"
              "       --tokens  Print the token stream and stop.\n"
              "       --ast     Print the AST (indented text) and stop.\n"
              "       --json    Print the AST as JSON and stop.\n"
              "       --check   Run semantic analysis only; no TAC output.\n"
              "       Source is read from a file path, inline arguments, or stdin\n"
              "       when no argument is supplied.\n\n"
              "Exit codes: 0 = compiled, 2 = lexical/syntax error,\n"
              "            3 = semantic error, 1 = internal error.\n";
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

void reportError(const std::string& phase, const optiq::SourceLocation& location,
                 const std::string& message) {
    std::cerr << phase << " error at line " << location.line << ", column "
              << location.column << ": " << message << '\n';
}

std::string tokenValueText(const optiq::Token& token) {
    switch (token.kind) {
    case optiq::TokenKind::Identifier:
        return " \"" + std::get<std::string>(token.value) + "\"";
    case optiq::TokenKind::IntegerLiteral:
        return " " + std::to_string(std::get<std::int64_t>(token.value));
    case optiq::TokenKind::FloatLiteral:
        return " " + std::to_string(std::get<double>(token.value));
    case optiq::TokenKind::True:
    case optiq::TokenKind::False:
        return std::get<bool>(token.value) ? " true" : " false";
    default:
        return "";
    }
}

} // namespace

int main(int argc, char* argv[]) {
    enum class Mode { Tac, Tokens, Ast, Json, Check };
    Mode mode = Mode::Tac;
    std::string source;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            printUsage(std::cout);
            return 0;
        }
        if (argument == "--tokens") { mode = Mode::Tokens; continue; }
        if (argument == "--ast") { mode = Mode::Ast; continue; }
        if (argument == "--json") { mode = Mode::Json; continue; }
        if (argument == "--check") { mode = Mode::Check; continue; }
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

    try {
        // Phase 1: lexical analysis.
        optiq::LexResult lexed = optiq::tokenize(source);
        if (lexed.error) {
            reportError("Lexical", lexed.error->location, lexed.error->message);
            return 2;
        }
        if (mode == Mode::Tokens) {
            for (const auto& token : lexed.tokens) {
                std::cout << "line " << token.location.line << ", column "
                          << token.location.column << ": "
                          << optiq::tokenKindName(token.kind)
                          << tokenValueText(token) << '\n';
            }
            return 0;
        }

        // Phase 2: syntax analysis (with Bison error recovery).
        optiq::ParseResult parsed = optiq::parseProgram(source);
        if (!parsed.errors.empty()) {
            for (const auto& error : parsed.errors) {
                reportError("Syntax", error.location, error.message);
            }
            return 2;
        }
        if (mode == Mode::Ast) {
            optiq::writeProgramText(std::cout, *parsed.program);
            return 0;
        }
        if (mode == Mode::Json) {
            optiq::writeProgramJson(std::cout, *parsed.program);
            std::cout << '\n';
            return 0;
        }

        // Phase 3: semantic analysis (symbol table + type checking).
        optiq::SemanticResult semantics = optiq::analyze(*parsed.program);
        if (!semantics.errors.empty()) {
            for (const auto& error : semantics.errors) {
                reportError("Semantic", error.location, error.message);
            }
            return 3;
        }
        if (mode == Mode::Check) {
            std::cout << "Semantic analysis passed: no errors\n";
            return 0;
        }

        // Phase 4: intermediate code generation (final output).
        std::cout << optiq::tacToString(optiq::generateTac(*parsed.program));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Internal compiler error: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Internal compiler error: unknown failure\n";
        return 1;
    }
}

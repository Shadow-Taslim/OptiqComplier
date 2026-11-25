// Drives every program in tests/programs through the full compiler pipeline
// (lex -> parse -> semantic -> TAC) and compares the emitted output with the
// golden file of the same name in tests/expected.
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

#include "optiq/parser.hpp"
#include "optiq/semantic.hpp"
#include "optiq/tac.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    require(input.good(), "file exists: " + path.string());
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

std::string formatError(const std::string& phase,
                        const optiq::SourceLocation& location,
                        const std::string& message) {
    std::ostringstream text;
    text << phase << " error at line " << location.line << ", column "
         << location.column << ": " << message << '\n';
    return text.str();
}

// Mirrors the CLI pipeline, returning what the CLI would print (stdout for
// valid programs, stderr for rejected ones).
std::string compile(const std::string& source) {
    optiq::LexResult lexed = optiq::tokenize(source);
    if (lexed.error) {
        return formatError("Lexical", lexed.error->location, lexed.error->message);
    }
    optiq::ParseResult parsed = optiq::parseProgram(source);
    if (!parsed.errors.empty()) {
        std::string output;
        for (const auto& error : parsed.errors) {
            output += formatError("Syntax", error.location, error.message);
        }
        return output;
    }
    optiq::SemanticResult semantics = optiq::analyze(*parsed.program);
    if (!semantics.errors.empty()) {
        std::string output;
        for (const auto& error : semantics.errors) {
            output += formatError("Semantic", error.location, error.message);
        }
        return output;
    }
    return optiq::tacToString(optiq::generateTac(*parsed.program));
}

} // namespace

int main(int argc, char* argv[]) {
    require(argc == 2, "example test requires the repository root path");
    const std::filesystem::path root = argv[1];
    const std::filesystem::path programs = root / "tests" / "programs";
    const std::filesystem::path expected = root / "tests" / "expected";

    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(programs)) {
        if (entry.path().extension() != ".mc") continue;
        const std::string name = entry.path().stem().string();
        const auto golden = expected / (name + ".out");
        const std::string actual = compile(readFile(entry.path()));
        const std::string wanted = readFile(golden);
        if (actual != wanted) {
            std::cerr << "FAIL: output mismatch for " << name << "\nExpected:\n"
                      << wanted << "Actual:\n" << actual;
            return 1;
        }
        ++count;
    }
    require(count == 11, "all eleven test programs are exercised");

    std::cout << "PASS: all test programs match their expected outputs\n";
    return 0;
}

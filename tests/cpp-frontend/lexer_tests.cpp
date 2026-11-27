#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "optiq/cpp/lexer.hpp"

using optiq::cpp::LexResult;
using optiq::cpp::TokenType;

namespace {

struct Runner {
    int checks = 0;
    int failures = 0;

    void check(bool condition, const std::string& detail) {
        ++checks;
        if (!condition) {
            ++failures;
            std::cerr << "  FAILED CHECK: " << detail << '\n';
        }
    }
};

void expectTypes(Runner& runner, const std::string& source,
                 const std::vector<TokenType>& expected,
                 const std::vector<std::string>& lexemes = {}) {
    const auto result = optiq::cpp::tokenize(source);
    runner.check(result.success(), "expected no lexical diagnostics for: " + source);
    runner.check(result.tokens.size() == expected.size(),
                 "expected " + std::to_string(expected.size()) + " tokens, got " +
                     std::to_string(result.tokens.size()));

    const auto count = std::min(result.tokens.size(), expected.size());
    for (std::size_t i = 0; i < count; ++i) {
        runner.check(result.tokens[i].type == expected[i],
                     "unexpected token type at index " + std::to_string(i) +
                         ": " + result.tokens[i].lexeme);
        if (!lexemes.empty() && i < lexemes.size()) {
            runner.check(result.tokens[i].lexeme == lexemes[i],
                         "unexpected lexeme at index " + std::to_string(i));
        }
    }
}

void expectError(Runner& runner, const std::string& source,
                 const std::string& messagePart) {
    const auto result = optiq::cpp::tokenize(source);
    runner.check(!result.success(), "expected a lexical diagnostic for: " + source);
    const auto found = std::find_if(
        result.diagnostics.begin(), result.diagnostics.end(),
        [&](const auto& diagnostic) {
            return diagnostic.message.find(messagePart) != std::string::npos;
        });
    runner.check(found != result.diagnostics.end(),
                 "expected diagnostic containing '" + messagePart + "' for: " + source);
    for (const auto& diagnostic : result.diagnostics) {
        runner.check(diagnostic.line >= 1 && diagnostic.column >= 1,
                     "diagnostic must have a one-based source location");
    }
}

void testKeywords(Runner& runner) {
    const std::string source =
        "int float double char bool void if else while for do return "
        "break continue true false";
    expectTypes(runner, source, std::vector<TokenType>(16, TokenType::Keyword));
}

void testIdentifiers(Runner& runner) {
    expectTypes(runner, "x total studentName _value counter123 Int intx",
                std::vector<TokenType>(7, TokenType::Identifier));
}

void testIntegerLiterals(Runner& runner) {
    expectTypes(runner, "0 42 1000",
                std::vector<TokenType>(3, TokenType::IntegerLiteral),
                {"0", "42", "1000"});
}

void testFloatingLiterals(Runner& runner) {
    expectTypes(runner, "3.14 0.5 25.50 1.25f 2.0F",
                std::vector<TokenType>(5, TokenType::FloatingLiteral),
                {"3.14", "0.5", "25.50", "1.25f", "2.0F"});
}

void testCharacterLiterals(Runner& runner) {
    expectTypes(runner, R"('a' 'Z' '\n' '\t' '\\' '\'' '"')",
                std::vector<TokenType>(7, TokenType::CharacterLiteral));
}

void testStringLiterals(Runner& runner) {
    expectTypes(runner,
                R"("Hello World" "OptiQ Compiler" "line\n" "tab\t" "quote:\"" "apostrophe:\'" "slash:\\")",
                std::vector<TokenType>(7, TokenType::StringLiteral));
}

void testArithmeticOperators(Runner& runner) {
    expectTypes(runner, "+ - * / %",
                {TokenType::Plus, TokenType::Minus, TokenType::Star,
                 TokenType::Slash, TokenType::Percent});
}

void testRelationalOperators(Runner& runner) {
    expectTypes(runner, "< > <= >= == !=",
                {TokenType::Less, TokenType::Greater, TokenType::LessEqual,
                 TokenType::GreaterEqual, TokenType::EqualEqual, TokenType::NotEqual});
    expectTypes(runner, "x==1",
                {TokenType::Identifier, TokenType::EqualEqual, TokenType::IntegerLiteral});
}

void testLogicalOperators(Runner& runner) {
    expectTypes(runner, "&& || !",
                {TokenType::LogicalAnd, TokenType::LogicalOr, TokenType::LogicalNot});
}

void testAssignmentOperators(Runner& runner) {
    expectTypes(runner, "= += -= *= /=",
                {TokenType::Assignment, TokenType::PlusAssignment,
                 TokenType::MinusAssignment, TokenType::StarAssignment,
                 TokenType::SlashAssignment});
    expectTypes(runner, "x+=1",
                {TokenType::Identifier, TokenType::PlusAssignment,
                 TokenType::IntegerLiteral});
}

void testIncrementDecrement(Runner& runner) {
    expectTypes(runner, "x++ --y",
                {TokenType::Identifier, TokenType::Increment,
                 TokenType::Decrement, TokenType::Identifier});
}

void testBitwiseOperators(Runner& runner) {
    expectTypes(runner, "& | ^ ~ << >> -> . ::",
                {TokenType::BitwiseAnd, TokenType::BitwiseOr, TokenType::BitwiseXor,
                 TokenType::BitwiseNot, TokenType::ShiftLeft, TokenType::ShiftRight,
                 TokenType::Arrow, TokenType::Dot, TokenType::Scope});
}

void testDelimiters(Runner& runner) {
    expectTypes(runner, "( ) { } [ ] ; , :",
                {TokenType::LeftParen, TokenType::RightParen, TokenType::LeftBrace,
                 TokenType::RightBrace, TokenType::LeftBracket, TokenType::RightBracket,
                 TokenType::Semicolon, TokenType::Comma, TokenType::Colon});
}

void testComments(Runner& runner) {
    expectTypes(runner,
                "int x; // ignored\n/* multi\nline comment */\nx = 2;",
                {TokenType::Keyword, TokenType::Identifier, TokenType::Semicolon,
                 TokenType::Identifier, TokenType::Assignment,
                 TokenType::IntegerLiteral, TokenType::Semicolon});
}

void testPreprocessor(Runner& runner) {
    const auto result = optiq::cpp::tokenize(
        "  #include <iostream>\n#define MAX 100\n#include \"local.hpp\"");
    runner.check(result.success(), "supported directives must not produce diagnostics");
    runner.check(result.tokens.size() == 3, "expected three preprocessor tokens");
    for (const auto& token : result.tokens) {
        runner.check(token.type == TokenType::Preprocessor,
                     "directive must be a PREPROCESSOR token");
    }
    if (!result.tokens.empty()) {
        runner.check(result.tokens[0].lexeme == "#include <iostream>",
                     "leading whitespace must not enter the preprocessor lexeme");
        runner.check(result.tokens[0].location.line == 1 &&
                         result.tokens[0].location.column == 3,
                     "preprocessor location must identify the # character");
    }
    expectError(runner, "#pragma once", "Unsupported preprocessor directive");
    expectError(runner, "#include", "Malformed #include directive");
    expectError(runner, "#define 123 value", "Malformed #define directive");
}

void testInvalidCharacters(Runner& runner) {
    const auto result = optiq::cpp::tokenize("int x; @ $");
    runner.check(!result.success(), "invalid characters must fail lexical analysis");
    runner.check(result.tokens.size() == 3, "valid tokens around invalid characters remain");
    runner.check(result.diagnostics.size() == 2, "expected two invalid-character diagnostics");
    if (result.diagnostics.size() == 2) {
        runner.check(result.diagnostics[0].line == 1 && result.diagnostics[0].column == 8,
                     "@ must be reported at line 1, column 8");
        runner.check(result.diagnostics[1].line == 1 && result.diagnostics[1].column == 10,
                     "$ must be reported at line 1, column 10");
    }
}

void testMalformedLiterals(Runner& runner) {
    expectError(runner, "123abc", "Malformed numeric literal");
    expectError(runner, "12.3.4", "Malformed numeric literal");
    expectError(runner, "1.0ff", "Malformed numeric literal");
    expectError(runner, "1e3", "Malformed numeric literal");
    expectError(runner, ".5", "Malformed numeric literal");
    expectError(runner, "\"unterminated", "Unterminated string literal");
    expectError(runner, "'x", "Unterminated character literal");
    expectError(runner, "/* unterminated", "Unterminated block comment");
    expectError(runner, "\"bad\\q\"", "Invalid escape sequence");
    expectError(runner, "''", "Empty character literal");
    expectError(runner, "'ab'", "exactly one character");
}

void testSourceLocations(Runner& runner) {
    const auto result = optiq::cpp::tokenize("int x;\n  x = 2;\n");
    runner.check(result.success(), "location fixture must tokenize successfully");
    runner.check(result.tokens.size() == 7, "expected seven tokens in location fixture");
    if (result.tokens.size() != 7) return;

    const std::vector<std::size_t> lines = {1, 1, 1, 2, 2, 2, 2};
    const std::vector<std::size_t> columns = {1, 5, 6, 3, 5, 7, 8};
    for (std::size_t i = 0; i < result.tokens.size(); ++i) {
        runner.check(result.tokens[i].location.line == lines[i],
                     "unexpected line for token index " + std::to_string(i));
        runner.check(result.tokens[i].location.column == columns[i],
                     "unexpected column for token index " + std::to_string(i));
    }
}

struct TestCase {
    const char* name;
    const char* input;
    const char* expected;
    void (*run)(Runner&);
};

const std::vector<TestCase>& tests() {
    static const std::vector<TestCase> all = {
        {"keywords", "All 16 documented keywords", "16 KEYWORD tokens", testKeywords},
        {"identifiers", "x total studentName _value counter123 Int intx", "7 IDENTIFIER tokens", testIdentifiers},
        {"integer_literals", "0 42 1000", "3 INTEGER_LITERAL tokens", testIntegerLiterals},
        {"floating_literals", "3.14 0.5 25.50 1.25f 2.0F", "5 FLOATING_LITERAL tokens", testFloatingLiterals},
        {"character_literals", "Simple characters and five documented escapes", "7 CHARACTER_LITERAL tokens", testCharacterLiterals},
        {"string_literals", "Text and documented escape sequences", "7 STRING_LITERAL tokens", testStringLiterals},
        {"arithmetic_operators", "+ - * / %", "Five distinct arithmetic tokens", testArithmeticOperators},
        {"relational_operators", "< > <= >= == != and x==1", "Longest-match relational tokens", testRelationalOperators},
        {"logical_operators", "&& || !", "LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT", testLogicalOperators},
        {"assignment_operators", "= += -= *= /= and x+=1", "Longest-match assignment tokens", testAssignmentOperators},
        {"increment_decrement", "x++ --y", "INCREMENT and DECREMENT tokens", testIncrementDecrement},
        {"bitwise_operators", "& | ^ ~ << >> -> . ::", "Nine bitwise/other operator tokens", testBitwiseOperators},
        {"delimiters", "( ) { } [ ] ; , :", "Nine delimiter tokens", testDelimiters},
        {"comments", "Line and multi-line comments around int/x/2", "Comments discarded; 7 tokens retained", testComments},
        {"preprocessor", "include/define plus pragma and malformed directives", "3 supported tokens; 3 diagnostics", testPreprocessor},
        {"invalid_characters", "int x; @ $", "3 valid tokens; diagnostics at columns 8 and 10", testInvalidCharacters},
        {"malformed_literals", "Malformed numbers, literals, escapes, and comment", "Each case produces its specific diagnostic", testMalformedLiterals},
        {"source_locations", "int x; then an indented assignment on line 2", "Seven exact line/column positions", testSourceLocations},
    };
    return all;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: optiq_cpp_lexer_tests <test-name>\n";
        return 1;
    }

    const auto& all = tests();
    const auto test = std::find_if(all.begin(), all.end(), [&](const auto& candidate) {
        return candidate.name == std::string(argv[1]);
    });
    if (test == all.end()) {
        std::cerr << "Unknown test: " << argv[1] << '\n';
        return 1;
    }

    std::cout << "Test: " << test->name << '\n'
              << "Input: " << test->input << '\n'
              << "Expected: " << test->expected << '\n';
    Runner runner;
    test->run(runner);
    std::cout << "Actual: " << runner.checks << " checks executed; " << runner.failures
              << " failed\n"
              << (runner.failures == 0 ? "PASS\n" : "FAIL\n");
    return runner.failures == 0 ? 0 : 1;
}

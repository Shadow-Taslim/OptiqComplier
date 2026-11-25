#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "optiq/lexer.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

void expectKinds(const std::string& source,
                 const std::vector<optiq::TokenKind>& expected) {
    const auto result = optiq::tokenize(source);
    require(result.success(), "tokenization succeeds for: " + source);
    require(result.tokens.size() == expected.size(), "token count matches for: " + source);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(result.tokens[i].kind == expected[i],
                "token " + std::to_string(i) + " matches for: " + source);
    }
}

} // namespace

int main() {
    using optiq::TokenKind;
    const auto end = TokenKind::End;

    expectKinds("int float bool if else while print true false",
                {TokenKind::Int, TokenKind::Float, TokenKind::Bool, TokenKind::If,
                 TokenKind::Else, TokenKind::While, TokenKind::Print,
                 TokenKind::True, TokenKind::False, end});

    // Keywords are case-sensitive; `Int` and `PRINT` are plain identifiers.
    expectKinds("Int PRINT True",
                {TokenKind::Identifier, TokenKind::Identifier, TokenKind::Identifier,
                 end});

    expectKinds("_count total9 x_y",
                {TokenKind::Identifier, TokenKind::Identifier, TokenKind::Identifier,
                 end});

    expectKinds("42 3.14 0.5 100",
                {TokenKind::IntegerLiteral, TokenKind::FloatLiteral,
                 TokenKind::FloatLiteral, TokenKind::IntegerLiteral, end});

    expectKinds("+ - * / % < > <= >= == != && || ! =",
                {TokenKind::Plus, TokenKind::Minus, TokenKind::Star, TokenKind::Slash,
                 TokenKind::Percent, TokenKind::Less, TokenKind::Greater,
                 TokenKind::LessEqual, TokenKind::GreaterEqual,
                 TokenKind::EqualEqual, TokenKind::NotEqual, TokenKind::AndAnd,
                 TokenKind::OrOr, TokenKind::Bang, TokenKind::Assign, end});

    expectKinds("{ ( ) ; }",
                {TokenKind::LeftBrace, TokenKind::LeftParen, TokenKind::RightParen,
                 TokenKind::Semicolon, TokenKind::RightBrace, end});

    // Comments are discarded, never tokenized.
    expectKinds("// line comment\nint x; /* block\ncomment */ x = 1;",
                {TokenKind::Int, TokenKind::Identifier, TokenKind::Semicolon,
                 TokenKind::Identifier, TokenKind::Assign,
                 TokenKind::IntegerLiteral, TokenKind::Semicolon, end});

    const auto values = optiq::tokenize("17 2.5 flag true");
    require(values.success(), "literal tokenization succeeds");
    require(std::get<std::int64_t>(values.tokens[0].value) == 17,
            "integer value is converted");
    require(std::get<double>(values.tokens[1].value) == 2.5,
            "float value is converted");
    require(std::get<std::string>(values.tokens[2].value) == "flag",
            "identifier keeps its name");
    require(std::get<bool>(values.tokens[3].value) == true,
            "true carries a boolean value");

    const auto locations = optiq::tokenize("int x;\nx = 1;");
    require(locations.success(), "multiline tokenization succeeds");
    require(locations.tokens[3].location.line == 2 &&
                locations.tokens[3].location.column == 1,
            "line and column tracking is accurate across newlines");

    const auto commentLines = optiq::tokenize("/* a\nb */ int x;");
    require(commentLines.success(), "block comment tokenizes");
    require(commentLines.tokens[0].location.line == 2,
            "line tracking survives block comments");

    // Lexical errors carry precise locations.
    const auto invalid = optiq::tokenize("int x = @;");
    require(!invalid.success(), "invalid character is rejected");
    require(invalid.error->location.line == 1 && invalid.error->location.column == 9,
            "lexical error has a precise location");

    require(!optiq::tokenize("int x = 12abc;").success(),
            "malformed numeric literal is rejected");
    require(!optiq::tokenize("int x = 999999999999999999999999;").success(),
            "out-of-range integer is rejected");
    require(!optiq::tokenize("/* open").success(),
            "unterminated block comment is rejected");

    std::cout << "PASS: lexer keywords, literals, operators, comments, locations\n";
    return 0;
}

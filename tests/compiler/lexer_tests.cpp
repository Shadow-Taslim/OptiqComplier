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

void expectKinds(const std::string& sql, const std::vector<optiq::TokenKind>& expected) {
    const auto result = optiq::tokenize(sql);
    require(result.success(), "tokenization succeeds for: " + sql);
    require(result.tokens.size() == expected.size(), "token count matches for: " + sql);
    for (std::size_t i = 0; i < expected.size(); ++i) {
        require(result.tokens[i].kind == expected[i],
                "token " + std::to_string(i) + " matches for: " + sql);
    }
}

} // namespace

int main() {
    using optiq::TokenKind;
    const auto end = TokenKind::End;

    expectKinds("select SeLeCt FROM from WHERE Group by ORDER asc DESC LIMIT",
                {TokenKind::Select, TokenKind::Select, TokenKind::From, TokenKind::From,
                 TokenKind::Where, TokenKind::Group, TokenKind::By, TokenKind::Order,
                 TokenKind::Asc, TokenKind::Desc, TokenKind::Limit, end});
    expectKinds("AS INNER JOIN ON AND OR NOT COUNT SUM AVG MIN MAX",
                {TokenKind::As, TokenKind::Inner, TokenKind::Join, TokenKind::On,
                 TokenKind::And, TokenKind::Or, TokenKind::Not, TokenKind::Count,
                 TokenKind::Sum, TokenKind::Average, TokenKind::Minimum,
                 TokenKind::Maximum, end});
    expectKinds("employee e._col2 ( ) , ; . = == != <> < <= > >= + - * /",
                {TokenKind::Identifier, TokenKind::Identifier, TokenKind::Dot,
                 TokenKind::Identifier, TokenKind::LeftParen, TokenKind::RightParen,
                 TokenKind::Comma, TokenKind::Semicolon, TokenKind::Dot,
                 TokenKind::Equal, TokenKind::Equal, TokenKind::NotEqual,
                 TokenKind::NotEqual, TokenKind::Less, TokenKind::LessEqual,
                 TokenKind::Greater, TokenKind::GreaterEqual, TokenKind::Plus,
                 TokenKind::Minus, TokenKind::Star, TokenKind::Slash, end});
    expectKinds("SELECT 42 3.14 'text' FROM t",
                {TokenKind::Select, TokenKind::IntegerLiteral, TokenKind::DecimalLiteral,
                 TokenKind::StringLiteral, TokenKind::From, TokenKind::Identifier, end});
    expectKinds("let Let IF Else WHILE print TRUE false",
                {TokenKind::Let, TokenKind::Let, TokenKind::If, TokenKind::Else,
                 TokenKind::While, TokenKind::Print, TokenKind::True, TokenKind::False,
                 end});
    expectKinds("{ x } ;",
                {TokenKind::LeftBrace, TokenKind::Identifier, TokenKind::RightBrace,
                 TokenKind::Semicolon, end});

    const auto lineComment = optiq::tokenize("// note\nlet x = 1;");
    require(lineComment.success(), "line comments tokenize");
    require(lineComment.tokens.size() == 6 &&
                lineComment.tokens[0].kind == TokenKind::Let,
            "// comments are ignored");
    require(lineComment.tokens[0].location.line == 2,
            "line tracking survives // comments");

    const auto literals = optiq::tokenize("123 45.67 'Aisha''s'");
    require(literals.success(), "literal tokenization succeeds");
    require(std::get<std::int64_t>(literals.tokens[0].value) == 123,
            "integer value is converted");
    require(std::get<double>(literals.tokens[1].value) == 45.67,
            "decimal value is converted");
    require(std::get<std::string>(literals.tokens[2].value) == "Aisha's",
            "escaped SQL string is decoded");

    const auto comments = optiq::tokenize("SELECT /* first\nsecond */ name -- trailing\nFROM t");
    require(comments.success() && comments.tokens.size() == 5,
            "line and block comments are ignored");
    require(comments.tokens[1].location.line == 2 && comments.tokens[1].location.column == 11,
            "source line and column survive comments");

    const auto locations = optiq::tokenize("SELECT name\nFROM employees");
    require(locations.tokens[2].kind == TokenKind::From,
            "multiline token stream is complete");
    require(locations.tokens[2].location.line == 2 && locations.tokens[2].location.column == 1,
            "multiline location is accurate");

    const auto invalid = optiq::tokenize("SELECT @");
    require(!invalid.success() && invalid.error->location.column == 8,
            "invalid character has a precise location");
    require(!optiq::tokenize("SELECT 'open").success(),
            "unterminated string is rejected");
    require(!optiq::tokenize("SELECT /* open").success(),
            "unterminated block comment is rejected");
    require(!optiq::tokenize("SELECT 12abc").success(),
            "malformed numeric literal is rejected");
    require(!optiq::tokenize("SELECT 999999999999999999999999").success(),
            "out-of-range integer is rejected");

    std::cout << "PASS: lexer keywords, tokens, literals, comments, and locations\n";
    return 0;
}

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

void expectKinds(const std::string& source, const std::vector<optiq::TokenKind>& expected) {
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
                {TokenKind::IntType, TokenKind::FloatType, TokenKind::BoolType,
                 TokenKind::If, TokenKind::Else, TokenKind::While, TokenKind::Print,
                 TokenKind::True, TokenKind::False, end});
    expectKinds("name _total2 INT Let SELECT",
                {TokenKind::Identifier, TokenKind::Identifier, TokenKind::Identifier,
                 TokenKind::Identifier, TokenKind::Identifier, end});
    expectKinds("( ) { } ; = == != < <= > >= + - * / % && || !",
                {TokenKind::LeftParen, TokenKind::RightParen, TokenKind::LeftBrace,
                 TokenKind::RightBrace, TokenKind::Semicolon, TokenKind::Assign,
                 TokenKind::Equal, TokenKind::NotEqual, TokenKind::Less,
                 TokenKind::LessEqual, TokenKind::Greater, TokenKind::GreaterEqual,
                 TokenKind::Plus, TokenKind::Minus, TokenKind::Star, TokenKind::Slash,
                 TokenKind::Percent, TokenKind::LogicalAnd, TokenKind::LogicalOr,
                 TokenKind::Not, end});

    const auto literals = optiq::tokenize("123 45.67");
    require(literals.success() && literals.tokens.size() == 3,
            "integer and float literals tokenize");
    require(std::get<std::int64_t>(literals.tokens[0].value) == 123,
            "integer value is converted");
    require(std::get<double>(literals.tokens[1].value) == 45.67,
            "float value is converted");

    const auto comments = optiq::tokenize("int x; /* first\nsecond */ // trailing\nprint(x);");
    require(comments.success() && comments.tokens.size() == 9,
            "line and block comments are ignored");
    require(comments.tokens[3].kind == TokenKind::Print &&
                comments.tokens[3].location.line == 3 &&
                comments.tokens[3].location.column == 1,
            "source locations survive multiline comments");

    const auto invalid = optiq::tokenize("int x; @");
    require(!invalid.success() && invalid.error->location.column == 8,
            "invalid character has a precise location");
    require(!optiq::tokenize("int x; /* open").success(),
            "unterminated block comment is rejected");
    require(!optiq::tokenize("int 12abc;").success(),
            "malformed numeric literal is rejected");
    require(!optiq::tokenize("int 999999999999999999999999;").success(),
            "out-of-range integer is rejected");
    require(!optiq::tokenize("print('text');").success(),
            "SQL string literals are outside the required language");

    std::cout << "PASS: manual-language keywords, operators, literals, comments, and locations\n";
    return 0;
}

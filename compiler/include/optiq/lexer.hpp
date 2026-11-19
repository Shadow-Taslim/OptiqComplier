#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "optiq/ast.hpp"

namespace optiq {

enum class TokenKind {
    End,
    Identifier,
    IntegerLiteral,
    DecimalLiteral,
    StringLiteral,
    Select, From, As, Inner, Join, On, Where,
    Group, By, Order, Asc, Desc, Limit,
    And, Or, Not,
    Count, Sum, Average, Minimum, Maximum,
    LeftParen, RightParen, Comma, Semicolon, Dot,
    Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
    Plus, Minus, Star, Slash
};

using TokenValue = std::variant<std::monostate, std::int64_t, double, std::string>;

struct Token {
    TokenKind kind = TokenKind::End;
    std::string lexeme;
    TokenValue value;
    SourceLocation location;
};

struct Diagnostic {
    std::string message;
    SourceLocation location;
};

struct LexResult {
    std::vector<Token> tokens;
    std::optional<Diagnostic> error;

    bool success() const { return !error.has_value(); }
};

LexResult tokenize(const std::string& sql);
std::string tokenKindName(TokenKind kind);

} // namespace optiq

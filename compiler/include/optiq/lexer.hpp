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
    // Keywords (course language: int, float, bool, if, else, while, print,
    // true, false)
    Int, Float, Bool, If, Else, While, Print, True, False,
    // Names and literals
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    // Operators
    Plus, Minus, Star, Slash, Percent,
    Less, Greater, LessEqual, GreaterEqual, EqualEqual, NotEqual,
    AndAnd, OrOr, Bang,
    Assign,
    // Delimiters
    LeftParen, RightParen, LeftBrace, RightBrace, Semicolon
};

using TokenValue = std::variant<std::monostate, std::int64_t, double, bool, std::string>;

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

LexResult tokenize(const std::string& source);
std::string tokenKindName(TokenKind kind);

} // namespace optiq

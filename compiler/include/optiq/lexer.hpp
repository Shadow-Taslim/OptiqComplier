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
    FloatLiteral,
    IntType, FloatType, BoolType,
    If, Else, While, Print, True, False,
    LeftParen, RightParen, LeftBrace, RightBrace, Semicolon,
    Assign, Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
    Plus, Minus, Star, Slash, Percent,
    LogicalAnd, LogicalOr, Not
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

LexResult tokenize(const std::string& source);
std::string tokenKindName(TokenKind kind);

} // namespace optiq

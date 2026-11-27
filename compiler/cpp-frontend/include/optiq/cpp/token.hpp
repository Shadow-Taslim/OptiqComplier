#pragma once

#include <cstddef>
#include <string>

namespace optiq::cpp {

struct SourceLocation {
    std::size_t line = 1;
    std::size_t column = 1;
    std::size_t offset = 0;
};

enum class TokenType {
    Keyword,
    Identifier,
    IntegerLiteral,
    FloatingLiteral,
    CharacterLiteral,
    StringLiteral,
    Preprocessor,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    EqualEqual,
    NotEqual,
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    Assignment,
    PlusAssignment,
    MinusAssignment,
    StarAssignment,
    SlashAssignment,
    Increment,
    Decrement,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    BitwiseNot,
    ShiftLeft,
    ShiftRight,
    Arrow,
    Dot,
    Scope,

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Semicolon,
    Comma,
    Colon
};

struct Token {
    TokenType type = TokenType::Identifier;
    std::string lexeme;
    SourceLocation location;
};

std::string tokenTypeName(TokenType type);

} // namespace optiq::cpp

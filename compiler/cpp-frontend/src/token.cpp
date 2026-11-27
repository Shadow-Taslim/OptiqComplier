#include "optiq/cpp/token.hpp"

namespace optiq::cpp {

std::string tokenTypeName(TokenType type) {
    switch (type) {
    case TokenType::Keyword: return "KEYWORD";
    case TokenType::Identifier: return "IDENTIFIER";
    case TokenType::IntegerLiteral: return "INTEGER_LITERAL";
    case TokenType::FloatingLiteral: return "FLOATING_LITERAL";
    case TokenType::CharacterLiteral: return "CHARACTER_LITERAL";
    case TokenType::StringLiteral: return "STRING_LITERAL";
    case TokenType::Preprocessor: return "PREPROCESSOR";
    case TokenType::Plus: return "PLUS";
    case TokenType::Minus: return "MINUS";
    case TokenType::Star: return "STAR";
    case TokenType::Slash: return "SLASH";
    case TokenType::Percent: return "PERCENT";
    case TokenType::Less: return "LESS";
    case TokenType::Greater: return "GREATER";
    case TokenType::LessEqual: return "LESS_EQUAL";
    case TokenType::GreaterEqual: return "GREATER_EQUAL";
    case TokenType::EqualEqual: return "EQUAL_EQUAL";
    case TokenType::NotEqual: return "NOT_EQUAL";
    case TokenType::LogicalAnd: return "LOGICAL_AND";
    case TokenType::LogicalOr: return "LOGICAL_OR";
    case TokenType::LogicalNot: return "LOGICAL_NOT";
    case TokenType::Assignment: return "ASSIGNMENT";
    case TokenType::PlusAssignment: return "PLUS_ASSIGNMENT";
    case TokenType::MinusAssignment: return "MINUS_ASSIGNMENT";
    case TokenType::StarAssignment: return "STAR_ASSIGNMENT";
    case TokenType::SlashAssignment: return "SLASH_ASSIGNMENT";
    case TokenType::Increment: return "INCREMENT";
    case TokenType::Decrement: return "DECREMENT";
    case TokenType::BitwiseAnd: return "BITWISE_AND";
    case TokenType::BitwiseOr: return "BITWISE_OR";
    case TokenType::BitwiseXor: return "BITWISE_XOR";
    case TokenType::BitwiseNot: return "BITWISE_NOT";
    case TokenType::ShiftLeft: return "SHIFT_LEFT";
    case TokenType::ShiftRight: return "SHIFT_RIGHT";
    case TokenType::Arrow: return "ARROW";
    case TokenType::Dot: return "DOT";
    case TokenType::Scope: return "SCOPE";
    case TokenType::LeftParen: return "LEFT_PAREN";
    case TokenType::RightParen: return "RIGHT_PAREN";
    case TokenType::LeftBrace: return "LEFT_BRACE";
    case TokenType::RightBrace: return "RIGHT_BRACE";
    case TokenType::LeftBracket: return "LEFT_BRACKET";
    case TokenType::RightBracket: return "RIGHT_BRACKET";
    case TokenType::Semicolon: return "SEMICOLON";
    case TokenType::Comma: return "COMMA";
    case TokenType::Colon: return "COLON";
    }
    return "UNKNOWN";
}

} // namespace optiq::cpp

#include "scanner_state.hpp"

#include <charconv>
#include <cmath>
#include <string>
#include <unordered_map>

namespace optiq {

void ScannerState::advance(const char* text, std::size_t length) {
    current = next;
    for (std::size_t i = 0; i < length; ++i) {
        ++next.offset;
        if (text[i] == '\n') {
            ++next.line;
            next.column = 1;
        } else {
            ++next.column;
        }
    }
}

void ScannerState::add(TokenKind kind, const char* text, std::size_t length) {
    if (result.error) return;
    result.tokens.push_back(Token{kind, std::string(text, length), std::monostate{}, current});
}

void ScannerState::addIdentifier(const char* text, std::size_t length) {
    if (result.error) return;
    std::string value(text, length);
    result.tokens.push_back(Token{TokenKind::Identifier, value, value, current});
}

void ScannerState::addKeyword(const char* text) {
    static const std::unordered_map<std::string, TokenKind> keywords = {
        {"int", TokenKind::IntType}, {"float", TokenKind::FloatType},
        {"bool", TokenKind::BoolType}, {"if", TokenKind::If},
        {"else", TokenKind::Else}, {"while", TokenKind::While},
        {"print", TokenKind::Print}, {"true", TokenKind::True},
        {"false", TokenKind::False}
    };

    const std::string word(text);
    const auto found = keywords.find(word);
    if (found == keywords.end()) {
        addIdentifier(text, word.size());
    } else {
        add(found->second, text, word.size());
    }
}

void ScannerState::addInteger(const char* text) {
    std::int64_t value = 0;
    const char* end = text + std::char_traits<char>::length(text);
    const auto conversion = std::from_chars(text, end, value);
    if (conversion.ec != std::errc{} || conversion.ptr != end) {
        fail("Integer literal is out of range: " + std::string(text));
        return;
    }
    result.tokens.push_back(Token{TokenKind::IntegerLiteral, text, value, current});
}

void ScannerState::addFloat(const char* text) {
    try {
        std::size_t consumed = 0;
        const std::string lexeme(text);
        const double value = std::stod(lexeme, &consumed);
        if (consumed != lexeme.size() || !std::isfinite(value)) {
            fail("Invalid float literal: " + lexeme);
            return;
        }
        result.tokens.push_back(Token{TokenKind::FloatLiteral, lexeme, value, current});
    } catch (const std::exception&) {
        fail("Invalid or out-of-range float literal: " + std::string(text));
    }
}

void ScannerState::fail(const std::string& message) {
    fail(message, current);
}

void ScannerState::fail(const std::string& message, SourceLocation location) {
    if (!result.error) result.error = Diagnostic{message, location};
}

} // namespace optiq

#include "scanner_state.hpp"

#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
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
        {"SELECT", TokenKind::Select}, {"FROM", TokenKind::From},
        {"AS", TokenKind::As}, {"INNER", TokenKind::Inner},
        {"JOIN", TokenKind::Join}, {"ON", TokenKind::On},
        {"WHERE", TokenKind::Where}, {"GROUP", TokenKind::Group},
        {"BY", TokenKind::By}, {"ORDER", TokenKind::Order},
        {"ASC", TokenKind::Asc}, {"DESC", TokenKind::Desc},
        {"LIMIT", TokenKind::Limit}, {"AND", TokenKind::And},
        {"OR", TokenKind::Or}, {"NOT", TokenKind::Not},
        {"COUNT", TokenKind::Count}, {"SUM", TokenKind::Sum},
        {"AVG", TokenKind::Average}, {"MIN", TokenKind::Minimum},
        {"MAX", TokenKind::Maximum}
    };

    std::string keyword(text);
    for (char& ch : keyword) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    const auto found = keywords.find(keyword);
    if (found == keywords.end()) {
        addIdentifier(text, keyword.size());
    } else {
        add(found->second, text, keyword.size());
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

void ScannerState::addDecimal(const char* text) {
    try {
        std::size_t consumed = 0;
        const std::string lexeme(text);
        const double value = std::stod(lexeme, &consumed);
        if (consumed != lexeme.size() || !std::isfinite(value)) {
            fail("Invalid decimal literal: " + lexeme);
            return;
        }
        result.tokens.push_back(Token{TokenKind::DecimalLiteral, lexeme, value, current});
    } catch (const std::exception&) {
        fail("Invalid or out-of-range decimal literal: " + std::string(text));
    }
}

void ScannerState::addString(const char* text, std::size_t length) {
    std::string value;
    value.reserve(length - 2);
    for (std::size_t i = 1; i + 1 < length; ++i) {
        if (text[i] == '\'' && i + 2 < length && text[i + 1] == '\'') {
            value.push_back('\'');
            ++i;
        } else {
            value.push_back(text[i]);
        }
    }
    result.tokens.push_back(Token{TokenKind::StringLiteral, std::string(text, length),
                                  std::move(value), current});
}

void ScannerState::fail(const std::string& message) {
    fail(message, current);
}

void ScannerState::fail(const std::string& message, SourceLocation location) {
    if (!result.error) result.error = Diagnostic{message, location};
}

} // namespace optiq

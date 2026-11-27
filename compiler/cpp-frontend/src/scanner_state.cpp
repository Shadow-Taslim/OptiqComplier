#include "scanner_state.hpp"

#include <cctype>
#include <unordered_set>
#include <utility>

namespace optiq::cpp {
namespace {

bool isIdentifierStart(char ch) {
    const auto value = static_cast<unsigned char>(ch);
    return std::isalpha(value) != 0 || ch == '_';
}

bool isIdentifierPart(char ch) {
    const auto value = static_cast<unsigned char>(ch);
    return std::isalnum(value) != 0 || ch == '_';
}

void skipSpace(const std::string& text, std::size_t& position) {
    while (position < text.size() &&
           std::isspace(static_cast<unsigned char>(text[position])) != 0) {
        ++position;
    }
}

std::string trimRight(std::string text) {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.pop_back();
    }
    return text;
}

} // namespace

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

void ScannerState::addAt(TokenType type, const std::string& lexeme,
                         SourceLocation location) {
    result.tokens.push_back(Token{type, lexeme, location});
}

void ScannerState::add(TokenType type, const char* text, std::size_t length) {
    addAt(type, std::string(text, length), current);
}

void ScannerState::addIdentifier(const char* text, std::size_t length) {
    static const std::unordered_set<std::string> keywords = {
        "int", "float", "double", "char", "bool", "void",
        "if", "else", "while", "for", "do", "return",
        "break", "continue", "true", "false"
    };

    const std::string word(text, length);
    addAt(keywords.count(word) == 1 ? TokenType::Keyword : TokenType::Identifier,
          word, current);
}

void ScannerState::addInteger(const char* text, std::size_t length) {
    add(TokenType::IntegerLiteral, text, length);
}

void ScannerState::addFloating(const char* text, std::size_t length) {
    add(TokenType::FloatingLiteral, text, length);
}

void ScannerState::addPreprocessor(const char* text, std::size_t length) {
    std::string line(text, length);
    const std::size_t hash = line.find('#');
    if (hash == std::string::npos) {
        fail("Malformed preprocessor directive");
        return;
    }

    SourceLocation location = current;
    location.column += hash;
    location.offset += hash;
    const std::string lexeme = trimRight(line.substr(hash));

    std::size_t position = 1;
    skipSpace(lexeme, position);
    const std::size_t directiveStart = position;
    while (position < lexeme.size() && isIdentifierPart(lexeme[position])) {
        ++position;
    }
    const std::string directive = lexeme.substr(directiveStart, position - directiveStart);
    skipSpace(lexeme, position);
    const std::string arguments = trimRight(lexeme.substr(position));

    if (directive == "include") {
        const bool angleHeader = arguments.size() >= 3 && arguments.front() == '<' &&
                                 arguments.back() == '>';
        const bool quotedHeader = arguments.size() >= 3 && arguments.front() == '"' &&
                                  arguments.back() == '"';
        if (!angleHeader && !quotedHeader) {
            failAt("Malformed #include directive; expected <header> or a quoted header",
                   location);
            return;
        }
    } else if (directive == "define") {
        if (arguments.empty() || !isIdentifierStart(arguments.front())) {
            failAt("Malformed #define directive; expected an identifier after #define",
                   location);
            return;
        }
        std::size_t nameEnd = 1;
        while (nameEnd < arguments.size() && isIdentifierPart(arguments[nameEnd])) {
            ++nameEnd;
        }
        if (nameEnd < arguments.size() &&
            std::isspace(static_cast<unsigned char>(arguments[nameEnd])) == 0) {
            failAt("Malformed #define directive; expected whitespace after the macro name",
                   location);
            return;
        }
    } else {
        failAt("Unsupported preprocessor directive: #" +
                   (directive.empty() ? std::string("<missing>") : directive),
               location);
        return;
    }

    addAt(TokenType::Preprocessor, lexeme, location);
}

void ScannerState::beginBlockComment() {
    blockCommentStart = current;
}

void ScannerState::failUnterminatedBlockComment() {
    failAt("Unterminated block comment", blockCommentStart);
}

void ScannerState::beginLiteral(const char* text, std::size_t length, char delimiter) {
    literalStart = current;
    literalText.assign(text, length);
    literalUnits = 0;
    literalDelimiter = delimiter;
    literalValid = true;
}

void ScannerState::appendLiteral(const char* text, std::size_t length,
                                 std::size_t units) {
    literalText.append(text, length);
    literalUnits += units;
}

void ScannerState::invalidEscape(const char* text, std::size_t length) {
    literalValid = false;
    failAt("Invalid escape sequence: " + std::string(text, length), current);
}

void ScannerState::finishLiteral() {
    literalText.push_back(literalDelimiter);
    if (!literalValid) return;

    if (literalDelimiter == '"') {
        addAt(TokenType::StringLiteral, literalText, literalStart);
        return;
    }

    if (literalUnits == 0) {
        failAt("Empty character literal", literalStart);
    } else if (literalUnits != 1) {
        failAt("Character literal must contain exactly one character or escape sequence",
               literalStart);
    } else {
        addAt(TokenType::CharacterLiteral, literalText, literalStart);
    }
}

void ScannerState::failUnterminatedLiteral() {
    literalValid = false;
    failAt(literalDelimiter == '"' ? "Unterminated string literal"
                                   : "Unterminated character literal",
           literalStart);
}

void ScannerState::fail(const std::string& message) {
    failAt(message, current);
}

void ScannerState::failAt(const std::string& message, SourceLocation location) {
    result.diagnostics.push_back(Diagnostic{message, location.line, location.column});
}

} // namespace optiq::cpp

#pragma once

#include <cstddef>
#include <string>

#include "optiq/cpp/lexer.hpp"

namespace optiq::cpp {

class ScannerState {
public:
    void advance(const char* text, std::size_t length);
    void add(TokenType type, const char* text, std::size_t length);
    void addIdentifier(const char* text, std::size_t length);
    void addInteger(const char* text, std::size_t length);
    void addFloating(const char* text, std::size_t length);
    void addPreprocessor(const char* text, std::size_t length);

    void beginBlockComment();
    void failUnterminatedBlockComment();

    void beginLiteral(const char* text, std::size_t length, char delimiter);
    void appendLiteral(const char* text, std::size_t length, std::size_t units = 0);
    void invalidEscape(const char* text, std::size_t length);
    void finishLiteral();
    void failUnterminatedLiteral();

    void fail(const std::string& message);
    void failAt(const std::string& message, SourceLocation location);

    LexResult result;
    SourceLocation next{1, 1, 0};
    SourceLocation current{1, 1, 0};

private:
    void addAt(TokenType type, const std::string& lexeme, SourceLocation location);

    SourceLocation blockCommentStart{1, 1, 0};
    SourceLocation literalStart{1, 1, 0};
    std::string literalText;
    std::size_t literalUnits = 0;
    char literalDelimiter = 0;
    bool literalValid = true;
};

} // namespace optiq::cpp

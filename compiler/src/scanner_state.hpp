#pragma once

#include <cstddef>
#include <string>

#include "optiq/lexer.hpp"

namespace optiq {

class ScannerState {
public:
    void advance(const char* text, std::size_t length);
    void add(TokenKind kind, const char* text, std::size_t length);
    void addIdentifier(const char* text, std::size_t length);
    void addKeyword(const char* text);
    void addInteger(const char* text);
    void addFloat(const char* text);
    void fail(const std::string& message);
    void fail(const std::string& message, SourceLocation location);

    LexResult result;
    SourceLocation next{1, 1, 0};
    SourceLocation current{1, 1, 0};
    SourceLocation blockCommentStart{1, 1, 0};
};

} // namespace optiq

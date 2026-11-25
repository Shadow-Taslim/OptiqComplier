#pragma once

#include <cstddef>
#include <vector>

#include "optiq/lexer.hpp"

namespace optiq {

// Feeds the pre-tokenized input to the Bison parser one token at a time.
class TokenStream {
public:
    explicit TokenStream(const std::vector<Token>& tokens) : tokens(tokens) {}

    const Token& next() {
        const Token& token = tokens[index];
        if (token.kind != TokenKind::End) ++index;
        return token;
    }

private:
    const std::vector<Token>& tokens;
    std::size_t index = 0;
};

} // namespace optiq

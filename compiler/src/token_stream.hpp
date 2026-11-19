#pragma once

#include <cstddef>
#include <vector>

#include "optiq/lexer.hpp"

namespace optiq {

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

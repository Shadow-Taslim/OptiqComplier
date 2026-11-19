#pragma once

#include <memory>
#include <optional>
#include <string>

#include "optiq/ast.hpp"
#include "optiq/lexer.hpp"

namespace optiq {

struct ParseResult {
    std::unique_ptr<SelectStatement> statement;
    std::optional<Diagnostic> error;

    bool success() const { return statement != nullptr && !error.has_value(); }
};

ParseResult parseSql(const std::string& sql);

} // namespace optiq

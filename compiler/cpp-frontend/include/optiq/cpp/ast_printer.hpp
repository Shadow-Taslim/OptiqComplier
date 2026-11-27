#pragma once

#include <iosfwd>

#include "optiq/cpp/ast.hpp"

namespace optiq::cpp {

// Writes a stable, indented tree for diagnostics, tests, and manual inspection.
void printAst(const Node& root, std::ostream& output);

} // namespace optiq::cpp

#pragma once

#include <ostream>
#include <string>

#include "optiq/ast.hpp"

namespace optiq {

// Indented, human-readable AST printing (used by the CLI and the report).
void writeProgramText(std::ostream& output, const Program& program);
std::string printProgramText(const Program& program);

// Deterministic JSON printing (used by automated tests).
void writeProgramJson(std::ostream& output, const Program& program);
std::string printProgramJson(const Program& program);

} // namespace optiq

#pragma once

#include <iosfwd>
#include <string>

#include "optiq/ast.hpp"

namespace optiq {

void writeProgramJson(std::ostream& output, const Program& program);
std::string printProgramJson(const Program& program);

} // namespace optiq

#pragma once

#include <string>
#include <vector>

#include "optiq/ast.hpp"

namespace optiq {

// Generates Three Address Code (TAC) for a semantically valid program.
// Conventions: temporaries t1, t2, ...; labels L1, L2, ...; control flow via
// `ifFalse <cond> goto <label>` and `goto <label>`; output via `print <x>`.
struct TacProgram {
    std::vector<std::string> instructions;
};

TacProgram generateTac(const Program& program);
std::string tacToString(const TacProgram& tac);

} // namespace optiq

#pragma once

#include <cstddef>
#include <string>

namespace optiq::cpp {

struct Diagnostic {
    std::string message;
    std::size_t line = 1;
    std::size_t column = 1;
};

} // namespace optiq::cpp

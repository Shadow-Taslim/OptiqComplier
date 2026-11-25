#include "optiq/ast.hpp"

#include <stdexcept>

namespace optiq {

std::string dataTypeName(DataType type) {
    switch (type) {
    case DataType::Int: return "int";
    case DataType::Float: return "float";
    case DataType::Bool: return "bool";
    }
    throw std::logic_error("Unknown data type");
}

} // namespace optiq

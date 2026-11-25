#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include "optiq/ast.hpp"
#include "optiq/ast_printer.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace optiq;

    Program program;
    program.statements.push_back(std::make_unique<DeclarationStatement>(
        SourceLocation{1, 1, 0}, DataType::Int, "x"));

    auto value = std::make_unique<BinaryExpression>(
        SourceLocation{2, 5, 4}, BinaryExpression::Operator::Add,
        std::make_unique<VariableExpression>(SourceLocation{2, 5, 4}, "x"),
        std::make_unique<LiteralExpression>(SourceLocation{2, 9, 8}, std::int64_t{1}));
    program.statements.push_back(std::make_unique<AssignmentStatement>(
        SourceLocation{2, 1, 0}, "x", std::move(value)));
    program.statements.push_back(std::make_unique<PrintStatement>(
        SourceLocation{3, 1, 0},
        std::make_unique<LiteralExpression>(SourceLocation{3, 7, 0}, true)));

    const std::string json = printProgramJson(program);
    require(json.find("\"type\": \"Program\"") != std::string::npos,
            "printer emits the program root");
    require(json.find("\"type\": \"DeclarationStatement\"") != std::string::npos,
            "printer emits typed declarations");
    require(json.find("\"dataType\": \"int\"") != std::string::npos,
            "printer emits the declaration type");
    require(json.find("\"operator\": \"+\"") != std::string::npos,
            "printer traverses binary expressions");
    require(json.find("\"value\": true") != std::string::npos,
            "printer emits boolean literals");
    require(json.find("\"line\": 2") != std::string::npos &&
                json.find("\"column\": 5") != std::string::npos,
            "printer retains source locations");
    require(dataTypeName(DataType::Float) == "float" &&
                dataTypeName(DataType::Bool) == "bool",
            "all manual data types have readable names");

    std::cout << "PASS: typed AST ownership, traversal, locations, and JSON printing\n";
    return 0;
}

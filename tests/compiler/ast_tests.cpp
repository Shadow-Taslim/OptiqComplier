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
        SourceLocation{}, ValueType::Int, "x",
        std::make_unique<LiteralExpression>(SourceLocation{}, std::int64_t{5})));
    program.statements.push_back(std::make_unique<DeclarationStatement>(
        SourceLocation{}, ValueType::Bool, "flag",
        std::make_unique<LiteralExpression>(SourceLocation{}, true)));
    program.statements.push_back(std::make_unique<AssignmentStatement>(
        SourceLocation{}, "x",
        std::make_unique<BinaryExpression>(
            SourceLocation{}, BinaryExpression::Operator::Add,
            std::make_unique<IdentifierExpression>(SourceLocation{}, "x"),
            std::make_unique<LiteralExpression>(SourceLocation{}, 2.5))));

    auto thenBlock = std::make_unique<BlockStatement>(SourceLocation{});
    thenBlock->statements.push_back(std::make_unique<PrintStatement>(
        SourceLocation{},
        std::make_unique<IdentifierExpression>(SourceLocation{}, "x")));
    program.statements.push_back(std::make_unique<IfStatement>(
        SourceLocation{},
        std::make_unique<BinaryExpression>(
            SourceLocation{}, BinaryExpression::Operator::Greater,
            std::make_unique<IdentifierExpression>(SourceLocation{}, "x"),
            std::make_unique<LiteralExpression>(SourceLocation{}, std::int64_t{0})),
        std::move(thenBlock)));

    const std::string json = printProgramJson(program);
    require(json.find("\"type\": \"Program\"") != std::string::npos,
            "printer emits the program root");
    require(json.find("\"type\": \"DeclarationStatement\"") != std::string::npos &&
                json.find("\"dataType\": \"int\"") != std::string::npos,
            "printer emits typed declarations");
    require(json.find("\"kind\": \"boolean\"") != std::string::npos &&
                json.find("\"value\": true") != std::string::npos,
            "printer emits boolean literals");
    require(json.find("\"type\": \"BinaryExpression\", \"operator\": \"+\"") !=
                std::string::npos,
            "printer emits binary operators");
    require(json.find("\"type\": \"IfStatement\"") != std::string::npos,
            "printer emits if statements");

    const std::string text = printProgramText(program);
    require(text.find("Program\n") == 0, "text printer starts at the root");
    require(text.find("Declaration int x =") != std::string::npos,
            "text printer emits declarations");
    require(text.find("  Binary +") != std::string::npos,
            "text printer indents child expressions");
    require(text.find("Literal 2.5") != std::string::npos,
            "text printer emits float literals");
    require(text.find("If") != std::string::npos &&
                text.find("condition:") != std::string::npos,
            "text printer emits if structure");

    std::cout << "PASS: AST ownership, structure, and text/JSON printing\n";
    return 0;
}

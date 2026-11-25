#include <cstdlib>
#include <iostream>
#include <string>

#include "optiq/ast.hpp"
#include "optiq/ast_printer.hpp"
#include "optiq/parser.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

optiq::ParseResult valid(const std::string& source) {
    auto result = optiq::parseProgram(source);
    if (!result.success()) {
        std::cerr << "Unexpected parse error for " << source << '\n';
    }
    require(result.success(), "valid program parses: " + source);
    return result;
}

void invalid(const std::string& source) {
    const auto result = optiq::parseProgram(source);
    require(!result.success() && !result.errors.empty(),
            "invalid program is rejected: " + source);
}

} // namespace

int main() {
    using namespace optiq;

    auto empty = parseProgram("// only a comment\n");
    require(empty.success() && empty.program->statements.empty(),
            "an empty program parses to zero statements");

    auto declarations = valid("int count; float ratio; bool ready;");
    require(declarations.program->statements.size() == 3, "all declarations parse");
    const auto& ratio = static_cast<const DeclarationStatement&>(
        *declarations.program->statements[1]);
    require(ratio.declaredType == DataType::Float && ratio.name == "ratio",
            "declaration retains its type and name");

    auto branch = valid(
        "if (count > 3) { print(count); }"
        " else if (count > 0) { print(1); } else { }");
    const auto& conditional = static_cast<const IfStatement&>(*branch.program->statements[0]);
    require(conditional.hasElse && conditional.elseBlock.size() == 1,
            "else-if occupies the else block");
    require(conditional.elseBlock[0]->kind == Statement::Kind::If,
            "else-if is represented as a nested if");

    auto loop = valid("while (count <= 4) { { int inner; } count = count + 1; }");
    const auto& whileNode = static_cast<const WhileStatement&>(*loop.program->statements[0]);
    require(whileNode.body.size() == 2 &&
                whileNode.body[0]->kind == Statement::Kind::Block &&
                whileNode.body[1]->kind == Statement::Kind::Assignment,
            "nested blocks and assignments retain their order");

    const std::string json = printProgramJson(*valid("bool ok; ok = true; print(ok);").program);
    require(json.find("DeclarationStatement") != std::string::npos &&
                json.find("AssignmentStatement") != std::string::npos &&
                json.find("PrintStatement") != std::string::npos,
            "the complete program AST can be printed");

    invalid("int x");
    invalid("int x = 1;");
    invalid("let x = 1;");
    invalid("x = ;");
    invalid("if (true) { print(1);");
    invalid("while x > 1 { }");
    invalid("print 1;");
    invalid("print('text');");
    invalid("SELECT name FROM employees;");
    invalid("else { print(1); }");

    auto recovered = parseProgram("x = ;\nprint(1);\ny = ;\nprint(2);");
    require(!recovered.success() && recovered.errors.size() >= 2,
            "error recovery reports multiple independent syntax errors");
    require(recovered.program != nullptr && recovered.program->statements.size() == 2,
            "valid statements after syntax errors are retained: " +
                std::to_string(recovered.program ? recovered.program->statements.size() : 999));

    std::cout << "PASS: statements, nested blocks, control flow, and syntax recovery\n";
    return 0;
}

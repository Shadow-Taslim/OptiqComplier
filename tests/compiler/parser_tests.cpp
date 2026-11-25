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
    for (const auto& error : result.errors) {
        std::cerr << "Unexpected parse error for " << source << ": " << error.message
                  << '\n';
    }
    require(result.success(), "valid program parses: " + source);
    return result;
}

void invalid(const std::string& source) {
    const auto result = optiq::parseProgram(source);
    require(!result.success(), "invalid program is rejected: " + source);
}

const optiq::Statement& onlyStatement(const optiq::ParseResult& result) {
    require(result.program->statements.size() == 1, "program has exactly one statement");
    return *result.program->statements[0];
}

const optiq::BinaryExpression& binary(const optiq::Expression& expression) {
    require(expression.kind == optiq::Expression::Kind::Binary, "expression is binary");
    return static_cast<const optiq::BinaryExpression&>(expression);
}

} // namespace

int main() {
    using namespace optiq;

    {
        auto result = valid("int x; float y = 1.5; bool flag = true;");
        require(result.program->statements.size() == 3, "three declarations parse");
        const auto& first =
            static_cast<const DeclarationStatement&>(*result.program->statements[0]);
        require(first.type == ValueType::Int && first.name == "x" &&
                    first.initializer == nullptr,
                "plain declaration keeps type, name, and no initializer");
        const auto& second =
            static_cast<const DeclarationStatement&>(*result.program->statements[1]);
        require(second.type == ValueType::Float && second.initializer != nullptr,
                "initialized float declaration parses");
        const auto& third =
            static_cast<const DeclarationStatement&>(*result.program->statements[2]);
        const auto& literal =
            static_cast<const LiteralExpression&>(*third.initializer);
        require(literal.literalKind == LiteralExpression::LiteralKind::Boolean &&
                    std::get<bool>(literal.value),
                "true parses as a boolean literal");
    }

    {
        auto result = valid("x = 2 + 3 * 4 - 10 / 5 % 2;");
        const auto& assign =
            static_cast<const AssignmentStatement&>(onlyStatement(result));
        const auto& sum = binary(*assign.value);
        require(sum.op == BinaryExpression::Operator::Subtract, "subtraction is root");
        require(binary(*sum.left).op == BinaryExpression::Operator::Add,
                "addition groups left of subtraction");
        const auto& product = binary(*binary(*sum.left).right);
        require(product.op == BinaryExpression::Operator::Multiply,
                "multiplication binds tighter than addition");
        const auto& divMod = binary(*sum.right);
        require(divMod.op == BinaryExpression::Operator::Modulo,
                "modulo is the outer of / and %");
        require(binary(*divMod.left).op == BinaryExpression::Operator::Divide,
                "division groups left of modulo (left-associative)");
    }

    {
        // Precedence ladder: || < && < ==/!= < relational < additive.
        auto result = valid("flag = a < 1 + 2 == b || c && !d;");
        const auto& assign =
            static_cast<const AssignmentStatement&>(onlyStatement(result));
        const auto& root = binary(*assign.value);
        require(root.op == BinaryExpression::Operator::Or, "|| is the root");
        require(binary(*root.right).op == BinaryExpression::Operator::And,
                "&& binds tighter than ||");
        const auto& equality = binary(*root.left);
        require(equality.op == BinaryExpression::Operator::Equal,
                "== binds tighter than &&");
        require(binary(*equality.left).op == BinaryExpression::Operator::Less,
                "< binds tighter than ==");
        require(binary(*binary(*equality.left).right).op ==
                    BinaryExpression::Operator::Add,
                "+ binds tighter than <");
        require(binary(*root.right).right->kind == Expression::Kind::Unary,
                "! applies to its operand");
    }

    {
        auto result = valid("if (x > 0) { y = 1; } else { y = 2; }");
        const auto& branch = static_cast<const IfStatement&>(onlyStatement(result));
        require(branch.thenBlock->statements.size() == 1, "then block parses");
        require(branch.elseBlock != nullptr &&
                    branch.elseBlock->statements.size() == 1,
                "else block parses");
    }

    {
        auto result = valid("while (x > 0) { if (x == 3) { print x; } x = x - 1; }");
        const auto& loop = static_cast<const WhileStatement&>(onlyStatement(result));
        require(loop.body->statements.size() == 2, "loop body holds two statements");
        require(loop.body->statements[0]->kind == Statement::Kind::If,
                "nested if inside while block");
        require(loop.body->statements[1]->kind == Statement::Kind::Assignment,
                "nested assignment inside while block");
    }

    {
        auto result = valid("{ int x = 1; { x = x + 1; } print x; }");
        const auto& block = static_cast<const BlockStatement&>(onlyStatement(result));
        require(block.statements.size() == 3, "nested block statements parse");
        require(block.statements[1]->kind == Statement::Kind::Block,
                "inner block is a statement");
    }

    {
        const std::string json =
            printProgramJson(*valid("int x = 1; print x;").program);
        require(json.find("\"type\": \"Program\"") != std::string::npos,
                "printer emits the program root");
        require(json.find("\"type\": \"DeclarationStatement\"") != std::string::npos &&
                    json.find("\"type\": \"PrintStatement\"") != std::string::npos,
                "printer emits statement nodes");
    }

    // Invalid programs are rejected.
    invalid("int x = ;");
    invalid("x = 5");
    invalid("if (x > 1) { x = 1;");
    invalid("while x > 1 { x = 1; }");
    invalid("int 3x;");
    invalid("print ;");
    invalid("else { x = 1; }");
    invalid("x = 1 2;");

    // Error recovery: both bad statements are reported in one run.
    {
        const auto result = parseProgram("int x = ;\nfloat y = ;\nint ok;");
        require(result.errors.size() == 2,
                "error recovery reports multiple syntax errors");
        require(result.errors[1].location.line == 2,
                "the second error carries its own line number");
    }

    // Errors carry locations.
    {
        const auto result = parseProgram("int x;\nx = ;");
        require(result.errors.size() == 1, "one syntax error is reported");
        require(result.errors[0].location.line == 2,
                "syntax errors carry the source line");
    }

    std::cout << "PASS: declarations, statements, precedence, blocks, error recovery\n";
    return 0;
}

#include <cstdlib>
#include <iostream>
#include <string>

#include "optiq/ast.hpp"
#include "optiq/parser.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

optiq::ParseResult validExpression(const std::string& source) {
    auto result = optiq::parseProgram("print(" + source + ");");
    if (!result.success()) {
        std::cerr << "Unexpected parse error for " << source << '\n';
    }
    require(result.success(), "valid expression parses: " + source);
    return result;
}

const optiq::Expression& printedExpression(const optiq::ParseResult& result) {
    const auto& print = static_cast<const optiq::PrintStatement&>(*result.program->statements[0]);
    return *print.expression;
}

const optiq::BinaryExpression& binary(const optiq::Expression& expression) {
    require(expression.kind == optiq::Expression::Kind::Binary, "expression is binary");
    return static_cast<const optiq::BinaryExpression&>(expression);
}

} // namespace

int main() {
    using namespace optiq;

    const auto arithmetic = validExpression("1 + 2 * 6 % 4");
    const auto& sum = binary(printedExpression(arithmetic));
    require(sum.op == BinaryExpression::Operator::Add, "addition is the root operation");
    require(binary(*sum.right).op == BinaryExpression::Operator::Modulo,
            "modulo shares multiplicative precedence");
    require(binary(*binary(*sum.right).left).op == BinaryExpression::Operator::Multiply,
            "multiplication and modulo are left associative");

    const auto logical = validExpression("!ready || x < 3 && done");
    const auto& disjunction = binary(printedExpression(logical));
    require(disjunction.op == BinaryExpression::Operator::Or, "|| has the lowest precedence");
    require(binary(*disjunction.right).op == BinaryExpression::Operator::And,
            "&& binds more tightly than ||");
    require(disjunction.left->kind == Expression::Kind::Unary,
            "logical negation produces a unary node");

    auto unaryResult = validExpression("-(x + 1) * 2");
    const auto& unary = printedExpression(unaryResult);
    require(binary(unary).left->kind == Expression::Kind::Unary,
            "unary minus binds to the parenthesized expression");
    auto literalResult = validExpression("3.25");
    const auto& literal = printedExpression(literalResult);
    require(literal.kind == Expression::Kind::Literal &&
                static_cast<const LiteralExpression&>(literal).type == DataType::Float,
            "float literal type is retained");

    require(!optiq::parseProgram("print(a < b < c);").success(),
            "comparison chaining is rejected by the non-associative grammar");
    require(!optiq::parseProgram("print(a = b);").success(),
            "assignment cannot be used as an expression");

    auto located = optiq::parseProgram("int x;\nprint(x)");
    require(!located.success() && !located.errors.empty() &&
                located.errors[0].location.line == 2,
            "syntax diagnostics include the source line");

    std::cout << "PASS: expression grammar, precedence, associativity, and diagnostics\n";
    return 0;
}

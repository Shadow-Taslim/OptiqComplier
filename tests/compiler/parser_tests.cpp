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

optiq::ParseResult valid(const std::string& sql) {
    auto result = optiq::parseSql(sql);
    if (result.error) {
        std::cerr << "Unexpected parse error for " << sql << ": "
                  << result.error->message << '\n';
    }
    require(result.success(), "valid SQL parses: " + sql);
    return result;
}

void invalid(const std::string& sql) {
    const auto result = optiq::parseSql(sql);
    require(!result.success() && result.error.has_value(), "invalid SQL is rejected: " + sql);
}

const optiq::BinaryExpression& binary(const optiq::Expression& expression) {
    require(expression.kind == optiq::Expression::Kind::Binary, "expression is binary");
    return static_cast<const optiq::BinaryExpression&>(expression);
}

} // namespace

int main() {
    using namespace optiq;

    auto arithmetic = valid("SELECT 1 + 2 * 3 FROM employees;");
    const auto& sum = binary(*arithmetic.statement->items[0].expression);
    require(sum.op == BinaryExpression::Operator::Add, "addition is the root operation");
    require(binary(*sum.right).op == BinaryExpression::Operator::Multiply,
            "multiplication binds more tightly than addition");

    auto boolean = valid("SELECT id FROM employees WHERE NOT a = 1 AND b = 2 OR c = 3;");
    const auto& disjunction = binary(*boolean.statement->where->predicate);
    require(disjunction.op == BinaryExpression::Operator::Or, "OR has the lowest precedence");
    require(binary(*disjunction.left).op == BinaryExpression::Operator::And,
            "AND binds more tightly than OR");
    const auto& andExpression = binary(*disjunction.left);
    require(andExpression.left->kind == Expression::Kind::Unary,
            "NOT produces a unary expression");
    const auto& negation = static_cast<const UnaryExpression&>(*andExpression.left);
    require(binary(*negation.operand).op == BinaryExpression::Operator::Equal,
            "NOT applies to the comparison");

    auto full = valid(
        "SELECT e.name AS employee_name, COUNT(*) AS total "
        "FROM employees e INNER JOIN departments d ON e.dept_id = d.id "
        "WHERE e.salary >= 50000 + 20000 "
        "GROUP BY e.name ORDER BY e.name DESC LIMIT 5;");
    require(full.statement->items.size() == 2, "select list contains both items");
    require(full.statement->items[0].alias == std::optional<std::string>("employee_name"),
            "select alias is retained");
    require(full.statement->from.alias == std::optional<std::string>("e"),
            "table alias is retained");
    require(full.statement->join.has_value(), "INNER JOIN is represented");
    require(full.statement->groupBy.has_value(), "GROUP BY is represented");
    require(full.statement->orderBy->items[0].direction == OrderDirection::Descending,
            "DESC ordering is represented");
    require(full.statement->limit->count == 5, "LIMIT is represented");
    const auto& count = static_cast<const FunctionCall&>(
        *full.statement->items[1].expression);
    require(count.function == FunctionCall::Function::Count && count.starArgument,
            "COUNT(*) is parsed as an aggregate");

    auto plainJoin = valid("SELECT * FROM employees e JOIN departments d ON e.dept_id=d.id");
    require(plainJoin.statement->join.has_value(), "JOIN without INNER is accepted");
    require(plainJoin.statement->items[0].isWildcard,
            "wildcard selection is represented");

    auto aggregates = valid("SELECT SUM(salary), AVG(salary), MIN(salary), MAX(salary) FROM employees");
    require(aggregates.statement->items.size() == 4, "all aggregate functions parse");
    require(printAstJson(*aggregates.statement).find("SelectStatement") != std::string::npos,
            "parsed AST can be printed");

    invalid("");
    invalid("SELECT FROM employees");
    invalid("SELECT name employees");
    invalid("SELECT name FROM");
    invalid("SELECT name FROM employees WHERE");
    invalid("SELECT name FROM employees LIMIT");
    invalid("SELECT name FROM employees; SELECT * FROM departments");
    invalid("SELECT name FROM employees ORDER salary");
    invalid("SELECT SUM(*) FROM employees");
    invalid("SELECT name FROM employees @");

    auto locatedError = parseSql("SELECT name\nemployees");
    require(locatedError.error.has_value(), "missing FROM reports an error");
    require(locatedError.error->location.line == 2,
            "syntax error includes the source line");

    std::cout << "PASS: parser grammar, precedence, AST structure, and diagnostics\n";
    return 0;
}

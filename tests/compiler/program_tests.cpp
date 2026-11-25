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

optiq::ProgramParseResult valid(const std::string& source) {
    auto result = optiq::parseProgram(source);
    if (result.error) {
        std::cerr << "Unexpected parse error for " << source << ": "
                  << result.error->message << '\n';
    }
    require(result.success(), "valid program parses: " + source);
    return result;
}

void invalid(const std::string& source) {
    const auto result = optiq::parseProgram(source);
    require(!result.success() && result.error.has_value(),
            "invalid program is rejected: " + source);
}

const optiq::Statement& onlyStatement(const optiq::ProgramParseResult& result) {
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

    auto empty = parseProgram("-- only a comment\n");
    require(empty.success() && empty.program->statements.empty(),
            "an empty program parses to zero statements");

    {
        auto result = valid("let total = 50000 + 5000 * 2;");
        const auto& let = static_cast<const LetStatement&>(onlyStatement(result));
        require(let.name == "total", "let keeps the variable name");
        const auto& sum = binary(*let.initializer);
        require(sum.op == BinaryExpression::Operator::Add, "addition is the root");
        require(binary(*sum.right).op == BinaryExpression::Operator::Multiply,
                "multiplication binds more tightly in programs");
    }

    {
        auto result = valid("threshold = threshold + 1000;");
        const auto& assign = static_cast<const AssignStatement&>(onlyStatement(result));
        require(assign.name == "threshold", "assignment keeps the variable name");
        require(assign.value->kind == Expression::Kind::Binary,
                "assignment stores the value expression");
    }

    {
        auto result = valid("let ok = true; let done = false;");
        require(result.program->statements.size() == 2, "two declarations parse");
        const auto& first = static_cast<const LetStatement&>(*result.program->statements[0]);
        require(first.initializer->kind == Expression::Kind::Literal,
                "boolean initializer is a literal");
        const auto& literal = static_cast<const LiteralExpression&>(*first.initializer);
        require(literal.literalKind == LiteralExpression::LiteralKind::Boolean &&
                    std::get<bool>(literal.value),
                "TRUE parses as a boolean literal");
    }

    {
        auto result = valid(
            "if (matches > 3) { print('large'); } else if (matches > 0) { print('small'); }"
            " else { print('empty'); }");
        const auto& branch = static_cast<const IfStatement&>(onlyStatement(result));
        require(branch.thenBlock.size() == 1, "then block holds one statement");
        require(branch.elseBlock.size() == 1, "else-if occupies the else block");
        const auto& elseIf = *branch.elseBlock[0];
        require(elseIf.kind == Statement::Kind::If, "else-if is a nested if statement");
        const auto& nested = static_cast<const IfStatement&>(elseIf);
        require(nested.elseBlock.size() == 1 &&
                    nested.elseBlock[0]->kind == Statement::Kind::Print,
                "final else block is retained");
    }

    {
        auto result = valid("while (level <= 4) { if (level == 3) { print(level); } level = level + 1; }");
        const auto& loop = static_cast<const WhileStatement&>(onlyStatement(result));
        require(loop.body.size() == 2, "loop body holds both statements");
        require(loop.body[0]->kind == Statement::Kind::If &&
                    loop.body[1]->kind == Statement::Kind::Assign,
                "nested blocks keep their statement order");
    }

    {
        auto result = valid("SELECT name FROM employees ORDER BY name;");
        const auto& query = static_cast<const QueryStatement&>(onlyStatement(result));
        require(query.query->from.name == "employees", "query statement wraps the SELECT");
        require(query.query->orderBy.has_value(), "query clauses survive wrapping");
    }

    {
        auto result = valid("let rows = SELECT name FROM employees WHERE salary >= threshold;");
        const auto& let = static_cast<const LetStatement&>(onlyStatement(result));
        require(let.initializer->kind == Expression::Kind::Query,
                "SELECT binds as a query expression");
        const auto& query = static_cast<const QueryExpression&>(*let.initializer);
        require(query.query->where.has_value(), "query expression keeps its WHERE clause");
    }

    {
        auto result = valid("print(NOT a = 1 OR b = 2);");
        const auto& print = static_cast<const PrintStatement&>(onlyStatement(result));
        const auto& disjunction = binary(*print.expression);
        require(disjunction.op == BinaryExpression::Operator::Or,
                "OR is the root of the print expression");
        require(disjunction.left->kind == Expression::Kind::Unary,
                "NOT applies before OR");
    }

    {
        const std::string json = printProgramJson(*valid("let x = 1; print(x);").program);
        require(json.find("\"type\": \"Program\"") != std::string::npos,
                "printer emits the program root");
        require(json.find("\"type\": \"LetStatement\"") != std::string::npos &&
                    json.find("\"type\": \"PrintStatement\"") != std::string::npos,
                "printer emits statement nodes");
    }

    invalid("let x = 5");
    invalid("let x = ;");
    invalid("x = ;");
    invalid("if (true) { print(1);");
    invalid("while x > 1 { }");
    invalid("print(1");
    invalid("print 1;");
    invalid("let x = @;");
    invalid("SELECT FROM employees;");
    invalid("else { print(1); }");

    auto located = parseProgram("let x = 5;\nprint(x)");
    require(located.error.has_value(), "missing semicolon reports an error");
    require(located.error->location.line == 2, "program errors carry the source line");

    std::cout << "PASS: program statements, query expressions, precedence, and diagnostics\n";
    return 0;
}

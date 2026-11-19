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

std::unique_ptr<optiq::Expression> column(const std::string& name) {
    return std::make_unique<optiq::ColumnReference>(optiq::SourceLocation{}, name);
}

} // namespace

int main() {
    using namespace optiq;

    SelectStatement statement;
    statement.from = TableReference{{}, "employees", std::string("e")};
    statement.items.push_back(SelectItem::expressionItem(
        {}, std::make_unique<ColumnReference>(SourceLocation{}, "name", std::string("e")),
        std::string("employee_name")));
    statement.items.push_back(SelectItem::wildcard({}));

    auto predicate = std::make_unique<BinaryExpression>(
        SourceLocation{}, BinaryExpression::Operator::GreaterEqual, column("salary"),
        std::make_unique<LiteralExpression>(SourceLocation{}, std::int64_t{70000}));
    statement.where = WhereClause{{}, std::move(predicate)};

    statement.groupBy = GroupByClause{};
    statement.groupBy->columns.emplace_back(SourceLocation{}, "dept_id");
    statement.orderBy = OrderByClause{};
    statement.orderBy->items.push_back(OrderByItem{
        {}, ColumnReference(SourceLocation{}, "name"), OrderDirection::Descending});
    statement.limit = LimitClause{{}, 10};

    const std::string json = printAstJson(statement);
    require(json.find("\"type\": \"SelectStatement\"") != std::string::npos,
            "printer emits the root statement");
    require(json.find("\"alias\": \"employee_name\"") != std::string::npos,
            "printer emits select aliases");
    require(json.find("\"wildcard\": \"*\"") != std::string::npos,
            "printer emits wildcard selections");
    require(json.find("\"operator\": \">=\"") != std::string::npos,
            "printer traverses predicates");
    require(json.find("\"direction\": \"DESC\"") != std::string::npos,
            "printer emits ordering direction");

    auto count = FunctionCall::countStar({});
    require(count->starArgument && count->argument == nullptr,
            "COUNT(*) has an explicit typed representation");

    SelectStatement escaping;
    escaping.from = TableReference{{}, "departments", std::nullopt};
    escaping.items.push_back(SelectItem::expressionItem(
        {}, std::make_unique<LiteralExpression>(SourceLocation{}, std::string("a\"b\\c\n"))));
    const std::string escaped = printAstJson(escaping);
    require(escaped.find("a\\\"b\\\\c\\n") != std::string::npos,
            "printer escapes JSON string literals");

    std::cout << "PASS: AST ownership, structure, traversal, and JSON printing\n";
    return 0;
}

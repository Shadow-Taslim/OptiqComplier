#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "optiq/ast_printer.hpp"
#include "optiq/parser.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

const optiq::BinaryExpression& binary(const optiq::Expression& expression) {
    require(expression.kind == optiq::Expression::Kind::Binary, "expected binary expression");
    return static_cast<const optiq::BinaryExpression&>(expression);
}

const optiq::FunctionCall& function(const optiq::SelectItem& item) {
    require(item.expression && item.expression->kind == optiq::Expression::Kind::Function,
            "expected function call");
    return static_cast<const optiq::FunctionCall&>(*item.expression);
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace optiq;
    require(argc == 2, "fixture test requires the repository root path");
    const std::filesystem::path directory = std::filesystem::path(argv[1]) / "tests/fixtures";
    const std::array<const char*, 12> fixtures = {
        "01-ordered-projection.sql", "02-equality-filter.sql",
        "03-constant-arithmetic.sql", "04-inner-join-filter.sql",
        "05-grouped-count.sql", "06-grouped-average.sql",
        "07-filter-join-group-sum.sql", "08-conjunctive-range.sql",
        "09-disjunctive-filter.sql", "10-date-order-limit.sql",
        "11-global-aggregates.sql", "12-department-aggregate.sql"
    };

    for (std::size_t index = 0; index < fixtures.size(); ++index) {
        const auto path = directory / fixtures[index];
        std::ifstream input(path);
        require(input.good(), "fixture exists: " + path.string());
        const std::string sql((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
        auto result = parseSql(sql);
        if (result.error) {
            std::cerr << fixtures[index] << ": " << result.error->message << '\n';
        }
        require(result.success(), "fixture parses: " + path.string());
        const auto& statement = *result.statement;

        switch (index) {
        case 0:
            require(statement.items.size() == 2 && statement.orderBy.has_value(),
                    "ordered projection structure");
            break;
        case 1:
            require(binary(*statement.where->predicate).op == BinaryExpression::Operator::Equal,
                    "equality filter structure");
            break;
        case 2: {
            const auto& comparison = binary(*statement.where->predicate);
            require(comparison.op == BinaryExpression::Operator::Greater &&
                        binary(*comparison.right).op == BinaryExpression::Operator::Add,
                    "constant arithmetic remains explicit in the AST");
            break;
        }
        case 3:
            require(statement.join.has_value() && statement.where.has_value(),
                    "join and pushed-down-filter candidate structure");
            break;
        case 4:
            require(statement.groupBy.has_value() &&
                        function(statement.items[1]).function == FunctionCall::Function::Count,
                    "grouped COUNT structure");
            break;
        case 5:
            require(function(statement.items[1]).function == FunctionCall::Function::Average,
                    "grouped AVG structure");
            break;
        case 6:
            require(statement.join && statement.where && statement.groupBy &&
                        function(statement.items[1]).function == FunctionCall::Function::Sum,
                    "filter, join, group, and SUM structure");
            break;
        case 7:
            require(binary(*statement.where->predicate).op == BinaryExpression::Operator::And,
                    "conjunctive range structure");
            break;
        case 8:
            require(binary(*statement.where->predicate).op == BinaryExpression::Operator::Or,
                    "disjunctive filter structure");
            break;
        case 9:
            require(statement.orderBy && statement.limit && statement.limit->count == 3,
                    "date ordering and LIMIT structure");
            break;
        case 10:
            require(statement.items.size() == 3 && function(statement.items[0]).starArgument &&
                        function(statement.items[1]).function == FunctionCall::Function::Minimum &&
                        function(statement.items[2]).function == FunctionCall::Function::Maximum,
                    "global aggregate structure");
            break;
        case 11:
            require(statement.from.name == "departments" && statement.groupBy &&
                        function(statement.items[1]).starArgument,
                    "second-table aggregate structure");
            break;
        default:
            break;
        }

        require(!printAstJson(statement).empty(), "fixture AST prints: " + path.string());
    }

    std::cout << "PASS: all 12 documented SQL fixtures parse with expected AST structure\n";
    return 0;
}

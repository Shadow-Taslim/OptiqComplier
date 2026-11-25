#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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

} // namespace

int main(int argc, char* argv[]) {
    using namespace optiq;
    require(argc == 2, "program fixture test requires the repository root path");
    const std::filesystem::path directory = std::filesystem::path(argv[1]) / "examples";
    const std::array<const char*, 10> fixtures = {
        "01-variables-and-print.oq", "02-assignment-and-loop.oq",
        "03-conditional.oq", "04-else-if-chain.oq",
        "05-nested-blocks.oq", "06-query-statement.oq",
        "07-query-variable.oq", "08-filter-by-variable.oq",
        "09-loop-over-threshold.oq", "10-full-pipeline.oq"
    };
    const std::array<std::size_t, 10> statementCounts = {5, 3, 2, 3, 4, 1, 2, 3, 2, 4};

    for (std::size_t index = 0; index < fixtures.size(); ++index) {
        const auto path = directory / fixtures[index];
        std::ifstream input(path);
        require(input.good(), "example exists: " + path.string());
        const std::string source((std::istreambuf_iterator<char>(input)),
                                 std::istreambuf_iterator<char>());
        auto result = parseProgram(source);
        if (result.error) {
            std::cerr << fixtures[index] << ": " << result.error->message << '\n';
        }
        require(result.success(), "example parses: " + path.string());
        require(result.program->statements.size() == statementCounts[index],
                "example statement count matches: " + std::string(fixtures[index]));

        switch (index) {
        case 1:
            require(result.program->statements[1]->kind == Statement::Kind::While,
                    "loop example contains a while statement");
            break;
        case 3:
            require(result.program->statements[2]->kind == Statement::Kind::If,
                    "else-if example ends with a conditional");
            break;
        case 5:
            require(result.program->statements[0]->kind == Statement::Kind::Query,
                    "query example is a single query statement");
            break;
        case 6: {
            const auto& let = static_cast<const LetStatement&>(
                *result.program->statements[0]);
            require(let.initializer->kind == Expression::Kind::Query,
                    "query variable example binds a query expression");
            break;
        }
        case 9: {
            bool hasJoin = false;
            for (const auto& statement : result.program->statements) {
                if (statement->kind != Statement::Kind::Let) continue;
                const auto& let = static_cast<const LetStatement&>(*statement);
                if (let.initializer && let.initializer->kind == Expression::Kind::Query) {
                    const auto& query = static_cast<const QueryExpression&>(*let.initializer);
                    hasJoin = hasJoin || query.query->join.has_value();
                }
            }
            require(hasJoin, "full pipeline example contains a JOIN query");
            break;
        }
        default:
            break;
        }

        require(!printProgramJson(*result.program).empty(),
                "example AST prints: " + path.string());
    }

    std::cout << "PASS: all 10 documented Optiq example programs parse with expected structure\n";
    return 0;
}

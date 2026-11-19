#include <exception>
#include <iostream>
#include <iterator>
#include <string>

#include "optiq/ast_printer.hpp"
#include "optiq/parser.hpp"

namespace {

void printUsage(std::ostream& output) {
    output << "Usage: optiq_cli [SQL]\n"
              "       Read SQL from standard input when no argument is supplied.\n\n"
              "Exit codes: 0 = valid SQL, 2 = lexical or syntax error, 1 = internal error.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(std::cout);
        return 0;
    }

    std::string sql;
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (i > 1) sql.push_back(' ');
            sql += argv[i];
        }
    } else {
        sql.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    }

    try {
        auto result = optiq::parseSql(sql);
        if (!result.success()) {
            const auto& error = *result.error;
            std::cerr << "SQL error at line " << error.location.line
                      << ", column " << error.location.column << ": "
                      << error.message << '\n';
            return 2;
        }

        optiq::writeAstJson(std::cout, *result.statement);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Internal compiler error: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Internal compiler error: unknown failure\n";
        return 1;
    }
}

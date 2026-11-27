#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "optiq/cpp/ast_printer.hpp"
#include "optiq/cpp/parser.hpp"

namespace {

using optiq::cpp::NodeKind;
using optiq::cpp::ParseResult;

struct Runner {
    int checks = 0;
    int failures = 0;
    std::string actual;

    void check(bool condition, const std::string& detail) {
        ++checks;
        if (!condition) {
            ++failures;
            std::cerr << "  FAILED CHECK: " << detail << '\n';
        }
    }
};

std::string escaped(const std::string& input) {
    std::string result;
    for (char ch : input) {
        if (ch == '\n') result += "\\n";
        else if (ch == '\t') result += "\\t";
        else result += ch;
    }
    return result.empty() ? "<empty source>" : result;
}

void printDiagnostics(const ParseResult& result) {
    for (const auto& diagnostic : result.lexicalDiagnostics) {
        std::cerr << "  lexical: " << diagnostic.line << ':' << diagnostic.column
                  << ": " << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : result.syntaxDiagnostics) {
        std::cerr << "  syntax: " << diagnostic.line << ':' << diagnostic.column
                  << ": " << diagnostic.message << '\n';
    }
}

void expectValid(Runner& runner, const std::string& source) {
    const auto result = optiq::cpp::parse(source);
    runner.check(result.success(), "expected valid syntax and a generated AST");
    runner.check(result.lexicalDiagnostics.empty(), "valid input must have no lexical errors");
    runner.check(result.syntaxDiagnostics.empty(), "valid input must have no syntax errors");
    runner.check(result.program != nullptr, "valid input must produce a program node");
    runner.actual = result.success() ? "Parser SUCCESS; AST GENERATED" : "Parser FAILED";
    if (!result.success()) printDiagnostics(result);
}

void expectInvalid(Runner& runner, const std::string& source) {
    const auto result = optiq::cpp::parse(source);
    runner.check(!result.success(), "invalid syntax must not report success");
    runner.check(result.program == nullptr, "failed parsing must not expose a partial AST");
    runner.check(result.lexicalDiagnostics.empty(), "fixture must reach the parser");
    runner.check(!result.syntaxDiagnostics.empty(), "invalid syntax must produce a diagnostic");
    for (const auto& diagnostic : result.syntaxDiagnostics) {
        runner.check(diagnostic.line >= 1 && diagnostic.column >= 1,
                     "syntax diagnostic must have a one-based location");
        runner.check(!diagnostic.message.empty(), "syntax diagnostic must explain the error");
    }
    runner.actual = "Parser FAILED; " + std::to_string(result.syntaxDiagnostics.size()) +
                    " syntax diagnostic(s)";
    if (result.success()) runner.actual = "Unexpected Parser SUCCESS";
}

void testOperatorPrecedence(Runner& runner) {
    const auto result = optiq::cpp::parse(
        "int main() { int result = 10 + 5 * 2; return result; }");
    runner.check(result.success(), "precedence fixture must parse");
    if (!result.success()) {
        printDiagnostics(result);
        runner.actual = "Parser FAILED";
        return;
    }

    const auto* function = result.program->declarations.at(0).get();
    runner.check(function->kind == NodeKind::FunctionDefinition, "expected main function");
    const auto& definition = static_cast<const optiq::cpp::FunctionDefinitionNode&>(*function);
    const auto* block = static_cast<const optiq::cpp::BlockStatementNode*>(definition.body.get());
    const auto* declaration = static_cast<const optiq::cpp::VariableDeclarationNode*>(
        block->statements.at(0).get());
    const auto* sum = declaration->initializer.get();
    runner.check(sum->kind == NodeKind::BinaryExpression, "root must be a binary expression");
    const auto& sumNode = static_cast<const optiq::cpp::BinaryExpressionNode&>(*sum);
    runner.check(sumNode.operation == "+", "addition must be the root operation");
    runner.check(sumNode.right->kind == NodeKind::BinaryExpression,
                 "multiplication must be nested below addition");
    const auto& product = static_cast<const optiq::cpp::BinaryExpressionNode&>(*sumNode.right);
    runner.check(product.operation == "*", "right child must be multiplication");
    runner.actual = "AST root '+', right child '*': PASS structure";
}

void testAssignmentAssociativity(Runner& runner) {
    const auto result = optiq::cpp::parse(
        "int main() { int x; int y; x = y = 1; return 0; }");
    runner.check(result.success(), "assignment fixture must parse");
    if (!result.success()) {
        runner.actual = "Parser FAILED";
        return;
    }
    const auto& function = static_cast<const optiq::cpp::FunctionDefinitionNode&>(
        *result.program->declarations.at(0));
    const auto* block = static_cast<const optiq::cpp::BlockStatementNode*>(function.body.get());
    const auto* statement = static_cast<const optiq::cpp::ExpressionStatementNode*>(
        block->statements.at(2).get());
    const auto* assignment = statement->expression.get();
    runner.check(assignment->kind == NodeKind::AssignmentExpression,
                 "outer expression must be assignment");
    const auto& outer = static_cast<const optiq::cpp::AssignmentExpressionNode&>(*assignment);
    runner.check(outer.value->kind == NodeKind::AssignmentExpression,
                 "assignment must associate to the right");
    runner.actual = "Right-associative assignment AST verified";
}

void testAstLocations(Runner& runner) {
    const auto result = optiq::cpp::parse(
        "#include <iostream>\nint main() {\n  int value = 42;\n  return value;\n}\n");
    runner.check(result.success(), "location fixture must parse");
    if (!result.success()) {
        runner.actual = "Parser FAILED";
        return;
    }
    runner.check(result.program->location.line == 1 && result.program->location.column == 1,
                 "program location must identify the first declaration");
    const auto& function = static_cast<const optiq::cpp::FunctionDefinitionNode&>(
        *result.program->declarations.at(1));
    runner.check(function.location.line == 2 && function.location.column == 1,
                 "function location must be line 2, column 1");
    const auto* block = static_cast<const optiq::cpp::BlockStatementNode*>(function.body.get());
    const auto* declaration = static_cast<const optiq::cpp::VariableDeclarationNode*>(
        block->statements.at(0).get());
    runner.check(declaration->location.line == 3 && declaration->location.column == 7,
                 "declarator location must identify the variable name");
    runner.check(declaration->initializer->location.line == 3 &&
                     declaration->initializer->location.column == 15,
                 "literal location must identify its first digit");
    std::ostringstream tree;
    optiq::cpp::printAst(*result.program, tree);
    runner.check(tree.str().find("VariableDeclaration int value @3:7") != std::string::npos,
                 "AST printer must include node details and source locations");
    runner.actual = "AST locations and printed tree verified";
}

void testErrorRecovery(Runner& runner) {
    const auto result = optiq::cpp::parse(
        "int main() { int x = ; x = 1; return 0; }");
    runner.check(!result.success(), "recovered syntax errors must still fail the parse");
    runner.check(result.syntaxDiagnostics.size() == 1,
                 "statement synchronization must avoid cascading errors");
    runner.check(result.program == nullptr, "failed parse must not expose a partial AST");
    if (!result.syntaxDiagnostics.empty()) {
        runner.check(result.syntaxDiagnostics.front().message.find(";") != std::string::npos,
                     "diagnostic must identify the unexpected semicolon");
    }
    runner.actual = "Parser FAILED; synchronized at semicolon; " +
                    std::to_string(result.syntaxDiagnostics.size()) + " diagnostic";
}

void testTypeRestrictions(Runner& runner) {
    const auto valid = optiq::cpp::parse("void done() { return; }");
    runner.check(valid.success(), "void must be accepted as a function return type");
    const auto variable = optiq::cpp::parse("int main() { void value; return 0; }");
    runner.check(!variable.success() && !variable.syntaxDiagnostics.empty(),
                 "void variable declarations must be rejected by the grammar");
    const auto parameter = optiq::cpp::parse("int bad(void value) { return 0; }");
    runner.check(!parameter.success() && !parameter.syntaxDiagnostics.empty(),
                 "void named parameters must be rejected by the grammar");
    runner.actual = "Void return accepted; void variable and parameter rejected";
}

enum class TestKind { Valid, Invalid, Structural };
struct TestCase {
    const char* name;
    const char* source;
    const char* expected;
    TestKind kind;
    void (*run)(Runner&) = nullptr;
};

const std::vector<TestCase>& tests() {
    static const std::vector<TestCase> all = {
        {"positive_empty_translation_unit", "", "Empty Program AST", TestKind::Valid},
        {"positive_main_function", "int main() { return 0; }", "Main function parses", TestKind::Valid},
        {"positive_variable_declarations", "int main() { int x, y, z; float price; char grade; bool active; return 0; }", "Multiple and single declarations parse", TestKind::Valid},
        {"positive_variable_initialization", "int main() { double total = 25.5; char grade = 'A'; bool active = true; return 0; }", "Initialized declarations parse", TestKind::Valid},
        {"positive_arithmetic_expressions", "int main() { int r = (10 + 20) * 2 - 3 / 1 % 2; return r; }", "Arithmetic AST parses", TestKind::Valid},
        {"positive_relational_expressions", "int main() { bool r = 1 < 2 && 3 >= 2 && 4 != 5; return 0; }", "Relational/equality expressions parse", TestKind::Valid},
        {"positive_logical_expressions", "int main() { bool r = !false || true && false; return 0; }", "Logical precedence parses", TestKind::Valid},
        {"positive_nested_expressions", "int main() { int r = ((10 + 5) * (2 + 1)); return r; }", "Nested parentheses parse", TestKind::Valid},
        {"positive_if_statement", "int main() { int x = 11; if (x > 10) { x = x - 1; } return x; }", "If statement parses", TestKind::Valid},
        {"positive_if_else_statement", "int main() { int x = 0; if (x) x = 1; else x = 2; return x; }", "Dangling else binds to nearest if", TestKind::Valid},
        {"positive_while_loop", "int main() { int x = 3; while (x > 0) { x--; } return x; }", "While loop parses", TestKind::Valid},
        {"positive_for_loop", "int main() { int x = 0; for (int i = 0; i < 10; i++) { x += i; } return x; }", "For loop with declaration parses", TestKind::Valid},
        {"positive_do_while_loop", "int main() { int x = 3; do { x--; } while (x > 0); return x; }", "Do-while loop parses", TestKind::Valid},
        {"positive_function_definition", "int add(int a, int b); int add(int a, int b) { return a + b; }", "Prototype and definition parse", TestKind::Valid},
        {"positive_function_call", "int add(int a, int b) { return a + b; } int main() { int r = add(10, 20); int z = add(); return r; }", "Calls with arguments and empty arguments parse", TestKind::Valid},
        {"positive_return_statement", "void done() { return; } int main() { return 0; }", "Void and valued returns parse", TestKind::Valid},
        {"positive_nested_blocks", "int main() { int x = 10; { int y = 20; x = x + y; } return x; }", "Nested block parses", TestKind::Valid},
        {"positive_increment_decrement", "int main() { int i = 0; i++; ++i; i--; --i; return i; }", "Prefix/postfix increment and decrement parse", TestKind::Valid},
        {"positive_cout_syntax", "int main() { cout << \"Hello World\" << endl; return 0; }", "Unqualified stream output parses", TestKind::Valid},
        {"positive_cin_syntax", "int main() { int x; cin >> x; return 0; }", "Unqualified stream input parses", TestKind::Valid},
        {"positive_qualified_stream_syntax", "int main() { int x; std::cin >> x; std::cout << x << std::endl; return 0; }", "Qualified stream input/output parses", TestKind::Valid},
        {"positive_include_directives", "#include <iostream>\n#include <vector>\nint main() { return 0; }", "Supported include directives parse", TestKind::Valid},
        {"positive_namespace_declaration", "#include <iostream>\nusing namespace std;\nint main() { cout << endl; return 0; }", "Using-namespace declaration parses", TestKind::Valid},
        {"negative_missing_semicolon", "int main() { return 0 }", "Reject missing semicolon", TestKind::Invalid},
        {"negative_missing_closing_brace", "int main() { return 0;", "Reject missing closing brace", TestKind::Invalid},
        {"negative_missing_opening_parenthesis", "int main) { return 0; }", "Reject missing opening parenthesis", TestKind::Invalid},
        {"negative_invalid_expression", "int main() { int x = 10 + * 2; return 0; }", "Reject invalid expression", TestKind::Invalid},
        {"negative_incomplete_assignment", "int main() { int x; x = ; return 0; }", "Reject incomplete assignment", TestKind::Invalid},
        {"negative_invalid_function_declaration", "int main( { return 0; }", "Reject invalid function declarator", TestKind::Invalid},
        {"negative_incorrect_parameter_syntax", "int add(int a, b) { return a; }", "Reject parameter without a type", TestKind::Invalid},
        {"negative_invalid_for_loop", "int main() { for (int i = 0 i < 10; i++) { i++; } return 0; }", "Reject malformed for-loop clauses", TestKind::Invalid},
        {"negative_invalid_stream_insertion", "int main() { cout << ; return 0; }", "Reject missing stream operand", TestKind::Invalid},
        {"negative_unexpected_token", "int main() { int x = 1 ] ; return 0; }", "Reject unexpected closing bracket", TestKind::Invalid},
        {"negative_malformed_nested_expression", "int main() { int x = (10 + 5 * 2; return 0; }", "Reject unmatched nested expression", TestKind::Invalid},
        {"negative_invalid_control_flow", "int main() { int x = 1; if x > 10) { x++; } return 0; }", "Reject invalid if condition syntax", TestKind::Invalid},
        {"operator_precedence", "int result = 10 + 5 * 2", "Multiplication nests below addition", TestKind::Structural, testOperatorPrecedence},
        {"assignment_associativity", "x = y = 1", "Assignment associates right", TestKind::Structural, testAssignmentAssociativity},
        {"ast_source_locations", "Located declaration and AST printer", "Exact node locations and printed AST", TestKind::Structural, testAstLocations},
        {"error_recovery", "Bad declaration followed by valid statements", "One diagnostic; synchronize at semicolon", TestKind::Structural, testErrorRecovery},
        {"type_restrictions", "Void return, variable, and parameter", "Only the void function return is accepted", TestKind::Structural, testTypeRestrictions},
    };
    return all;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: optiq_cpp_parser_tests <test-name>\n";
        return 1;
    }
    const auto& all = tests();
    const auto test = std::find_if(all.begin(), all.end(), [&](const auto& candidate) {
        return candidate.name == std::string(argv[1]);
    });
    if (test == all.end()) {
        std::cerr << "Unknown test: " << argv[1] << '\n';
        return 1;
    }

    std::cout << "Test: " << test->name << '\n'
              << "Input: " << escaped(test->source) << '\n'
              << "Expected: " << test->expected << '\n';
    Runner runner;
    if (test->kind == TestKind::Valid) expectValid(runner, test->source);
    else if (test->kind == TestKind::Invalid) expectInvalid(runner, test->source);
    else test->run(runner);

    if (runner.actual.empty()) runner.actual = "No result recorded";
    std::cout << "Actual: " << runner.actual << "; " << runner.checks
              << " checks executed; " << runner.failures << " failed\n"
              << (runner.failures == 0 ? "PASS\n" : "FAIL\n");
    return runner.failures == 0 ? 0 : 1;
}

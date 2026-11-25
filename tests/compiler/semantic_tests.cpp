#include <cstdlib>
#include <iostream>
#include <string>

#include "optiq/parser.hpp"
#include "optiq/semantic.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

optiq::SemanticResult analyzeSource(const std::string& source) {
    auto parsed = optiq::parseProgram(source);
    require(parsed.success(), "test program parses: " + source);
    return optiq::analyze(*parsed.program);
}

void expectError(const std::string& source, const std::string& needle) {
    const auto result = analyzeSource(source);
    require(!result.success(), "semantic error is detected: " + source);
    bool found = false;
    for (const auto& error : result.errors) {
        if (error.message.find(needle) != std::string::npos) found = true;
    }
    require(found, "error mentions \"" + needle + "\" for: " + source);
}

void expectClean(const std::string& source) {
    const auto result = analyzeSource(source);
    for (const auto& error : result.errors) {
        std::cerr << "Unexpected semantic error for " << source << ": "
                  << error.message << '\n';
    }
    require(result.success(), "valid program passes semantic analysis: " + source);
}

} // namespace

int main() {
    // A fully valid program compiles cleanly.
    expectClean(
        "int x;\nfloat y;\nbool flag;\n"
        "x = 10;\ny = x + 0.5;\nflag = true;\n"
        "while (x > 0) {\n  x = x - 1;\n}\n"
        "if (flag == true) {\n  print y;\n} else {\n  print x;\n}\n");

    // Widening int -> float is allowed; everything else must match exactly.
    expectClean("float f = 5;");
    expectClean("int a = 1; float b = a * 2;");

    // Undeclared variable use.
    expectError("x = 5;", "Undeclared variable 'x'");
    expectError("int x = y + 1;", "Undeclared variable 'y'");

    // Redeclaration in the same scope.
    expectError("int x; int x;", "Redeclaration of 'x'");

    // Scope violation: a block-local variable is not visible outside.
    expectError("if (true) { int y = 1; } y = 2;", "Undeclared variable 'y'");
    expectError("while (false) { float z = 1.0; } print z;",
                "Undeclared variable 'z'");

    // ...but shadowing an outer variable inside a block is legal.
    expectClean("int x = 1; if (true) { int x = 2; print x; } print x;");
    // ...and outer variables remain visible inside blocks.
    expectClean("int x = 1; if (true) { x = x + 1; }");

    // Type mismatch in a declaration initializer (the manual's example).
    expectError("bool b = 5 + 3.2;", "Type mismatch");
    expectError("int x = true;", "Type mismatch");
    expectError("int x = 3.2;", "Type mismatch");

    // Invalid assignment.
    expectError("int x; x = true;", "Invalid assignment");
    expectError("int x; x = 1.5;", "Invalid assignment");
    expectError("float f; f = false;", "Invalid assignment");

    // Invalid expressions: logical operators on numeric operands.
    expectError("bool b = 1 && 2;", "bool operands");
    expectError("bool b = !5;", "'!' requires a bool operand");
    expectError("int x = -true;", "numeric operand");
    expectError("bool b; b = 1 < 2 < 3;", "Relational operator");

    // Operator-specific rules.
    expectError("int x = 1.5 % 2;", "'%' requires int operands");
    expectError("bool b = true < false;", "numeric operands");
    expectError("bool b = 1 == true;", "matching operand types");

    // Conditions must be bool.
    expectError("if (1) { int x; }", "if condition");
    expectError("while (5 + 2) { int x; }", "while condition");

    // Semantic errors carry line numbers.
    {
        const auto result = analyzeSource("int x;\nx = 1;\ny = 2;");
        require(result.errors.size() == 1, "one semantic error is reported");
        require(result.errors[0].location.line == 3,
                "semantic errors carry the source line");
    }

    std::cout << "PASS: scoping, declaration rules, and type checking\n";
    return 0;
}

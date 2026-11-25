#include <cstdlib>
#include <iostream>
#include <string>

#include "optiq/parser.hpp"
#include "optiq/tac.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

std::string tacOf(const std::string& source) {
    auto parsed = optiq::parseProgram(source);
    require(parsed.success(), "test program parses: " + source);
    return optiq::tacToString(optiq::generateTac(*parsed.program));
}

void expectLines(const std::string& source, std::initializer_list<const char*> lines) {
    const std::string tac = tacOf(source);
    std::string expected;
    for (const char* line : lines) {
        expected += line;
        expected += '\n';
    }
    if (tac != expected) {
        std::cerr << "TAC mismatch for: " << source << "\nExpected:\n"
                  << expected << "Actual:\n" << tac;
        require(false, "TAC output matches the expected instruction sequence");
    }
}

} // namespace

int main() {
    // Declarations without initializers emit no code; plain assignments map
    // one-to-one, exactly like the manual's worked example.
    expectLines("int a; int b; int c; a = 5; b = 10; c = a + b * 2;",
                {"a = 5", "b = 10", "t1 = b * 2", "t2 = a + t1", "c = t2"});

    // Declaration with initializer behaves like a declaration + assignment.
    expectLines("int x = 40 + 2;", {"t1 = 40 + 2", "x = t1"});

    // Precedence is respected through temporary ordering.
    expectLines("x = a - b - c;", {"t1 = a - b", "t2 = t1 - c", "x = t2"});

    // Unary operators.
    expectLines("x = -y; b = !flag;", {"t1 = -y", "x = t1", "t2 = !flag", "b = t2"});

    // Relational and logical operators produce temporaries.
    expectLines("b = x < 3 && y != 0 || flag;",
                {"t1 = x < 3", "t2 = y != 0", "t3 = t1 && t2", "t4 = t3 || flag",
                 "b = t4"});

    // if without else: conditional jump over the then block.
    expectLines("if (x > 0) { print x; }",
                {"t1 = x > 0", "ifFalse t1 goto L1", "print x", "L1:"});

    // if-else: conditional jump to else, unconditional jump to end.
    expectLines("if (x > 0) { y = 1; } else { y = 2; }",
                {"t1 = x > 0", "ifFalse t1 goto L1", "y = 1", "goto L2", "L1:",
                 "y = 2", "L2:"});

    // while: start label, conditional exit, body, backward jump.
    expectLines("while (x > 0) { x = x - 1; }",
                {"L1:", "t1 = x > 0", "ifFalse t1 goto L2", "t2 = x - 1",
                 "x = t2", "goto L1", "L2:"});

    // Nested control flow gets distinct labels.
    expectLines("while (x > 0) { if (x == 1) { print x; } x = x - 1; }",
                {"L1:", "t1 = x > 0", "ifFalse t1 goto L2", "t2 = x == 1",
                 "ifFalse t2 goto L3", "print x", "L3:", "t3 = x - 1", "x = t3",
                 "goto L1", "L2:"});

    // print accepts any expression.
    expectLines("print 42;", {"print 42"});
    expectLines("print x + 1;", {"t1 = x + 1", "print t1"});

    std::cout << "PASS: TAC generation for expressions, control flow, and print\n";
    return 0;
}

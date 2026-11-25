#include "optiq/ast_printer.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace optiq {
namespace {

std::string quoted(const std::string& value) {
    std::ostringstream output;
    output << '"';
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': output << "\\\""; break;
        case '\\': output << "\\\\"; break;
        case '\b': output << "\\b"; break;
        case '\f': output << "\\f"; break;
        case '\n': output << "\\n"; break;
        case '\r': output << "\\r"; break;
        case '\t': output << "\\t"; break;
        default:
            if (ch < 0x20) {
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                       << static_cast<int>(ch) << std::dec;
            } else {
                output << static_cast<char>(ch);
            }
        }
    }
    output << '"';
    return output.str();
}

std::ostream& indent(std::ostream& output, int depth) {
    return output << std::string(static_cast<std::size_t>(depth) * 2, ' ');
}

void writeLocation(std::ostream& output, const SourceLocation& location, int depth) {
    indent(output, depth) << "\"line\": " << location.line << ",\n";
    indent(output, depth) << "\"column\": " << location.column << ",\n";
}

std::string binaryOperator(BinaryExpression::Operator op) {
    switch (op) {
    case BinaryExpression::Operator::Or: return "||";
    case BinaryExpression::Operator::And: return "&&";
    case BinaryExpression::Operator::Equal: return "==";
    case BinaryExpression::Operator::NotEqual: return "!=";
    case BinaryExpression::Operator::Less: return "<";
    case BinaryExpression::Operator::LessEqual: return "<=";
    case BinaryExpression::Operator::Greater: return ">";
    case BinaryExpression::Operator::GreaterEqual: return ">=";
    case BinaryExpression::Operator::Add: return "+";
    case BinaryExpression::Operator::Subtract: return "-";
    case BinaryExpression::Operator::Multiply: return "*";
    case BinaryExpression::Operator::Divide: return "/";
    case BinaryExpression::Operator::Modulo: return "%";
    }
    throw std::logic_error("Unknown binary operator");
}

void writeExpression(std::ostream& output, const Expression& expression, int depth) {
    output << "{\n";
    switch (expression.kind) {
    case Expression::Kind::Variable: {
        const auto& variable = static_cast<const VariableExpression&>(expression);
        indent(output, depth + 1) << "\"type\": \"VariableExpression\",\n";
        writeLocation(output, variable.location, depth + 1);
        indent(output, depth + 1) << "\"name\": " << quoted(variable.name) << '\n';
        break;
    }
    case Expression::Kind::Literal: {
        const auto& literal = static_cast<const LiteralExpression&>(expression);
        indent(output, depth + 1) << "\"type\": \"LiteralExpression\",\n";
        writeLocation(output, literal.location, depth + 1);
        indent(output, depth + 1) << "\"dataType\": " << quoted(dataTypeName(literal.type)) << ",\n";
        indent(output, depth + 1) << "\"value\": ";
        if (const auto* value = std::get_if<std::int64_t>(&literal.value)) {
            output << *value;
        } else if (const auto* value = std::get_if<double>(&literal.value)) {
            output << std::setprecision(15) << *value;
        } else {
            output << (std::get<bool>(literal.value) ? "true" : "false");
        }
        output << '\n';
        break;
    }
    case Expression::Kind::Unary: {
        const auto& unary = static_cast<const UnaryExpression&>(expression);
        indent(output, depth + 1) << "\"type\": \"UnaryExpression\",\n";
        writeLocation(output, unary.location, depth + 1);
        indent(output, depth + 1) << "\"operator\": "
            << quoted(unary.op == UnaryExpression::Operator::Not ? "!" : "-") << ",\n";
        indent(output, depth + 1) << "\"operand\": ";
        writeExpression(output, *unary.operand, depth + 1);
        output << '\n';
        break;
    }
    case Expression::Kind::Binary: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        indent(output, depth + 1) << "\"type\": \"BinaryExpression\",\n";
        writeLocation(output, binary.location, depth + 1);
        indent(output, depth + 1) << "\"operator\": " << quoted(binaryOperator(binary.op)) << ",\n";
        indent(output, depth + 1) << "\"left\": ";
        writeExpression(output, *binary.left, depth + 1);
        output << ",\n";
        indent(output, depth + 1) << "\"right\": ";
        writeExpression(output, *binary.right, depth + 1);
        output << '\n';
        break;
    }
    }
    indent(output, depth) << '}';
}

void writeStatementList(std::ostream& output, const StatementList& statements, int depth);

void writeStatement(std::ostream& output, const Statement& statement, int depth) {
    output << "{\n";
    switch (statement.kind) {
    case Statement::Kind::Declaration: {
        const auto& declaration = static_cast<const DeclarationStatement&>(statement);
        indent(output, depth + 1) << "\"type\": \"DeclarationStatement\",\n";
        writeLocation(output, declaration.location, depth + 1);
        indent(output, depth + 1) << "\"dataType\": "
                                  << quoted(dataTypeName(declaration.declaredType)) << ",\n";
        indent(output, depth + 1) << "\"name\": " << quoted(declaration.name) << '\n';
        break;
    }
    case Statement::Kind::Assignment: {
        const auto& assignment = static_cast<const AssignmentStatement&>(statement);
        indent(output, depth + 1) << "\"type\": \"AssignmentStatement\",\n";
        writeLocation(output, assignment.location, depth + 1);
        indent(output, depth + 1) << "\"name\": " << quoted(assignment.name) << ",\n";
        indent(output, depth + 1) << "\"value\": ";
        writeExpression(output, *assignment.value, depth + 1);
        output << '\n';
        break;
    }
    case Statement::Kind::Print: {
        const auto& print = static_cast<const PrintStatement&>(statement);
        indent(output, depth + 1) << "\"type\": \"PrintStatement\",\n";
        writeLocation(output, print.location, depth + 1);
        indent(output, depth + 1) << "\"expression\": ";
        writeExpression(output, *print.expression, depth + 1);
        output << '\n';
        break;
    }
    case Statement::Kind::If: {
        const auto& branch = static_cast<const IfStatement&>(statement);
        indent(output, depth + 1) << "\"type\": \"IfStatement\",\n";
        writeLocation(output, branch.location, depth + 1);
        indent(output, depth + 1) << "\"condition\": ";
        writeExpression(output, *branch.condition, depth + 1);
        output << ",\n";
        indent(output, depth + 1) << "\"then\": ";
        writeStatementList(output, branch.thenBlock, depth + 1);
        output << ",\n";
        indent(output, depth + 1) << "\"else\": ";
        if (branch.hasElse) {
            writeStatementList(output, branch.elseBlock, depth + 1);
        } else {
            output << "null";
        }
        output << '\n';
        break;
    }
    case Statement::Kind::While: {
        const auto& loop = static_cast<const WhileStatement&>(statement);
        indent(output, depth + 1) << "\"type\": \"WhileStatement\",\n";
        writeLocation(output, loop.location, depth + 1);
        indent(output, depth + 1) << "\"condition\": ";
        writeExpression(output, *loop.condition, depth + 1);
        output << ",\n";
        indent(output, depth + 1) << "\"body\": ";
        writeStatementList(output, loop.body, depth + 1);
        output << '\n';
        break;
    }
    case Statement::Kind::Block: {
        const auto& block = static_cast<const BlockStatement&>(statement);
        indent(output, depth + 1) << "\"type\": \"BlockStatement\",\n";
        writeLocation(output, block.location, depth + 1);
        indent(output, depth + 1) << "\"statements\": ";
        writeStatementList(output, block.statements, depth + 1);
        output << '\n';
        break;
    }
    }
    indent(output, depth) << '}';
}

void writeStatementList(std::ostream& output, const StatementList& statements, int depth) {
    if (statements.empty()) {
        output << "[]";
        return;
    }
    output << "[\n";
    for (std::size_t i = 0; i < statements.size(); ++i) {
        indent(output, depth + 1);
        writeStatement(output, *statements[i], depth + 1);
        output << (i + 1 == statements.size() ? "\n" : ",\n");
    }
    indent(output, depth) << ']';
}

} // namespace

void writeProgramJson(std::ostream& output, const Program& program) {
    output << "{\n  \"type\": \"Program\",\n  \"statements\": ";
    writeStatementList(output, program.statements, 1);
    output << "\n}\n";
}

std::string printProgramJson(const Program& program) {
    std::ostringstream output;
    writeProgramJson(output, program);
    return output.str();
}

} // namespace optiq

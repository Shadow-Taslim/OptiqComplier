#include "optiq/ast_printer.hpp"

#include <sstream>
#include <string>

namespace optiq {

std::string valueTypeName(ValueType type) {
    switch (type) {
    case ValueType::Int: return "int";
    case ValueType::Float: return "float";
    case ValueType::Bool: return "bool";
    }
    return "unknown";
}

namespace {

std::string binaryOperatorName(BinaryExpression::Operator op) {
    switch (op) {
    case BinaryExpression::Operator::Add: return "+";
    case BinaryExpression::Operator::Subtract: return "-";
    case BinaryExpression::Operator::Multiply: return "*";
    case BinaryExpression::Operator::Divide: return "/";
    case BinaryExpression::Operator::Modulo: return "%";
    case BinaryExpression::Operator::Less: return "<";
    case BinaryExpression::Operator::Greater: return ">";
    case BinaryExpression::Operator::LessEqual: return "<=";
    case BinaryExpression::Operator::GreaterEqual: return ">=";
    case BinaryExpression::Operator::Equal: return "==";
    case BinaryExpression::Operator::NotEqual: return "!=";
    case BinaryExpression::Operator::And: return "&&";
    case BinaryExpression::Operator::Or: return "||";
    }
    return "?";
}

std::string unaryOperatorName(UnaryExpression::Operator op) {
    switch (op) {
    case UnaryExpression::Operator::Negate: return "-";
    case UnaryExpression::Operator::Not: return "!";
    }
    return "?";
}

std::string literalText(const LiteralExpression& literal) {
    std::ostringstream text;
    switch (literal.literalKind) {
    case LiteralExpression::LiteralKind::Integer:
        text << std::get<std::int64_t>(literal.value);
        break;
    case LiteralExpression::LiteralKind::Float:
        text << std::get<double>(literal.value);
        break;
    case LiteralExpression::LiteralKind::Boolean:
        text << (std::get<bool>(literal.value) ? "true" : "false");
        break;
    }
    return text.str();
}

// ---------------------------- Text printing ------------------------------

void writeExpressionText(std::ostream& out, const Expression& expression, int depth);

std::string indent(int depth) { return std::string(static_cast<std::size_t>(depth) * 2, ' '); }

void writeStatementText(std::ostream& out, const Statement& statement, int depth) {
    const std::string pad = indent(depth);
    switch (statement.kind) {
    case Statement::Kind::Declaration: {
        const auto& node = static_cast<const DeclarationStatement&>(statement);
        out << pad << "Declaration " << valueTypeName(node.type) << ' ' << node.name;
        if (node.initializer) {
            out << " =\n";
            writeExpressionText(out, *node.initializer, depth + 1);
        } else {
            out << '\n';
        }
        break;
    }
    case Statement::Kind::Assignment: {
        const auto& node = static_cast<const AssignmentStatement&>(statement);
        out << pad << "Assignment " << node.name << " =\n";
        writeExpressionText(out, *node.value, depth + 1);
        break;
    }
    case Statement::Kind::If: {
        const auto& node = static_cast<const IfStatement&>(statement);
        out << pad << "If\n" << pad << "  condition:\n";
        writeExpressionText(out, *node.condition, depth + 2);
        out << pad << "  then:\n";
        writeStatementText(out, *node.thenBlock, depth + 2);
        if (node.elseBlock) {
            out << pad << "  else:\n";
            writeStatementText(out, *node.elseBlock, depth + 2);
        }
        break;
    }
    case Statement::Kind::While: {
        const auto& node = static_cast<const WhileStatement&>(statement);
        out << pad << "While\n" << pad << "  condition:\n";
        writeExpressionText(out, *node.condition, depth + 2);
        out << pad << "  body:\n";
        writeStatementText(out, *node.body, depth + 2);
        break;
    }
    case Statement::Kind::Print: {
        const auto& node = static_cast<const PrintStatement&>(statement);
        out << pad << "Print\n";
        writeExpressionText(out, *node.expression, depth + 1);
        break;
    }
    case Statement::Kind::Block: {
        const auto& node = static_cast<const BlockStatement&>(statement);
        out << pad << "Block\n";
        for (const auto& child : node.statements) {
            writeStatementText(out, *child, depth + 1);
        }
        break;
    }
    }
}

void writeExpressionText(std::ostream& out, const Expression& expression, int depth) {
    const std::string pad = indent(depth);
    switch (expression.kind) {
    case Expression::Kind::Identifier:
        out << pad << "Identifier "
            << static_cast<const IdentifierExpression&>(expression).name << '\n';
        break;
    case Expression::Kind::Literal: {
        const auto& node = static_cast<const LiteralExpression&>(expression);
        out << pad << "Literal " << literalText(node) << '\n';
        break;
    }
    case Expression::Kind::Binary: {
        const auto& node = static_cast<const BinaryExpression&>(expression);
        out << pad << "Binary " << binaryOperatorName(node.op) << '\n';
        writeExpressionText(out, *node.left, depth + 1);
        writeExpressionText(out, *node.right, depth + 1);
        break;
    }
    case Expression::Kind::Unary: {
        const auto& node = static_cast<const UnaryExpression&>(expression);
        out << pad << "Unary " << unaryOperatorName(node.op) << '\n';
        writeExpressionText(out, *node.operand, depth + 1);
        break;
    }
    }
}

// ---------------------------- JSON printing ------------------------------

void writeEscaped(std::ostream& out, const std::string& text) {
    for (char ch : text) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\t': out << "\\t"; break;
        default: out << ch;
        }
    }
}

void writeExpressionJson(std::ostream& out, const Expression& expression);

void writeStatementJson(std::ostream& out, const Statement& statement);

void writeStatementListJson(std::ostream& out,
                            const std::vector<std::unique_ptr<Statement>>& statements) {
    out << '[';
    for (std::size_t i = 0; i < statements.size(); ++i) {
        if (i > 0) out << ", ";
        writeStatementJson(out, *statements[i]);
    }
    out << ']';
}

void writeStatementJson(std::ostream& out, const Statement& statement) {
    switch (statement.kind) {
    case Statement::Kind::Declaration: {
        const auto& node = static_cast<const DeclarationStatement&>(statement);
        out << "{\"type\": \"DeclarationStatement\", \"dataType\": \""
            << valueTypeName(node.type) << "\", \"name\": \"";
        writeEscaped(out, node.name);
        out << "\", \"initializer\": ";
        if (node.initializer) {
            writeExpressionJson(out, *node.initializer);
        } else {
            out << "null";
        }
        out << '}';
        break;
    }
    case Statement::Kind::Assignment: {
        const auto& node = static_cast<const AssignmentStatement&>(statement);
        out << "{\"type\": \"AssignmentStatement\", \"name\": \"";
        writeEscaped(out, node.name);
        out << "\", \"value\": ";
        writeExpressionJson(out, *node.value);
        out << '}';
        break;
    }
    case Statement::Kind::If: {
        const auto& node = static_cast<const IfStatement&>(statement);
        out << "{\"type\": \"IfStatement\", \"condition\": ";
        writeExpressionJson(out, *node.condition);
        out << ", \"then\": ";
        writeStatementJson(out, *node.thenBlock);
        out << ", \"else\": ";
        if (node.elseBlock) {
            writeStatementJson(out, *node.elseBlock);
        } else {
            out << "null";
        }
        out << '}';
        break;
    }
    case Statement::Kind::While: {
        const auto& node = static_cast<const WhileStatement&>(statement);
        out << "{\"type\": \"WhileStatement\", \"condition\": ";
        writeExpressionJson(out, *node.condition);
        out << ", \"body\": ";
        writeStatementJson(out, *node.body);
        out << '}';
        break;
    }
    case Statement::Kind::Print: {
        const auto& node = static_cast<const PrintStatement&>(statement);
        out << "{\"type\": \"PrintStatement\", \"expression\": ";
        writeExpressionJson(out, *node.expression);
        out << '}';
        break;
    }
    case Statement::Kind::Block: {
        const auto& node = static_cast<const BlockStatement&>(statement);
        out << "{\"type\": \"BlockStatement\", \"statements\": ";
        writeStatementListJson(out, node.statements);
        out << '}';
        break;
    }
    }
}

void writeExpressionJson(std::ostream& out, const Expression& expression) {
    switch (expression.kind) {
    case Expression::Kind::Identifier: {
        const auto& node = static_cast<const IdentifierExpression&>(expression);
        out << "{\"type\": \"Identifier\", \"name\": \"";
        writeEscaped(out, node.name);
        out << "\"}";
        break;
    }
    case Expression::Kind::Literal: {
        const auto& node = static_cast<const LiteralExpression&>(expression);
        out << "{\"type\": \"Literal\", \"kind\": \"";
        switch (node.literalKind) {
        case LiteralExpression::LiteralKind::Integer: out << "integer"; break;
        case LiteralExpression::LiteralKind::Float: out << "float"; break;
        case LiteralExpression::LiteralKind::Boolean: out << "boolean"; break;
        }
        out << "\", \"value\": " << literalText(node) << '}';
        break;
    }
    case Expression::Kind::Binary: {
        const auto& node = static_cast<const BinaryExpression&>(expression);
        out << "{\"type\": \"BinaryExpression\", \"operator\": \""
            << binaryOperatorName(node.op) << "\", \"left\": ";
        writeExpressionJson(out, *node.left);
        out << ", \"right\": ";
        writeExpressionJson(out, *node.right);
        out << '}';
        break;
    }
    case Expression::Kind::Unary: {
        const auto& node = static_cast<const UnaryExpression&>(expression);
        out << "{\"type\": \"UnaryExpression\", \"operator\": \""
            << unaryOperatorName(node.op) << "\", \"operand\": ";
        writeExpressionJson(out, *node.operand);
        out << '}';
        break;
    }
    }
}

} // namespace

void writeProgramText(std::ostream& output, const Program& program) {
    output << "Program\n";
    for (const auto& statement : program.statements) {
        writeStatementText(output, *statement, 1);
    }
}

std::string printProgramText(const Program& program) {
    std::ostringstream text;
    writeProgramText(text, program);
    return text.str();
}

void writeProgramJson(std::ostream& output, const Program& program) {
    output << "{\"type\": \"Program\", \"statements\": ";
    writeStatementListJson(output, program.statements);
    output << '}';
}

std::string printProgramJson(const Program& program) {
    std::ostringstream json;
    writeProgramJson(json, program);
    return json.str();
}

} // namespace optiq

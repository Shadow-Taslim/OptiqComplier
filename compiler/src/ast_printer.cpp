#include "optiq/ast_printer.hpp"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>

namespace optiq {
namespace {

std::string escapeJson(const std::string& value) {
    std::ostringstream output;
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
    return output.str();
}

std::string quoted(const std::string& value) {
    return "\"" + escapeJson(value) + "\"";
}

std::string optionalString(const std::optional<std::string>& value) {
    return value ? quoted(*value) : "null";
}

std::string binaryOperator(BinaryExpression::Operator op) {
    switch (op) {
    case BinaryExpression::Operator::Or: return "OR";
    case BinaryExpression::Operator::And: return "AND";
    case BinaryExpression::Operator::Equal: return "=";
    case BinaryExpression::Operator::NotEqual: return "!=";
    case BinaryExpression::Operator::Less: return "<";
    case BinaryExpression::Operator::LessEqual: return "<=";
    case BinaryExpression::Operator::Greater: return ">";
    case BinaryExpression::Operator::GreaterEqual: return ">=";
    case BinaryExpression::Operator::Add: return "+";
    case BinaryExpression::Operator::Subtract: return "-";
    case BinaryExpression::Operator::Multiply: return "*";
    case BinaryExpression::Operator::Divide: return "/";
    }
    throw std::logic_error("Unknown binary operator");
}

std::string unaryOperator(UnaryExpression::Operator op) {
    switch (op) {
    case UnaryExpression::Operator::Not: return "NOT";
    case UnaryExpression::Operator::Positive: return "+";
    case UnaryExpression::Operator::Negative: return "-";
    }
    throw std::logic_error("Unknown unary operator");
}

std::string functionName(FunctionCall::Function function) {
    switch (function) {
    case FunctionCall::Function::Count: return "COUNT";
    case FunctionCall::Function::Sum: return "SUM";
    case FunctionCall::Function::Average: return "AVG";
    case FunctionCall::Function::Minimum: return "MIN";
    case FunctionCall::Function::Maximum: return "MAX";
    }
    throw std::logic_error("Unknown function");
}

void indent(std::ostream& output, int depth) {
    output << std::string(static_cast<std::size_t>(depth) * 2, ' ');
}

void writeExpression(std::ostream& output, const Expression& expression, int depth);

void writeColumn(std::ostream& output, const ColumnReference& column, int depth) {
    output << "{\n";
    indent(output, depth + 1) << "\"type\": \"ColumnReference\",\n";
    indent(output, depth + 1) << "\"qualifier\": " << optionalString(column.qualifier) << ",\n";
    indent(output, depth + 1) << "\"name\": " << quoted(column.name) << "\n";
    indent(output, depth) << '}';
}

void writeExpression(std::ostream& output, const Expression& expression, int depth) {
    switch (expression.kind) {
    case Expression::Kind::Column:
        writeColumn(output, static_cast<const ColumnReference&>(expression), depth);
        return;
    case Expression::Kind::Literal: {
        const auto& literal = static_cast<const LiteralExpression&>(expression);
        output << "{\n";
        indent(output, depth + 1) << "\"type\": \"LiteralExpression\",\n";
        indent(output, depth + 1) << "\"kind\": ";
        switch (literal.literalKind) {
        case LiteralExpression::LiteralKind::Integer: output << "\"integer\""; break;
        case LiteralExpression::LiteralKind::Decimal: output << "\"decimal\""; break;
        case LiteralExpression::LiteralKind::String: output << "\"string\""; break;
        }
        output << ",\n" << std::string(static_cast<std::size_t>(depth + 1) * 2, ' ')
               << "\"value\": ";
        if (const auto* value = std::get_if<std::int64_t>(&literal.value)) {
            output << *value;
        } else if (const auto* value = std::get_if<double>(&literal.value)) {
            output << std::setprecision(15) << *value;
        } else {
            output << quoted(std::get<std::string>(literal.value));
        }
        output << '\n';
        indent(output, depth) << '}';
        return;
    }
    case Expression::Kind::Binary: {
        const auto& binary = static_cast<const BinaryExpression&>(expression);
        output << "{\n";
        indent(output, depth + 1) << "\"type\": \"BinaryExpression\",\n";
        indent(output, depth + 1) << "\"operator\": " << quoted(binaryOperator(binary.op)) << ",\n";
        indent(output, depth + 1) << "\"left\": ";
        writeExpression(output, *binary.left, depth + 1);
        output << ",\n";
        indent(output, depth + 1) << "\"right\": ";
        writeExpression(output, *binary.right, depth + 1);
        output << '\n';
        indent(output, depth) << '}';
        return;
    }
    case Expression::Kind::Unary: {
        const auto& unary = static_cast<const UnaryExpression&>(expression);
        output << "{\n";
        indent(output, depth + 1) << "\"type\": \"UnaryExpression\",\n";
        indent(output, depth + 1) << "\"operator\": " << quoted(unaryOperator(unary.op)) << ",\n";
        indent(output, depth + 1) << "\"operand\": ";
        writeExpression(output, *unary.operand, depth + 1);
        output << '\n';
        indent(output, depth) << '}';
        return;
    }
    case Expression::Kind::Function: {
        const auto& call = static_cast<const FunctionCall&>(expression);
        output << "{\n";
        indent(output, depth + 1) << "\"type\": \"FunctionCall\",\n";
        indent(output, depth + 1) << "\"function\": " << quoted(functionName(call.function)) << ",\n";
        indent(output, depth + 1) << "\"argument\": ";
        if (call.starArgument) {
            output << quoted("*");
        } else {
            writeExpression(output, *call.argument, depth + 1);
        }
        output << '\n';
        indent(output, depth) << '}';
        return;
    }
    }
    throw std::logic_error("Unknown expression kind");
}

void writeTable(std::ostream& output, const TableReference& table, int depth) {
    output << "{\n";
    indent(output, depth + 1) << "\"type\": \"TableReference\",\n";
    indent(output, depth + 1) << "\"name\": " << quoted(table.name) << ",\n";
    indent(output, depth + 1) << "\"alias\": " << optionalString(table.alias) << '\n';
    indent(output, depth) << '}';
}

} // namespace

void writeAstJson(std::ostream& output, const SelectStatement& statement) {
    output << "{\n  \"type\": \"SelectStatement\",\n  \"select\": [\n";
    for (std::size_t i = 0; i < statement.items.size(); ++i) {
        const auto& item = statement.items[i];
        indent(output, 2) << "{\n";
        indent(output, 3) << "\"type\": \"SelectItem\",\n";
        if (item.isWildcard) {
            indent(output, 3) << "\"wildcard\": \"*\",\n";
            indent(output, 3) << "\"qualifier\": " << optionalString(item.qualifier) << "\n";
        } else {
            indent(output, 3) << "\"expression\": ";
            writeExpression(output, *item.expression, 3);
            output << ",\n";
            indent(output, 3) << "\"alias\": " << optionalString(item.alias) << '\n';
        }
        indent(output, 2) << '}' << (i + 1 == statement.items.size() ? "\n" : ",\n");
    }
    output << "  ],\n  \"from\": ";
    writeTable(output, statement.from, 1);

    output << ",\n  \"join\": ";
    if (statement.join) {
        output << "{\n    \"type\": \"JoinClause\",\n    \"table\": ";
        writeTable(output, statement.join->table, 2);
        output << ",\n    \"condition\": ";
        writeExpression(output, *statement.join->condition, 2);
        output << "\n  }";
    } else {
        output << "null";
    }

    output << ",\n  \"where\": ";
    if (statement.where) {
        output << "{\n    \"type\": \"WhereClause\",\n    \"predicate\": ";
        writeExpression(output, *statement.where->predicate, 2);
        output << "\n  }";
    } else {
        output << "null";
    }

    output << ",\n  \"groupBy\": ";
    if (statement.groupBy) {
        output << "{\n    \"type\": \"GroupByClause\",\n    \"columns\": [\n";
        const auto& columns = statement.groupBy->columns;
        for (std::size_t i = 0; i < columns.size(); ++i) {
            indent(output, 3);
            writeColumn(output, columns[i], 3);
            output << (i + 1 == columns.size() ? "\n" : ",\n");
        }
        output << "    ]\n  }";
    } else {
        output << "null";
    }

    output << ",\n  \"orderBy\": ";
    if (statement.orderBy) {
        output << "{\n    \"type\": \"OrderByClause\",\n    \"items\": [\n";
        const auto& items = statement.orderBy->items;
        for (std::size_t i = 0; i < items.size(); ++i) {
            indent(output, 3) << "{\n";
            indent(output, 4) << "\"column\": ";
            writeColumn(output, items[i].column, 4);
            output << ",\n";
            indent(output, 4) << "\"direction\": \""
                              << (items[i].direction == OrderDirection::Descending ? "DESC" : "ASC")
                              << "\"\n";
            indent(output, 3) << '}' << (i + 1 == items.size() ? "\n" : ",\n");
        }
        output << "    ]\n  }";
    } else {
        output << "null";
    }

    output << ",\n  \"limit\": ";
    if (statement.limit) {
        output << "{\n    \"type\": \"LimitClause\",\n    \"count\": "
               << statement.limit->count << "\n  }";
    } else {
        output << "null";
    }
    output << "\n}\n";
}

std::string printAstJson(const SelectStatement& statement) {
    std::ostringstream output;
    writeAstJson(output, statement);
    return output.str();
}

} // namespace optiq

#include "optiq/tac.hpp"

#include <sstream>

namespace optiq {

namespace {

std::string binaryOperatorText(BinaryExpression::Operator op) {
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

std::string literalOperand(const LiteralExpression& literal) {
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

class TacGenerator {
public:
    TacProgram run(const Program& program) {
        for (const auto& statement : program.statements) {
            emitStatement(*statement);
        }
        return tac;
    }

private:
    TacProgram tac;
    int nextTemp = 1;
    int nextLabel = 1;

    std::string newTemp() { return "t" + std::to_string(nextTemp++); }
    std::string newLabel() { return "L" + std::to_string(nextLabel++); }

    void emit(const std::string& instruction) {
        tac.instructions.push_back(instruction);
    }

    // Returns the operand (variable, literal, or temporary) holding the value.
    std::string emitExpression(const Expression& expression) {
        switch (expression.kind) {
        case Expression::Kind::Identifier:
            return static_cast<const IdentifierExpression&>(expression).name;
        case Expression::Kind::Literal:
            return literalOperand(static_cast<const LiteralExpression&>(expression));
        case Expression::Kind::Unary: {
            const auto& node = static_cast<const UnaryExpression&>(expression);
            const std::string operand = emitExpression(*node.operand);
            const std::string temp = newTemp();
            const char* op = node.op == UnaryExpression::Operator::Negate ? "-" : "!";
            emit(temp + " = " + op + operand);
            return temp;
        }
        case Expression::Kind::Binary: {
            const auto& node = static_cast<const BinaryExpression&>(expression);
            const std::string left = emitExpression(*node.left);
            const std::string right = emitExpression(*node.right);
            const std::string temp = newTemp();
            emit(temp + " = " + left + " " + binaryOperatorText(node.op) + " " + right);
            return temp;
        }
        }
        return "?";
    }

    void emitStatement(const Statement& statement) {
        switch (statement.kind) {
        case Statement::Kind::Declaration: {
            const auto& node = static_cast<const DeclarationStatement&>(statement);
            if (node.initializer) {
                emit(node.name + " = " + emitExpression(*node.initializer));
            }
            break;
        }
        case Statement::Kind::Assignment: {
            const auto& node = static_cast<const AssignmentStatement&>(statement);
            emit(node.name + " = " + emitExpression(*node.value));
            break;
        }
        case Statement::Kind::Print: {
            const auto& node = static_cast<const PrintStatement&>(statement);
            emit("print " + emitExpression(*node.expression));
            break;
        }
        case Statement::Kind::Block: {
            const auto& node = static_cast<const BlockStatement&>(statement);
            for (const auto& child : node.statements) {
                emitStatement(*child);
            }
            break;
        }
        case Statement::Kind::If: {
            const auto& node = static_cast<const IfStatement&>(statement);
            const std::string condition = emitExpression(*node.condition);
            const std::string elseLabel = newLabel();
            emit("ifFalse " + condition + " goto " + elseLabel);
            for (const auto& child : node.thenBlock->statements) {
                emitStatement(*child);
            }
            if (node.elseBlock) {
                const std::string endLabel = newLabel();
                emit("goto " + endLabel);
                emit(elseLabel + ":");
                for (const auto& child : node.elseBlock->statements) {
                    emitStatement(*child);
                }
                emit(endLabel + ":");
            } else {
                emit(elseLabel + ":");
            }
            break;
        }
        case Statement::Kind::While: {
            const auto& node = static_cast<const WhileStatement&>(statement);
            const std::string startLabel = newLabel();
            const std::string endLabel = newLabel();
            emit(startLabel + ":");
            const std::string condition = emitExpression(*node.condition);
            emit("ifFalse " + condition + " goto " + endLabel);
            for (const auto& child : node.body->statements) {
                emitStatement(*child);
            }
            emit("goto " + startLabel);
            emit(endLabel + ":");
            break;
        }
        }
    }
};

} // namespace

TacProgram generateTac(const Program& program) {
    return TacGenerator().run(program);
}

std::string tacToString(const TacProgram& tac) {
    std::ostringstream text;
    for (const auto& instruction : tac.instructions) {
        text << instruction << '\n';
    }
    return text.str();
}

} // namespace optiq

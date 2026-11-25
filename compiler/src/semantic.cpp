#include "optiq/semantic.hpp"

#include <string>

#include "optiq/symbol_table.hpp"

namespace optiq {

namespace {

// `Unknown` marks expressions whose type could not be determined because an
// error was already reported; Unknown suppresses cascading follow-up errors.
enum class Type { Int, Float, Bool, Unknown };

bool isNumeric(Type type) { return type == Type::Int || type == Type::Float; }

std::string typeName(Type type) {
    switch (type) {
    case Type::Int: return "int";
    case Type::Float: return "float";
    case Type::Bool: return "bool";
    case Type::Unknown: return "unknown";
    }
    return "unknown";
}

// Assignment compatibility: exact match, plus int -> float widening.
bool assignable(Type target, Type source) {
    if (source == Type::Unknown || target == Type::Unknown) return true;
    if (target == source) return true;
    return target == Type::Float && source == Type::Int;
}

class Analyzer {
public:
    SemanticResult run(const Program& program) {
        table.enterScope(); // global scope
        for (const auto& statement : program.statements) {
            visit(*statement);
        }
        table.exitScope();
        return result;
    }

private:
    SymbolTable table;
    SemanticResult result;

    void error(SourceLocation location, const std::string& message) {
        result.errors.push_back(SemanticError{location, message});
    }

    void visit(const Statement& statement) {
        switch (statement.kind) {
        case Statement::Kind::Declaration:
            visitDeclaration(static_cast<const DeclarationStatement&>(statement));
            break;
        case Statement::Kind::Assignment:
            visitAssignment(static_cast<const AssignmentStatement&>(statement));
            break;
        case Statement::Kind::If: {
            const auto& node = static_cast<const IfStatement&>(statement);
            requireBool(*node.condition, "if condition");
            visitBlock(*node.thenBlock);
            if (node.elseBlock) visitBlock(*node.elseBlock);
            break;
        }
        case Statement::Kind::While: {
            const auto& node = static_cast<const WhileStatement&>(statement);
            requireBool(*node.condition, "while condition");
            visitBlock(*node.body);
            break;
        }
        case Statement::Kind::Print:
            typeOf(*static_cast<const PrintStatement&>(statement).expression);
            break;
        case Statement::Kind::Block:
            visitBlock(static_cast<const BlockStatement&>(statement));
            break;
        }
    }

    void visitBlock(const BlockStatement& block) {
        table.enterScope();
        for (const auto& statement : block.statements) {
            visit(*statement);
        }
        table.exitScope();
    }

    void visitDeclaration(const DeclarationStatement& node) {
        const Type declared = node.type == ValueType::Int    ? Type::Int
                              : node.type == ValueType::Float ? Type::Float
                                                              : Type::Bool;
        if (!table.declare(node.name, node.type, node.location.line)) {
            error(node.location, "Redeclaration of '" + node.name +
                                     "' in the same scope");
        }
        if (node.initializer) {
            const Type source = typeOf(*node.initializer);
            if (!assignable(declared, source)) {
                error(node.location, "Type mismatch: cannot initialize '" +
                                         valueTypeName(node.type) + " " + node.name +
                                         "' with an expression of type " +
                                         typeName(source));
            }
        }
    }

    void visitAssignment(const AssignmentStatement& node) {
        const Symbol* symbol = table.lookup(node.name);
        if (!symbol) {
            error(node.location, "Undeclared variable '" + node.name +
                                     "' (not visible in this scope)");
            return;
        }
        const Type target = symbol->type == ValueType::Int    ? Type::Int
                            : symbol->type == ValueType::Float ? Type::Float
                                                               : Type::Bool;
        const Type source = typeOf(*node.value);
        if (!assignable(target, source)) {
            error(node.location, "Invalid assignment: cannot assign " +
                                     typeName(source) + " to '" +
                                     valueTypeName(symbol->type) + " " + node.name +
                                     "'");
        }
    }

    void requireBool(const Expression& condition, const char* what) {
        const Type type = typeOf(condition);
        if (type != Type::Bool && type != Type::Unknown) {
            error(condition.location, std::string("A ") + what +
                                          " must be bool, not " + typeName(type));
        }
    }

    Type typeOf(const Expression& expression) {
        switch (expression.kind) {
        case Expression::Kind::Identifier: {
            const auto& node = static_cast<const IdentifierExpression&>(expression);
            const Symbol* symbol = table.lookup(node.name);
            if (!symbol) {
                error(expression.location, "Undeclared variable '" + node.name +
                                               "' (not visible in this scope)");
                return Type::Unknown;
            }
            switch (symbol->type) {
            case ValueType::Int: return Type::Int;
            case ValueType::Float: return Type::Float;
            case ValueType::Bool: return Type::Bool;
            }
            return Type::Unknown;
        }
        case Expression::Kind::Literal: {
            switch (static_cast<const LiteralExpression&>(expression).literalKind) {
            case LiteralExpression::LiteralKind::Integer: return Type::Int;
            case LiteralExpression::LiteralKind::Float: return Type::Float;
            case LiteralExpression::LiteralKind::Boolean: return Type::Bool;
            }
            return Type::Unknown;
        }
        case Expression::Kind::Unary: {
            const auto& node = static_cast<const UnaryExpression&>(expression);
            const Type operand = typeOf(*node.operand);
            if (operand == Type::Unknown) return Type::Unknown;
            if (node.op == UnaryExpression::Operator::Negate) {
                if (!isNumeric(operand)) {
                    error(expression.location,
                          "Unary '-' requires a numeric operand, not " +
                              typeName(operand));
                    return Type::Unknown;
                }
                return operand;
            }
            // Logical NOT
            if (operand != Type::Bool) {
                error(expression.location, "Operator '!' requires a bool operand, not " +
                                               typeName(operand));
                return Type::Unknown;
            }
            return Type::Bool;
        }
        case Expression::Kind::Binary: {
            const auto& node = static_cast<const BinaryExpression&>(expression);
            const Type left = typeOf(*node.left);
            const Type right = typeOf(*node.right);
            if (left == Type::Unknown || right == Type::Unknown) return Type::Unknown;

            switch (node.op) {
            case BinaryExpression::Operator::Add:
            case BinaryExpression::Operator::Subtract:
            case BinaryExpression::Operator::Multiply:
            case BinaryExpression::Operator::Divide:
                if (!isNumeric(left) || !isNumeric(right)) {
                    error(expression.location,
                          "Arithmetic operator requires numeric operands, not " +
                              typeName(left) + " and " + typeName(right));
                    return Type::Unknown;
                }
                return (left == Type::Float || right == Type::Float) ? Type::Float
                                                                     : Type::Int;
            case BinaryExpression::Operator::Modulo:
                if (left != Type::Int || right != Type::Int) {
                    error(expression.location,
                          "Operator '%' requires int operands, not " + typeName(left) +
                              " and " + typeName(right));
                    return Type::Unknown;
                }
                return Type::Int;
            case BinaryExpression::Operator::Less:
            case BinaryExpression::Operator::Greater:
            case BinaryExpression::Operator::LessEqual:
            case BinaryExpression::Operator::GreaterEqual:
                if (!isNumeric(left) || !isNumeric(right)) {
                    error(expression.location,
                          "Relational operator requires numeric operands, not " +
                              typeName(left) + " and " + typeName(right));
                    return Type::Unknown;
                }
                return Type::Bool;
            case BinaryExpression::Operator::Equal:
            case BinaryExpression::Operator::NotEqual:
                if (left != right && !(isNumeric(left) && isNumeric(right))) {
                    error(expression.location,
                          "Equality operator requires matching operand types, not " +
                              typeName(left) + " and " + typeName(right));
                    return Type::Unknown;
                }
                return Type::Bool;
            case BinaryExpression::Operator::And:
            case BinaryExpression::Operator::Or:
                if (left != Type::Bool || right != Type::Bool) {
                    error(expression.location,
                          "Logical operator requires bool operands, not " +
                              typeName(left) + " and " + typeName(right));
                    return Type::Unknown;
                }
                return Type::Bool;
            }
            return Type::Unknown;
        }
        }
        return Type::Unknown;
    }
};

} // namespace

SemanticResult analyze(const Program& program) {
    return Analyzer().run(program);
}

} // namespace optiq

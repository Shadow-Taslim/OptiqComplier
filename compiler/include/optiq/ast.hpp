#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace optiq {

struct SourceLocation {
    int line = 1;
    int column = 1;
    int offset = 0;
};

enum class DataType { Int, Float, Bool };
std::string dataTypeName(DataType type);

class Expression {
public:
    enum class Kind { Variable, Literal, Unary, Binary };

    Expression(Kind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Expression() = default;

    Expression(const Expression&) = delete;
    Expression& operator=(const Expression&) = delete;
    Expression(Expression&&) = default;
    Expression& operator=(Expression&&) = default;

    Kind kind;
    SourceLocation location;
};

class VariableExpression final : public Expression {
public:
    VariableExpression(SourceLocation location, std::string name)
        : Expression(Kind::Variable, location), name(std::move(name)) {}

    std::string name;
};

class LiteralExpression final : public Expression {
public:
    using Value = std::variant<std::int64_t, double, bool>;

    LiteralExpression(SourceLocation location, std::int64_t value)
        : Expression(Kind::Literal, location), type(DataType::Int), value(value) {}
    LiteralExpression(SourceLocation location, double value)
        : Expression(Kind::Literal, location), type(DataType::Float), value(value) {}
    LiteralExpression(SourceLocation location, bool value)
        : Expression(Kind::Literal, location), type(DataType::Bool), value(value) {}

    DataType type;
    Value value;
};

class UnaryExpression final : public Expression {
public:
    enum class Operator { Not, Negate };

    UnaryExpression(SourceLocation location, Operator op, std::unique_ptr<Expression> operand)
        : Expression(Kind::Unary, location), op(op), operand(std::move(operand)) {}

    Operator op;
    std::unique_ptr<Expression> operand;
};

class BinaryExpression final : public Expression {
public:
    enum class Operator {
        Or, And,
        Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
        Add, Subtract, Multiply, Divide, Modulo
    };

    BinaryExpression(SourceLocation location, Operator op,
                     std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : Expression(Kind::Binary, location), op(op), left(std::move(left)),
          right(std::move(right)) {}

    Operator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

using StatementList = std::vector<std::unique_ptr<class Statement>>;

class Statement {
public:
    enum class Kind { Declaration, Assignment, Print, If, While, Block };

    Statement(Kind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Statement() = default;

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&&) = default;
    Statement& operator=(Statement&&) = default;

    Kind kind;
    SourceLocation location;
};

class DeclarationStatement final : public Statement {
public:
    DeclarationStatement(SourceLocation location, DataType declaredType, std::string name)
        : Statement(Kind::Declaration, location), declaredType(declaredType),
          name(std::move(name)) {}

    DataType declaredType;
    std::string name;
};

class AssignmentStatement final : public Statement {
public:
    AssignmentStatement(SourceLocation location, std::string name,
                        std::unique_ptr<Expression> value)
        : Statement(Kind::Assignment, location), name(std::move(name)),
          value(std::move(value)) {}

    std::string name;
    std::unique_ptr<Expression> value;
};

class PrintStatement final : public Statement {
public:
    PrintStatement(SourceLocation location, std::unique_ptr<Expression> expression)
        : Statement(Kind::Print, location), expression(std::move(expression)) {}

    std::unique_ptr<Expression> expression;
};

class IfStatement final : public Statement {
public:
    IfStatement(SourceLocation location, std::unique_ptr<Expression> condition,
                StatementList thenBlock, StatementList elseBlock = {}, bool hasElse = false)
        : Statement(Kind::If, location), condition(std::move(condition)),
          thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)), hasElse(hasElse) {}

    std::unique_ptr<Expression> condition;
    StatementList thenBlock;
    StatementList elseBlock;
    bool hasElse;
};

class WhileStatement final : public Statement {
public:
    WhileStatement(SourceLocation location, std::unique_ptr<Expression> condition,
                   StatementList body)
        : Statement(Kind::While, location), condition(std::move(condition)),
          body(std::move(body)) {}

    std::unique_ptr<Expression> condition;
    StatementList body;
};

class BlockStatement final : public Statement {
public:
    BlockStatement(SourceLocation location, StatementList statements)
        : Statement(Kind::Block, location), statements(std::move(statements)) {}

    StatementList statements;
};

struct Program {
    StatementList statements;
};

} // namespace optiq

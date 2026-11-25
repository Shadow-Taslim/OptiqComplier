#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace optiq {

struct SourceLocation {
    int line = 1;
    int column = 1;
    std::size_t offset = 0;
};

// The three data types of the course-specified language.
enum class ValueType { Int, Float, Bool };

std::string valueTypeName(ValueType type);

// ------------------------------ Expressions ------------------------------

struct Expression {
    enum class Kind { Identifier, Literal, Binary, Unary };

    explicit Expression(SourceLocation location, Kind kind)
        : location(location), kind(kind) {}
    virtual ~Expression() = default;

    SourceLocation location;
    Kind kind;
};

struct IdentifierExpression : Expression {
    IdentifierExpression(SourceLocation location, std::string name)
        : Expression(location, Kind::Identifier), name(std::move(name)) {}

    std::string name;
};

struct LiteralExpression : Expression {
    enum class LiteralKind { Integer, Float, Boolean };

    LiteralExpression(SourceLocation location, std::int64_t value)
        : Expression(location, Kind::Literal), literalKind(LiteralKind::Integer),
          value(value) {}
    LiteralExpression(SourceLocation location, double value)
        : Expression(location, Kind::Literal), literalKind(LiteralKind::Float),
          value(value) {}
    LiteralExpression(SourceLocation location, bool value)
        : Expression(location, Kind::Literal), literalKind(LiteralKind::Boolean),
          value(value) {}

    LiteralKind literalKind;
    std::variant<std::int64_t, double, bool> value;
};

struct BinaryExpression : Expression {
    enum class Operator {
        Add, Subtract, Multiply, Divide, Modulo,
        Less, Greater, LessEqual, GreaterEqual, Equal, NotEqual,
        And, Or
    };

    BinaryExpression(SourceLocation location, Operator op,
                     std::unique_ptr<Expression> left,
                     std::unique_ptr<Expression> right)
        : Expression(location, Kind::Binary), op(op), left(std::move(left)),
          right(std::move(right)) {}

    Operator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

struct UnaryExpression : Expression {
    enum class Operator { Negate, Not };

    UnaryExpression(SourceLocation location, Operator op,
                    std::unique_ptr<Expression> operand)
        : Expression(location, Kind::Unary), op(op), operand(std::move(operand)) {}

    Operator op;
    std::unique_ptr<Expression> operand;
};

// ------------------------------ Statements -------------------------------

struct Statement {
    enum class Kind { Declaration, Assignment, If, While, Print, Block };

    explicit Statement(SourceLocation location, Kind kind)
        : location(location), kind(kind) {}
    virtual ~Statement() = default;

    SourceLocation location;
    Kind kind;
};

struct BlockStatement : Statement {
    explicit BlockStatement(SourceLocation location)
        : Statement(location, Kind::Block) {}

    std::vector<std::unique_ptr<Statement>> statements;
};

struct DeclarationStatement : Statement {
    DeclarationStatement(SourceLocation location, ValueType type, std::string name,
                         std::unique_ptr<Expression> initializer = nullptr)
        : Statement(location, Kind::Declaration), type(type),
          name(std::move(name)), initializer(std::move(initializer)) {}

    ValueType type;
    std::string name;
    std::unique_ptr<Expression> initializer; // nullptr for `int x;`
};

struct AssignmentStatement : Statement {
    AssignmentStatement(SourceLocation location, std::string name,
                        std::unique_ptr<Expression> value)
        : Statement(location, Kind::Assignment), name(std::move(name)),
          value(std::move(value)) {}

    std::string name;
    std::unique_ptr<Expression> value;
};

struct IfStatement : Statement {
    IfStatement(SourceLocation location, std::unique_ptr<Expression> condition,
                std::unique_ptr<BlockStatement> thenBlock,
                std::unique_ptr<BlockStatement> elseBlock = nullptr)
        : Statement(location, Kind::If), condition(std::move(condition)),
          thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}

    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> thenBlock;
    std::unique_ptr<BlockStatement> elseBlock; // nullptr when no else
};

struct WhileStatement : Statement {
    WhileStatement(SourceLocation location, std::unique_ptr<Expression> condition,
                   std::unique_ptr<BlockStatement> body)
        : Statement(location, Kind::While), condition(std::move(condition)),
          body(std::move(body)) {}

    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStatement> body;
};

struct PrintStatement : Statement {
    PrintStatement(SourceLocation location, std::unique_ptr<Expression> expression)
        : Statement(location, Kind::Print), expression(std::move(expression)) {}

    std::unique_ptr<Expression> expression;
};

struct Program {
    std::vector<std::unique_ptr<Statement>> statements;
};

} // namespace optiq

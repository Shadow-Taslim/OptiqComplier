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
    int offset = 0;
};

class Expression {
public:
    enum class Kind { Column, Literal, Binary, Unary, Function, Query };

    Expression(Kind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Expression() = default;

    Expression(const Expression&) = delete;
    Expression& operator=(const Expression&) = delete;
    Expression(Expression&&) = default;
    Expression& operator=(Expression&&) = default;

    Kind kind;
    SourceLocation location;
};

class ColumnReference final : public Expression {
public:
    ColumnReference(SourceLocation location, std::string name,
                    std::optional<std::string> qualifier = std::nullopt)
        : Expression(Kind::Column, location), name(std::move(name)),
          qualifier(std::move(qualifier)) {}

    std::string name;
    std::optional<std::string> qualifier;
};

class LiteralExpression final : public Expression {
public:
    enum class LiteralKind { Integer, Decimal, String, Boolean };
    using Value = std::variant<std::int64_t, double, std::string, bool>;

    LiteralExpression(SourceLocation location, std::int64_t value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::Integer), value(value) {}
    LiteralExpression(SourceLocation location, double value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::Decimal), value(value) {}
    LiteralExpression(SourceLocation location, std::string value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::String),
          value(std::move(value)) {}
    LiteralExpression(SourceLocation location, bool value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::Boolean), value(value) {}

    LiteralKind literalKind;
    Value value;
};

class BinaryExpression final : public Expression {
public:
    enum class Operator {
        Or, And,
        Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
        Add, Subtract, Multiply, Divide
    };

    BinaryExpression(SourceLocation location, Operator op,
                     std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
        : Expression(Kind::Binary, location), op(op), left(std::move(left)),
          right(std::move(right)) {}

    Operator op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
};

class UnaryExpression final : public Expression {
public:
    enum class Operator { Not, Positive, Negative };

    UnaryExpression(SourceLocation location, Operator op, std::unique_ptr<Expression> operand)
        : Expression(Kind::Unary, location), op(op), operand(std::move(operand)) {}

    Operator op;
    std::unique_ptr<Expression> operand;
};

class FunctionCall final : public Expression {
public:
    enum class Function { Count, Sum, Average, Minimum, Maximum };

    FunctionCall(SourceLocation location, Function function,
                 std::unique_ptr<Expression> argument)
        : Expression(Kind::Function, location), function(function),
          argument(std::move(argument)), starArgument(false) {}

    static std::unique_ptr<FunctionCall> countStar(SourceLocation location) {
        auto call = std::make_unique<FunctionCall>(location, Function::Count, nullptr);
        call->starArgument = true;
        return call;
    }

    Function function;
    std::unique_ptr<Expression> argument;
    bool starArgument;
};

struct SelectItem {
    static SelectItem wildcard(SourceLocation location,
                               std::optional<std::string> qualifier = std::nullopt) {
        SelectItem item;
        item.location = location;
        item.isWildcard = true;
        item.qualifier = std::move(qualifier);
        return item;
    }

    static SelectItem expressionItem(SourceLocation location,
                                     std::unique_ptr<Expression> expression,
                                     std::optional<std::string> alias = std::nullopt) {
        SelectItem item;
        item.location = location;
        item.expression = std::move(expression);
        item.alias = std::move(alias);
        return item;
    }

    SourceLocation location;
    std::unique_ptr<Expression> expression;
    std::optional<std::string> alias;
    bool isWildcard = false;
    std::optional<std::string> qualifier;
};

struct TableReference {
    SourceLocation location;
    std::string name;
    std::optional<std::string> alias;
};

struct JoinClause {
    SourceLocation location;
    TableReference table;
    std::unique_ptr<Expression> condition;
};

struct WhereClause {
    SourceLocation location;
    std::unique_ptr<Expression> predicate;
};

struct GroupByClause {
    SourceLocation location;
    std::vector<ColumnReference> columns;
};

enum class OrderDirection { Ascending, Descending };

struct OrderByItem {
    SourceLocation location;
    ColumnReference column;
    OrderDirection direction = OrderDirection::Ascending;
};

struct OrderByClause {
    SourceLocation location;
    std::vector<OrderByItem> items;
};

struct LimitClause {
    SourceLocation location;
    std::int64_t count = 0;
};

struct SelectStatement {
    SourceLocation location;
    std::vector<SelectItem> items;
    TableReference from;
    std::optional<JoinClause> join;
    std::optional<WhereClause> where;
    std::optional<GroupByClause> groupBy;
    std::optional<OrderByClause> orderBy;
    std::optional<LimitClause> limit;
};

// A SELECT query used as an expression; its runtime value is a table.
class QueryExpression final : public Expression {
public:
    QueryExpression(SourceLocation location, std::unique_ptr<SelectStatement> query)
        : Expression(Kind::Query, location), query(std::move(query)) {}

    std::unique_ptr<SelectStatement> query;
};

// Optiq mini-language statements. A Program is a sequence of statements.
class Statement {
public:
    enum class Kind { Let, Assign, Print, If, While, Query };

    Statement(Kind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Statement() = default;

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&&) = default;
    Statement& operator=(Statement&&) = default;

    Kind kind;
    SourceLocation location;
};

class LetStatement final : public Statement {
public:
    LetStatement(SourceLocation location, std::string name,
                 std::unique_ptr<Expression> initializer)
        : Statement(Kind::Let, location), name(std::move(name)),
          initializer(std::move(initializer)) {}

    std::string name;
    std::unique_ptr<Expression> initializer;
};

class AssignStatement final : public Statement {
public:
    AssignStatement(SourceLocation location, std::string name,
                    std::unique_ptr<Expression> value)
        : Statement(Kind::Assign, location), name(std::move(name)),
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
    IfStatement(SourceLocation location, std::unique_ptr<Expression> condition)
        : Statement(Kind::If, location), condition(std::move(condition)) {}

    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> thenBlock;
    // Empty when there is no else part; an else-if chain is a single IfStatement entry.
    std::vector<std::unique_ptr<Statement>> elseBlock;
};

class WhileStatement final : public Statement {
public:
    WhileStatement(SourceLocation location, std::unique_ptr<Expression> condition)
        : Statement(Kind::While, location), condition(std::move(condition)) {}

    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;
};

class QueryStatement final : public Statement {
public:
    QueryStatement(SourceLocation location, std::unique_ptr<SelectStatement> query)
        : Statement(Kind::Query, location), query(std::move(query)) {}

    std::unique_ptr<SelectStatement> query;
};

struct Program {
    std::vector<std::unique_ptr<Statement>> statements;
};

} // namespace optiq

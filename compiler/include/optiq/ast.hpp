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
    enum class Kind { Column, Literal, Binary, Unary, Function };

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
    enum class LiteralKind { Integer, Decimal, String };
    using Value = std::variant<std::int64_t, double, std::string>;

    LiteralExpression(SourceLocation location, std::int64_t value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::Integer), value(value) {}
    LiteralExpression(SourceLocation location, double value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::Decimal), value(value) {}
    LiteralExpression(SourceLocation location, std::string value)
        : Expression(Kind::Literal, location), literalKind(LiteralKind::String),
          value(std::move(value)) {}

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

} // namespace optiq

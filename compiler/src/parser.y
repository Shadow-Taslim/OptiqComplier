%define api.pure full
%define parse.error verbose
%locations
%start input

%lex-param { optiq::TokenStream& tokens }
%parse-param { optiq::TokenStream& tokens }
%parse-param { optiq::ProgramParseResult& result }

%code requires {
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "optiq/ast.hpp"
namespace optiq {
class TokenStream;
struct ProgramParseResult;
}
}

%code {
#include <memory>
#include <utility>
#include "optiq/lexer.hpp"
#include "optiq/parser.hpp"
#include "token_stream.hpp"

using namespace optiq;

int yylex(YYSTYPE* value, YYLTYPE* location, optiq::TokenStream& tokens);
void yyerror(YYLTYPE* location, optiq::TokenStream& tokens,
             optiq::ProgramParseResult& result, const char* message);

SourceLocation toLocation(const YYLTYPE& location) {
    return SourceLocation{location.first_line, location.first_column, 0};
}

Expression* makeBinary(const YYLTYPE& location, BinaryExpression::Operator op,
                       Expression* left, Expression* right) {
    return new BinaryExpression(toLocation(location), op,
                                std::unique_ptr<Expression>(left),
                                std::unique_ptr<Expression>(right));
}
}

%union {
    std::int64_t integerValue;
    double decimalValue;
    std::string* string;
    std::optional<std::string>* optionalString;
    optiq::Expression* expression;
    optiq::ColumnReference* column;
    optiq::BinaryExpression::Operator binaryOperator;
    optiq::FunctionCall::Function function;
    optiq::SelectItem* selectItem;
    std::vector<optiq::SelectItem>* selectItems;
    optiq::TableReference* table;
    optiq::JoinClause* join;
    optiq::WhereClause* where;
    optiq::GroupByClause* groupBy;
    std::vector<optiq::ColumnReference>* columns;
    optiq::OrderDirection orderDirection;
    optiq::OrderByItem* orderItem;
    std::vector<optiq::OrderByItem>* orderItems;
    optiq::OrderByClause* orderBy;
    optiq::LimitClause* limit;
    optiq::SelectStatement* statement;
    optiq::Statement* stmt;
    std::vector<std::unique_ptr<optiq::Statement>>* stmtList;
    optiq::Program* program;
}

%token <string> IDENTIFIER STRING_LITERAL
%token <integerValue> INTEGER_LITERAL
%token <decimalValue> DECIMAL_LITERAL

%token SELECT FROM AS INNER JOIN ON WHERE GROUP BY ORDER ASC DESC LIMIT
%token AND OR NOT COUNT SUM AVG MIN MAX
%token LET IF ELSE WHILE PRINT TRUE FALSE
%token LEFT_PAREN RIGHT_PAREN LEFT_BRACE RIGHT_BRACE COMMA SEMICOLON DOT
%token EQUAL NOT_EQUAL LESS LESS_EQUAL GREATER GREATER_EQUAL
%token PLUS MINUS STAR SLASH

%type <string> identifier
%type <optionalString> alias_opt
%type <selectItem> select_item
%type <selectItems> select_list
%type <table> table_reference
%type <join> join_opt
%type <where> where_opt
%type <groupBy> group_by_opt
%type <columns> column_list
%type <orderDirection> direction_opt
%type <orderItem> order_item
%type <orderItems> order_list
%type <orderBy> order_by_opt
%type <limit> limit_opt
%type <statement> select_statement
%type <program> program
%type <stmt> statement let_statement assign_statement print_statement
%type <stmt> if_statement while_statement query_statement
%type <stmtList> statement_list block else_part
%type <expression> expression or_expression and_expression not_expression
%type <expression> predicate additive_expression multiplicative_expression
%type <expression> unary_expression primary_expression function_call
%type <column> column_reference
%type <binaryOperator> comparison_operator
%type <function> aggregate_function

%destructor { delete $$; } <string> <optionalString> <expression> <column>
%destructor { delete $$; } <selectItem> <selectItems> <table> <join> <where>
%destructor { delete $$; } <groupBy> <columns> <orderItem> <orderItems> <orderBy>
%destructor { delete $$; } <limit> <statement> <stmt> <stmtList> <program>

%%

input:
    program
        { result.program.reset($1); }
    ;

program:
    statement_list
        {
            auto* parsed = new Program;
            parsed->statements = std::move(*$1);
            delete $1;
            $$ = parsed;
        }
    ;

statement_list:
    %empty
        { $$ = new std::vector<std::unique_ptr<Statement>>; }
    | statement_list statement
        {
            $$ = $1;
            $$->push_back(std::unique_ptr<Statement>($2));
        }
    ;

statement:
    let_statement      { $$ = $1; }
    | assign_statement { $$ = $1; }
    | print_statement  { $$ = $1; }
    | if_statement     { $$ = $1; }
    | while_statement  { $$ = $1; }
    | query_statement  { $$ = $1; }
    ;

let_statement:
    LET identifier EQUAL expression SEMICOLON
        {
            $$ = new LetStatement(toLocation(@$), std::move(*$2),
                                  std::unique_ptr<Expression>($4));
            delete $2;
        }
    | LET identifier EQUAL select_statement SEMICOLON
        {
            $$ = new LetStatement(toLocation(@$), std::move(*$2),
                std::unique_ptr<Expression>(
                    new QueryExpression(toLocation(@4),
                                        std::unique_ptr<SelectStatement>($4))));
            delete $2;
        }
    ;

assign_statement:
    IDENTIFIER EQUAL expression SEMICOLON
        {
            $$ = new AssignStatement(toLocation(@$), std::move(*$1),
                                     std::unique_ptr<Expression>($3));
            delete $1;
        }
    | IDENTIFIER EQUAL select_statement SEMICOLON
        {
            $$ = new AssignStatement(toLocation(@$), std::move(*$1),
                std::unique_ptr<Expression>(
                    new QueryExpression(toLocation(@3),
                                        std::unique_ptr<SelectStatement>($3))));
            delete $1;
        }
    ;

print_statement:
    PRINT LEFT_PAREN expression RIGHT_PAREN SEMICOLON
        { $$ = new PrintStatement(toLocation(@$), std::unique_ptr<Expression>($3)); }
    ;

if_statement:
    IF LEFT_PAREN expression RIGHT_PAREN block else_part
        {
            auto* parsed = new IfStatement(toLocation(@$), std::unique_ptr<Expression>($3));
            parsed->thenBlock = std::move(*$5);
            parsed->elseBlock = std::move(*$6);
            delete $5;
            delete $6;
            $$ = parsed;
        }
    ;

else_part:
    %empty
        { $$ = new std::vector<std::unique_ptr<Statement>>; }
    | ELSE block
        { $$ = $2; }
    | ELSE if_statement
        {
            $$ = new std::vector<std::unique_ptr<Statement>>;
            $$->push_back(std::unique_ptr<Statement>($2));
        }
    ;

while_statement:
    WHILE LEFT_PAREN expression RIGHT_PAREN block
        {
            auto* parsed = new WhileStatement(toLocation(@$), std::unique_ptr<Expression>($3));
            parsed->body = std::move(*$5);
            delete $5;
            $$ = parsed;
        }
    ;

block:
    LEFT_BRACE statement_list RIGHT_BRACE
        { $$ = $2; }
    ;

query_statement:
    select_statement SEMICOLON
        { $$ = new QueryStatement(toLocation(@$), std::unique_ptr<SelectStatement>($1)); }
    ;

select_statement:
    SELECT select_list FROM table_reference join_opt where_opt group_by_opt order_by_opt limit_opt
        {
            auto* parsed = new SelectStatement;
            parsed->location = toLocation(@$);
            parsed->items = std::move(*$2);
            parsed->from = std::move(*$4);
            if ($5) parsed->join = std::move(*$5);
            if ($6) parsed->where = std::move(*$6);
            if ($7) parsed->groupBy = std::move(*$7);
            if ($8) parsed->orderBy = std::move(*$8);
            if ($9) parsed->limit = std::move(*$9);
            delete $2;
            delete $4;
            delete $5;
            delete $6;
            delete $7;
            delete $8;
            delete $9;
            $$ = parsed;
        }
    ;

select_list:
    select_item
        {
            $$ = new std::vector<SelectItem>;
            $$->push_back(std::move(*$1));
            delete $1;
        }
    | select_list COMMA select_item
        {
            $$ = $1;
            $$->push_back(std::move(*$3));
            delete $3;
        }
    ;

select_item:
    STAR
        { $$ = new SelectItem(SelectItem::wildcard(toLocation(@$))); }
    | expression alias_opt
        {
            $$ = new SelectItem(SelectItem::expressionItem(
                toLocation(@$), std::unique_ptr<Expression>($1), std::move(*$2)));
            delete $2;
        }
    ;

alias_opt:
    %empty
        { $$ = new std::optional<std::string>(std::nullopt); }
    | AS identifier
        { $$ = new std::optional<std::string>(std::move(*$2)); delete $2; }
    | identifier
        { $$ = new std::optional<std::string>(std::move(*$1)); delete $1; }
    ;

table_reference:
    identifier alias_opt
        {
            $$ = new TableReference{toLocation(@$), std::move(*$1), std::move(*$2)};
            delete $1;
            delete $2;
        }
    ;

join_opt:
    %empty
        { $$ = nullptr; }
    | JOIN table_reference ON expression
        {
            $$ = new JoinClause{toLocation(@$), std::move(*$2),
                                std::unique_ptr<Expression>($4)};
            delete $2;
        }
    | INNER JOIN table_reference ON expression
        {
            $$ = new JoinClause{toLocation(@$), std::move(*$3),
                                std::unique_ptr<Expression>($5)};
            delete $3;
        }
    ;

where_opt:
    %empty
        { $$ = nullptr; }
    | WHERE expression
        { $$ = new WhereClause{toLocation(@$), std::unique_ptr<Expression>($2)}; }
    ;

group_by_opt:
    %empty
        { $$ = nullptr; }
    | GROUP BY column_list
        {
            $$ = new GroupByClause{toLocation(@$), std::move(*$3)};
            delete $3;
        }
    ;

column_list:
    column_reference
        {
            $$ = new std::vector<ColumnReference>;
            $$->push_back(std::move(*$1));
            delete $1;
        }
    | column_list COMMA column_reference
        {
            $$ = $1;
            $$->push_back(std::move(*$3));
            delete $3;
        }
    ;

order_by_opt:
    %empty
        { $$ = nullptr; }
    | ORDER BY order_list
        {
            $$ = new OrderByClause{toLocation(@$), std::move(*$3)};
            delete $3;
        }
    ;

order_list:
    order_item
        {
            $$ = new std::vector<OrderByItem>;
            $$->push_back(std::move(*$1));
            delete $1;
        }
    | order_list COMMA order_item
        {
            $$ = $1;
            $$->push_back(std::move(*$3));
            delete $3;
        }
    ;

order_item:
    column_reference direction_opt
        {
            $$ = new OrderByItem{toLocation(@$), std::move(*$1), $2};
            delete $1;
        }
    ;

direction_opt:
    %empty
        { $$ = OrderDirection::Ascending; }
    | ASC
        { $$ = OrderDirection::Ascending; }
    | DESC
        { $$ = OrderDirection::Descending; }
    ;

limit_opt:
    %empty
        { $$ = nullptr; }
    | LIMIT INTEGER_LITERAL
        { $$ = new LimitClause{toLocation(@$), $2}; }
    ;

identifier:
    IDENTIFIER
        { $$ = $1; }
    ;

expression:
    or_expression
        { $$ = $1; }
    ;

or_expression:
    and_expression
        { $$ = $1; }
    | or_expression OR and_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Or, $1, $3); }
    ;

and_expression:
    not_expression
        { $$ = $1; }
    | and_expression AND not_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::And, $1, $3); }
    ;

not_expression:
    predicate
        { $$ = $1; }
    | NOT not_expression
        {
            $$ = new UnaryExpression(toLocation(@$), UnaryExpression::Operator::Not,
                                     std::unique_ptr<Expression>($2));
        }
    ;

predicate:
    additive_expression
        { $$ = $1; }
    | additive_expression comparison_operator additive_expression
        { $$ = makeBinary(@$, $2, $1, $3); }
    ;

comparison_operator:
    EQUAL          { $$ = BinaryExpression::Operator::Equal; }
    | NOT_EQUAL    { $$ = BinaryExpression::Operator::NotEqual; }
    | LESS         { $$ = BinaryExpression::Operator::Less; }
    | LESS_EQUAL   { $$ = BinaryExpression::Operator::LessEqual; }
    | GREATER      { $$ = BinaryExpression::Operator::Greater; }
    | GREATER_EQUAL { $$ = BinaryExpression::Operator::GreaterEqual; }
    ;

additive_expression:
    multiplicative_expression
        { $$ = $1; }
    | additive_expression PLUS multiplicative_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Add, $1, $3); }
    | additive_expression MINUS multiplicative_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Subtract, $1, $3); }
    ;

multiplicative_expression:
    unary_expression
        { $$ = $1; }
    | multiplicative_expression STAR unary_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Multiply, $1, $3); }
    | multiplicative_expression SLASH unary_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Divide, $1, $3); }
    ;

unary_expression:
    primary_expression
        { $$ = $1; }
    | PLUS unary_expression
        {
            $$ = new UnaryExpression(toLocation(@$), UnaryExpression::Operator::Positive,
                                     std::unique_ptr<Expression>($2));
        }
    | MINUS unary_expression
        {
            $$ = new UnaryExpression(toLocation(@$), UnaryExpression::Operator::Negative,
                                     std::unique_ptr<Expression>($2));
        }
    ;

primary_expression:
    column_reference
        { $$ = $1; }
    | INTEGER_LITERAL
        { $$ = new LiteralExpression(toLocation(@$), $1); }
    | DECIMAL_LITERAL
        { $$ = new LiteralExpression(toLocation(@$), $1); }
    | STRING_LITERAL
        {
            $$ = new LiteralExpression(toLocation(@$), std::move(*$1));
            delete $1;
        }
    | TRUE
        { $$ = new LiteralExpression(toLocation(@$), true); }
    | FALSE
        { $$ = new LiteralExpression(toLocation(@$), false); }
    | function_call
        { $$ = $1; }
    | LEFT_PAREN expression RIGHT_PAREN
        { $$ = $2; }
    ;

column_reference:
    identifier
        {
            $$ = new ColumnReference(toLocation(@$), std::move(*$1));
            delete $1;
        }
    | identifier DOT identifier
        {
            $$ = new ColumnReference(toLocation(@$), std::move(*$3), std::move(*$1));
            delete $1;
            delete $3;
        }
    ;

function_call:
    COUNT LEFT_PAREN STAR RIGHT_PAREN
        { $$ = FunctionCall::countStar(toLocation(@$)).release(); }
    | COUNT LEFT_PAREN expression RIGHT_PAREN
        {
            $$ = new FunctionCall(toLocation(@$), FunctionCall::Function::Count,
                                  std::unique_ptr<Expression>($3));
        }
    | aggregate_function LEFT_PAREN expression RIGHT_PAREN
        {
            $$ = new FunctionCall(toLocation(@$), $1, std::unique_ptr<Expression>($3));
        }
    ;

aggregate_function:
    SUM { $$ = FunctionCall::Function::Sum; }
    | AVG { $$ = FunctionCall::Function::Average; }
    | MIN { $$ = FunctionCall::Function::Minimum; }
    | MAX { $$ = FunctionCall::Function::Maximum; }
    ;

%%

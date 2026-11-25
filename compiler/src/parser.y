%define api.pure full
%define parse.error verbose
%locations
%start input

%lex-param { optiq::TokenStream& tokens }
%parse-param { optiq::TokenStream& tokens }
%parse-param { optiq::ParseResult& result }

%code requires {
#include <cstdint>
#include <memory>
#include <string>
#include "optiq/ast.hpp"
namespace optiq {
class TokenStream;
struct ParseResult;
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
             optiq::ParseResult& result, const char* message);

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
    double floatValue;
    std::string* string;
    optiq::DataType dataType;
    optiq::Expression* expression;
    optiq::Statement* statement;
    optiq::StatementList* statements;
}

%token <string> IDENTIFIER
%token <integerValue> INTEGER_LITERAL
%token <floatValue> FLOAT_LITERAL
%token INT FLOAT BOOL IF ELSE WHILE PRINT TRUE FALSE
%token LEFT_PAREN RIGHT_PAREN LEFT_BRACE RIGHT_BRACE SEMICOLON
%token ASSIGN EQUAL NOT_EQUAL LESS LESS_EQUAL GREATER GREATER_EQUAL
%token PLUS MINUS STAR SLASH PERCENT AND OR NOT

%type <dataType> type
%type <expression> expression or_expression and_expression comparison_expression
%type <expression> additive_expression multiplicative_expression unary_expression primary_expression
%type <statement> statement declaration assignment print_statement if_statement while_statement
%type <statements> statement_list block

%destructor { delete $$; } <string> <expression> <statement> <statements>

%%

input:
    statement_list
        {
            result.program = std::make_unique<Program>();
            result.program->statements = std::move(*$1);
            delete $1;
        }
    ;

statement_list:
    %empty
        { $$ = new StatementList; }
    | statement_list statement
        {
            $$ = $1;
            if ($2 != nullptr) $$->push_back(std::unique_ptr<Statement>($2));
        }
    ;

statement:
    declaration     { $$ = $1; }
    | assignment    { $$ = $1; }
    | print_statement { $$ = $1; }
    | if_statement  { $$ = $1; }
    | while_statement { $$ = $1; }
    | block
        { $$ = new BlockStatement(toLocation(@$), std::move(*$1)); delete $1; }
    | error SEMICOLON
        { $$ = nullptr; yyerrok; }
    ;

declaration:
    type IDENTIFIER SEMICOLON
        {
            $$ = new DeclarationStatement(toLocation(@$), $1, std::move(*$2));
            delete $2;
        }
    ;

type:
    INT   { $$ = DataType::Int; }
    | FLOAT { $$ = DataType::Float; }
    | BOOL  { $$ = DataType::Bool; }
    ;

assignment:
    IDENTIFIER ASSIGN expression SEMICOLON
        {
            $$ = new AssignmentStatement(toLocation(@$), std::move(*$1),
                                         std::unique_ptr<Expression>($3));
            delete $1;
        }
    ;

print_statement:
    PRINT LEFT_PAREN expression RIGHT_PAREN SEMICOLON
        { $$ = new PrintStatement(toLocation(@$), std::unique_ptr<Expression>($3)); }
    ;

if_statement:
    IF LEFT_PAREN expression RIGHT_PAREN block
        {
            $$ = new IfStatement(toLocation(@$), std::unique_ptr<Expression>($3),
                                 std::move(*$5));
            delete $5;
        }
    | IF LEFT_PAREN expression RIGHT_PAREN block ELSE block
        {
            $$ = new IfStatement(toLocation(@$), std::unique_ptr<Expression>($3),
                                 std::move(*$5), std::move(*$7), true);
            delete $5;
            delete $7;
        }
    | IF LEFT_PAREN expression RIGHT_PAREN block ELSE if_statement
        {
            StatementList elseBlock;
            elseBlock.push_back(std::unique_ptr<Statement>($7));
            $$ = new IfStatement(toLocation(@$), std::unique_ptr<Expression>($3),
                                 std::move(*$5), std::move(elseBlock), true);
            delete $5;
        }
    ;

while_statement:
    WHILE LEFT_PAREN expression RIGHT_PAREN block
        {
            $$ = new WhileStatement(toLocation(@$), std::unique_ptr<Expression>($3),
                                    std::move(*$5));
            delete $5;
        }
    ;

block:
    LEFT_BRACE statement_list RIGHT_BRACE
        { $$ = $2; }
    ;

expression:
    or_expression { $$ = $1; }
    ;

or_expression:
    and_expression
        { $$ = $1; }
    | or_expression OR and_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Or, $1, $3); }
    ;

and_expression:
    comparison_expression
        { $$ = $1; }
    | and_expression AND comparison_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::And, $1, $3); }
    ;

comparison_expression:
    additive_expression
        { $$ = $1; }
    | additive_expression EQUAL additive_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Equal, $1, $3); }
    | additive_expression NOT_EQUAL additive_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::NotEqual, $1, $3); }
    | additive_expression LESS additive_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Less, $1, $3); }
    | additive_expression LESS_EQUAL additive_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::LessEqual, $1, $3); }
    | additive_expression GREATER additive_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Greater, $1, $3); }
    | additive_expression GREATER_EQUAL additive_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::GreaterEqual, $1, $3); }
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
    | multiplicative_expression PERCENT unary_expression
        { $$ = makeBinary(@$, BinaryExpression::Operator::Modulo, $1, $3); }
    ;

unary_expression:
    primary_expression
        { $$ = $1; }
    | MINUS unary_expression
        {
            $$ = new UnaryExpression(toLocation(@$), UnaryExpression::Operator::Negate,
                                     std::unique_ptr<Expression>($2));
        }
    | NOT unary_expression
        {
            $$ = new UnaryExpression(toLocation(@$), UnaryExpression::Operator::Not,
                                     std::unique_ptr<Expression>($2));
        }
    ;

primary_expression:
    IDENTIFIER
        {
            $$ = new VariableExpression(toLocation(@$), std::move(*$1));
            delete $1;
        }
    | INTEGER_LITERAL
        { $$ = new LiteralExpression(toLocation(@$), $1); }
    | FLOAT_LITERAL
        { $$ = new LiteralExpression(toLocation(@$), $1); }
    | TRUE
        { $$ = new LiteralExpression(toLocation(@$), true); }
    | FALSE
        { $$ = new LiteralExpression(toLocation(@$), false); }
    | LEFT_PAREN expression RIGHT_PAREN
        { $$ = $2; }
    ;

%%

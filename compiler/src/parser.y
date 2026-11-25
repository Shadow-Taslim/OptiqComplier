%require "3.0"
%defines
%define api.pure full
%locations
%define parse.error verbose
%parse-param { optiq::TokenStream& tokens }
%parse-param { optiq::ParseResult& result }
%lex-param { optiq::TokenStream& tokens }

%code requires {
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "optiq/ast.hpp"
#include "optiq/lexer.hpp"
#include "optiq/parser.hpp"

namespace optiq {
class TokenStream;
}
}

%code provides {
#include <utility>

#include "token_stream.hpp"

int yylex(YYSTYPE* value, YYLTYPE* location, optiq::TokenStream& tokens);
void yyerror(YYLTYPE* location, optiq::TokenStream& tokens,
             optiq::ParseResult& result, const char* message);

// Adapts Bison's default YYLTYPE to the AST's SourceLocation.
static optiq::SourceLocation loc(const YYLTYPE& location) {
    return optiq::SourceLocation{location.first_line, location.first_column, 0};
}

template <typename T>
std::unique_ptr<T> own(T* pointer) {
    return std::unique_ptr<T>(pointer);
}
}

%union {
    std::string* string;
    std::int64_t integerValue;
    double floatValue;
    bool boolValue;
    optiq::ValueType valueType;
    optiq::Expression* expression;
    optiq::Statement* statement;
    optiq::BlockStatement* block;
    std::vector<std::unique_ptr<optiq::Statement>>* statementList;
}

%token <string> IDENTIFIER "identifier"
%token <integerValue> INTEGER_LITERAL "integer literal"
%token <floatValue> FLOAT_LITERAL "float literal"
%token <boolValue> TRUE "true" FALSE "false"
%token INT "int" FLOAT "float" BOOL "bool"
%token IF "if" ELSE "else" WHILE "while" PRINT "print"
%token PLUS "+" MINUS "-" STAR "*" SLASH "/" PERCENT "%"
%token LESS "<" GREATER ">" LESS_EQUAL "<=" GREATER_EQUAL ">="
%token EQUAL_EQUAL "==" NOT_EQUAL "!="
%token AND_AND "&&" OR_OR "||" BANG "!" ASSIGN "="
%token LEFT_PAREN "(" RIGHT_PAREN ")" LEFT_BRACE "{" RIGHT_BRACE "}"
%token SEMICOLON ";"

%type <valueType> type
%type <expression> expression logic_or logic_and equality relational additive
%type <expression> multiplicative unary primary
%type <statement> statement declaration assignment if_statement while_statement
%type <statement> print_statement
%type <block> block
%type <statementList> statement_list

%destructor { delete $$; } <string>
%destructor { delete $$; } <expression>
%destructor { delete $$; } <statement>
%destructor { delete $$; } <block>
%destructor { delete $$; } <statementList>

%%

input:
    statement_list {
        auto program = std::make_unique<optiq::Program>();
        for (auto& statement : *$1) {
            if (statement) program->statements.push_back(std::move(statement));
        }
        delete $1;
        result.program = std::move(program);
    }
;

statement_list:
    %empty { $$ = new std::vector<std::unique_ptr<optiq::Statement>>(); }
  | statement_list statement {
        $$ = $1;
        $$->push_back(own($2)); // nullptr (recovered error) is filtered in `input`
    }
;

statement:
    declaration
  | assignment
  | if_statement
  | while_statement
  | print_statement
  | block { $$ = $1; }
  | error SEMICOLON {
        // Error recovery: skip to the next semicolon and continue parsing,
        // so one bad statement does not hide later errors.
        yyerrok;
        $$ = nullptr;
    }
;

type:
    INT   { $$ = optiq::ValueType::Int; }
  | FLOAT { $$ = optiq::ValueType::Float; }
  | BOOL  { $$ = optiq::ValueType::Bool; }
;

declaration:
    type IDENTIFIER SEMICOLON {
        $$ = new optiq::DeclarationStatement(loc(@1), $1, *$2);
        delete $2;
    }
  | type IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = new optiq::DeclarationStatement(loc(@1), $1, *$2, own($4));
        delete $2;
    }
;

assignment:
    IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = new optiq::AssignmentStatement(loc(@1), *$1, own($3));
        delete $1;
    }
;

if_statement:
    IF LEFT_PAREN expression RIGHT_PAREN block {
        $$ = new optiq::IfStatement(loc(@1), own($3), own($5));
    }
  | IF LEFT_PAREN expression RIGHT_PAREN block ELSE block {
        $$ = new optiq::IfStatement(loc(@1), own($3), own($5), own($7));
    }
;

while_statement:
    WHILE LEFT_PAREN expression RIGHT_PAREN block {
        $$ = new optiq::WhileStatement(loc(@1), own($3), own($5));
    }
;

print_statement:
    PRINT expression SEMICOLON {
        $$ = new optiq::PrintStatement(loc(@1), own($2));
    }
;

block:
    LEFT_BRACE statement_list RIGHT_BRACE {
        auto block = std::make_unique<optiq::BlockStatement>(loc(@1));
        for (auto& statement : *$2) {
            if (statement) block->statements.push_back(std::move(statement));
        }
        delete $2;
        $$ = block.release();
    }
;

expression: logic_or;

logic_or:
    logic_and
  | logic_or OR_OR logic_and {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Or,
                                         own($1), own($3));
    }
;

logic_and:
    equality
  | logic_and AND_AND equality {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::And,
                                         own($1), own($3));
    }
;

equality:
    relational
  | equality EQUAL_EQUAL relational {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Equal,
                                         own($1), own($3));
    }
  | equality NOT_EQUAL relational {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::NotEqual,
                                         own($1), own($3));
    }
;

relational:
    additive
  | relational LESS additive {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Less,
                                         own($1), own($3));
    }
  | relational GREATER additive {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Greater,
                                         own($1), own($3));
    }
  | relational LESS_EQUAL additive {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::LessEqual,
                                         own($1), own($3));
    }
  | relational GREATER_EQUAL additive {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::GreaterEqual,
                                         own($1), own($3));
    }
;

additive:
    multiplicative
  | additive PLUS multiplicative {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Add,
                                         own($1), own($3));
    }
  | additive MINUS multiplicative {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Subtract,
                                         own($1), own($3));
    }
;

multiplicative:
    unary
  | multiplicative STAR unary {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Multiply,
                                         own($1), own($3));
    }
  | multiplicative SLASH unary {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Divide,
                                         own($1), own($3));
    }
  | multiplicative PERCENT unary {
        $$ = new optiq::BinaryExpression(loc(@2), optiq::BinaryExpression::Operator::Modulo,
                                         own($1), own($3));
    }
;

unary:
    primary
  | MINUS unary {
        $$ = new optiq::UnaryExpression(loc(@1), optiq::UnaryExpression::Operator::Negate,
                                        own($2));
    }
  | BANG unary {
        $$ = new optiq::UnaryExpression(loc(@1), optiq::UnaryExpression::Operator::Not,
                                        own($2));
    }
;

primary:
    IDENTIFIER {
        $$ = new optiq::IdentifierExpression(loc(@1), *$1);
        delete $1;
    }
  | INTEGER_LITERAL { $$ = new optiq::LiteralExpression(loc(@1), $1); }
  | FLOAT_LITERAL   { $$ = new optiq::LiteralExpression(loc(@1), $1); }
  | TRUE            { $$ = new optiq::LiteralExpression(loc(@1), true); }
  | FALSE           { $$ = new optiq::LiteralExpression(loc(@1), false); }
  | LEFT_PAREN expression RIGHT_PAREN { $$ = $2; }
;

%%

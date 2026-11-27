%require "3.8"
%skeleton "lalr1.cc"
%defines
%define api.namespace {optiq::cpp::syntax}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%define parse.lac full
%locations
%parse-param { optiq::cpp::ParserContext& parserContext }
%lex-param { optiq::cpp::ParserContext& parserContext }

%code requires {
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "optiq/cpp/ast.hpp"

namespace optiq::cpp {
struct ParserContext;
}
}

%code {
#include "parser_context.hpp"

namespace optiq::cpp::syntax {

Parser::symbol_type yylex(optiq::cpp::ParserContext& context);

static optiq::cpp::SourceLocation loc(const Parser::location_type& location) {
    return optiq::cpp::SourceLocation{
        static_cast<std::size_t>(location.begin.line),
        static_cast<std::size_t>(location.begin.column),
        0
    };
}

static optiq::cpp::NodeList single(optiq::cpp::NodePtr node) {
    optiq::cpp::NodeList nodes;
    nodes.push_back(std::move(node));
    return nodes;
}

static void append(optiq::cpp::NodeList& destination,
                   optiq::cpp::NodeList source) {
    for (auto& node : source) destination.push_back(std::move(node));
}

} // namespace optiq::cpp::syntax
}

%token END 0 "end of file"
%token <std::string> IDENTIFIER "identifier"
%token <std::string> INTEGER_LITERAL "integer literal"
%token <std::string> FLOATING_LITERAL "floating literal"
%token <std::string> CHARACTER_LITERAL "character literal"
%token <std::string> STRING_LITERAL "string literal"
%token <std::string> PREPROCESSOR "preprocessor directive"

%token INT "int" FLOAT "float" DOUBLE "double" CHAR "char" BOOL "bool" VOID "void"
%token IF "if" ELSE "else" WHILE "while" FOR "for" DO "do" RETURN "return"
%token BREAK "break" CONTINUE "continue"
%token TRUE "true" FALSE "false"
%token USING "using" NAMESPACE "namespace"
%token STD "std" COUT "cout" CIN "cin" ENDL "endl"

%token PLUS "+" MINUS "-" STAR "*" SLASH "/" PERCENT "%"
%token LESS "<" GREATER ">" LESS_EQUAL "<=" GREATER_EQUAL ">="
%token EQUAL_EQUAL "==" NOT_EQUAL "!="
%token LOGICAL_AND "&&" LOGICAL_OR "||" LOGICAL_NOT "!"
%token ASSIGN "=" PLUS_ASSIGN "+=" MINUS_ASSIGN "-=" STAR_ASSIGN "*=" SLASH_ASSIGN "/="
%token INCREMENT "++" DECREMENT "--"
%token BITWISE_AND "&" BITWISE_OR "|" BITWISE_XOR "^" BITWISE_NOT "~"
%token SHIFT_LEFT "<<" SHIFT_RIGHT ">>" ARROW "->" DOT "." SCOPE "::"
%token LEFT_PAREN "(" RIGHT_PAREN ")" LEFT_BRACE "{" RIGHT_BRACE "}"
%token LEFT_BRACKET "[" RIGHT_BRACKET "]" SEMICOLON ";" COMMA "," COLON ":"

%precedence LOWER_THAN_ELSE
%precedence ELSE

%type <std::string> type_specifier unary_operator assignment_operator
%type <std::string> output_stream input_stream
%type <optiq::cpp::NodePtr> external_function_declaration external_function_definition
%type <optiq::cpp::NodePtr> parameter init_declarator compound_statement statement
%type <optiq::cpp::NodePtr> expression_statement selection_statement iteration_statement
%type <optiq::cpp::NodePtr> jump_statement for_declaration for_initialization for_condition
%type <optiq::cpp::NodePtr> for_update stream_output_statement stream_input_statement
%type <optiq::cpp::NodePtr> stream_output stream_input stream_element
%type <optiq::cpp::NodePtr> expression assignment_expression logical_or_expression
%type <optiq::cpp::NodePtr> logical_and_expression bitwise_or_expression
%type <optiq::cpp::NodePtr> bitwise_xor_expression bitwise_and_expression equality_expression
%type <optiq::cpp::NodePtr> relational_expression additive_expression multiplicative_expression
%type <optiq::cpp::NodePtr> unary_expression postfix_expression primary_expression
%type <optiq::cpp::NodeList> external_declaration_list external_declaration
%type <optiq::cpp::NodeList> variable_declaration init_declarator_list block_item_list
%type <optiq::cpp::NodeList> block_item parameter_list_opt parameter_list argument_list_opt
%type <optiq::cpp::NodeList> argument_list

%start translation_unit

%%

translation_unit:
    external_declaration_list {
        auto program = std::make_unique<optiq::cpp::ProgramNode>();
        program->declarations = std::move($1);
        if (!program->declarations.empty()) {
            program->location = program->declarations.front()->location;
        }
        parserContext.program = std::move(program);
    }
;

external_declaration_list:
    %empty { $$ = optiq::cpp::NodeList{}; }
  | external_declaration_list external_declaration {
        $$ = std::move($1);
        append($$, std::move($2));
    }
;

external_declaration:
    PREPROCESSOR {
        $$ = single(std::make_unique<optiq::cpp::PreprocessorNode>(loc(@1), std::move($1)));
    }
  | USING NAMESPACE STD SEMICOLON {
        $$ = single(std::make_unique<optiq::cpp::UsingNamespaceNode>(loc(@1), "std"));
    }
  | variable_declaration { $$ = std::move($1); }
  | external_function_declaration { $$ = single(std::move($1)); }
  | external_function_definition { $$ = single(std::move($1)); }
;

type_specifier:
    INT    { $$ = "int"; }
  | FLOAT  { $$ = "float"; }
  | DOUBLE { $$ = "double"; }
  | CHAR   { $$ = "char"; }
  | BOOL   { $$ = "bool"; }
  | VOID   { $$ = "void"; }
;

variable_declaration:
    type_specifier init_declarator_list SEMICOLON {
        if ($1 == "void") {
            parserContext.addSyntaxError(loc(@1), "void is not a valid variable type");
        }
        for (auto& node : $2) {
            static_cast<optiq::cpp::VariableDeclarationNode*>(node.get())->typeName = $1;
        }
        $$ = std::move($2);
    }
;

init_declarator_list:
    init_declarator { $$ = single(std::move($1)); }
  | init_declarator_list COMMA init_declarator {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

init_declarator:
    IDENTIFIER {
        $$ = std::make_unique<optiq::cpp::VariableDeclarationNode>(loc(@1), std::move($1));
    }
  | IDENTIFIER ASSIGN assignment_expression {
        $$ = std::make_unique<optiq::cpp::VariableDeclarationNode>(
            loc(@1), std::move($1), std::move($3));
    }
;

external_function_declaration:
    type_specifier IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN SEMICOLON {
        $$ = std::make_unique<optiq::cpp::FunctionDeclarationNode>(
            loc(@1), std::move($1), std::move($2), std::move($4));
    }
;

external_function_definition:
    type_specifier IDENTIFIER LEFT_PAREN parameter_list_opt RIGHT_PAREN compound_statement {
        $$ = std::make_unique<optiq::cpp::FunctionDefinitionNode>(
            loc(@1), std::move($1), std::move($2), std::move($4), std::move($6));
    }
;

parameter_list_opt:
    %empty { $$ = optiq::cpp::NodeList{}; }
  | parameter_list { $$ = std::move($1); }
;

parameter_list:
    parameter { $$ = single(std::move($1)); }
  | parameter_list COMMA parameter {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

parameter:
    type_specifier IDENTIFIER {
        if ($1 == "void") {
            parserContext.addSyntaxError(loc(@1), "void is not a valid parameter type");
        }
        $$ = std::make_unique<optiq::cpp::ParameterNode>(
            loc(@1), std::move($1), std::move($2));
    }
;

compound_statement:
    LEFT_BRACE block_item_list RIGHT_BRACE {
        auto block = std::make_unique<optiq::cpp::BlockStatementNode>(loc(@1));
        block->statements = std::move($2);
        $$ = std::move(block);
    }
;

block_item_list:
    %empty { $$ = optiq::cpp::NodeList{}; }
  | block_item_list block_item {
        $$ = std::move($1);
        append($$, std::move($2));
    }
;

block_item:
    variable_declaration { $$ = std::move($1); }
  | statement { $$ = single(std::move($1)); }
;

statement:
    expression_statement { $$ = std::move($1); }
  | compound_statement { $$ = std::move($1); }
  | selection_statement { $$ = std::move($1); }
  | iteration_statement { $$ = std::move($1); }
  | jump_statement { $$ = std::move($1); }
  | stream_output_statement { $$ = std::move($1); }
  | stream_input_statement { $$ = std::move($1); }
  | error SEMICOLON {
        yyerrok;
        $$ = std::make_unique<optiq::cpp::ErrorStatementNode>(loc(@$));
    }
;

expression_statement:
    expression SEMICOLON {
        $$ = std::make_unique<optiq::cpp::ExpressionStatementNode>(loc(@$), std::move($1));
    }
  | SEMICOLON { $$ = std::make_unique<optiq::cpp::EmptyStatementNode>(loc(@1)); }
;

selection_statement:
    IF LEFT_PAREN expression RIGHT_PAREN statement %prec LOWER_THAN_ELSE {
        $$ = std::make_unique<optiq::cpp::IfStatementNode>(
            loc(@1), std::move($3), std::move($5));
    }
  | IF LEFT_PAREN expression RIGHT_PAREN statement ELSE statement {
        $$ = std::make_unique<optiq::cpp::IfStatementNode>(
            loc(@1), std::move($3), std::move($5), std::move($7));
    }
;

iteration_statement:
    WHILE LEFT_PAREN expression RIGHT_PAREN statement {
        $$ = std::make_unique<optiq::cpp::WhileStatementNode>(
            loc(@1), std::move($3), std::move($5));
    }
  | FOR LEFT_PAREN for_initialization for_condition for_update RIGHT_PAREN statement {
        $$ = std::make_unique<optiq::cpp::ForStatementNode>(
            loc(@1), std::move($3), std::move($4), std::move($5), std::move($7));
    }
  | DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON {
        $$ = std::make_unique<optiq::cpp::DoWhileStatementNode>(
            loc(@1), std::move($2), std::move($5));
    }
;

for_initialization:
    SEMICOLON { $$ = nullptr; }
  | for_declaration { $$ = std::move($1); }
  | expression SEMICOLON {
        $$ = std::make_unique<optiq::cpp::ExpressionStatementNode>(loc(@$), std::move($1));
    }
;

for_declaration:
    type_specifier init_declarator SEMICOLON {
        if ($1 == "void") {
            parserContext.addSyntaxError(loc(@1), "void is not a valid for-loop variable type");
        }
        $$ = std::move($2);
        static_cast<optiq::cpp::VariableDeclarationNode*>($$.get())->typeName = std::move($1);
    }
;

for_condition:
    SEMICOLON { $$ = nullptr; }
  | expression SEMICOLON { $$ = std::move($1); }
;

for_update:
    %empty { $$ = nullptr; }
  | expression { $$ = std::move($1); }
;

jump_statement:
    RETURN SEMICOLON {
        $$ = std::make_unique<optiq::cpp::ReturnStatementNode>(loc(@1));
    }
  | RETURN expression SEMICOLON {
        $$ = std::make_unique<optiq::cpp::ReturnStatementNode>(loc(@1), std::move($2));
    }
;

stream_output_statement:
    stream_output SEMICOLON { $$ = std::move($1); }
;

output_stream:
    COUT { $$ = "cout"; }
  | STD SCOPE COUT { $$ = "std::cout"; }
;

stream_output:
    output_stream SHIFT_LEFT stream_element {
        $$ = std::make_unique<optiq::cpp::StreamOutputNode>(
            loc(@$), std::move($1), std::move($3));
    }
  | stream_output SHIFT_LEFT stream_element {
        $$ = std::move($1);
        static_cast<optiq::cpp::StreamOutputNode*>($$.get())->values.push_back(std::move($3));
    }
;

stream_element:
    assignment_expression { $$ = std::move($1); }
  | ENDL {
        $$ = std::make_unique<optiq::cpp::IdentifierNode>(loc(@1), "endl");
    }
  | STD SCOPE ENDL {
        $$ = std::make_unique<optiq::cpp::IdentifierNode>(loc(@1), "std::endl");
    }
;

stream_input_statement:
    stream_input SEMICOLON { $$ = std::move($1); }
;

input_stream:
    CIN { $$ = "cin"; }
  | STD SCOPE CIN { $$ = "std::cin"; }
;

stream_input:
    input_stream SHIFT_RIGHT IDENTIFIER {
        auto target = std::make_unique<optiq::cpp::IdentifierNode>(loc(@3), std::move($3));
        $$ = std::make_unique<optiq::cpp::StreamInputNode>(
            loc(@$), std::move($1), std::move(target));
    }
  | stream_input SHIFT_RIGHT IDENTIFIER {
        $$ = std::move($1);
        static_cast<optiq::cpp::StreamInputNode*>($$.get())->targets.push_back(
            std::make_unique<optiq::cpp::IdentifierNode>(loc(@3), std::move($3)));
    }
;

expression:
    assignment_expression { $$ = std::move($1); }
;

assignment_expression:
    logical_or_expression { $$ = std::move($1); }
  | IDENTIFIER assignment_operator assignment_expression {
        auto target = std::make_unique<optiq::cpp::IdentifierNode>(loc(@1), std::move($1));
        $$ = std::make_unique<optiq::cpp::AssignmentExpressionNode>(
            loc(@$), std::move($2), std::move(target), std::move($3));
    }
;

assignment_operator:
    ASSIGN       { $$ = "="; }
  | PLUS_ASSIGN  { $$ = "+="; }
  | MINUS_ASSIGN { $$ = "-="; }
  | STAR_ASSIGN  { $$ = "*="; }
  | SLASH_ASSIGN { $$ = "/="; }
;

logical_or_expression:
    logical_and_expression { $$ = std::move($1); }
  | logical_or_expression LOGICAL_OR logical_and_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "||", std::move($1), std::move($3));
    }
;

logical_and_expression:
    bitwise_or_expression { $$ = std::move($1); }
  | logical_and_expression LOGICAL_AND bitwise_or_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "&&", std::move($1), std::move($3));
    }
;

bitwise_or_expression:
    bitwise_xor_expression { $$ = std::move($1); }
  | bitwise_or_expression BITWISE_OR bitwise_xor_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "|", std::move($1), std::move($3));
    }
;

bitwise_xor_expression:
    bitwise_and_expression { $$ = std::move($1); }
  | bitwise_xor_expression BITWISE_XOR bitwise_and_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "^", std::move($1), std::move($3));
    }
;

bitwise_and_expression:
    equality_expression { $$ = std::move($1); }
  | bitwise_and_expression BITWISE_AND equality_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "&", std::move($1), std::move($3));
    }
;

equality_expression:
    relational_expression { $$ = std::move($1); }
  | equality_expression EQUAL_EQUAL relational_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "==", std::move($1), std::move($3));
    }
  | equality_expression NOT_EQUAL relational_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "!=", std::move($1), std::move($3));
    }
;

relational_expression:
    additive_expression { $$ = std::move($1); }
  | relational_expression LESS additive_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "<", std::move($1), std::move($3));
    }
  | relational_expression GREATER additive_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), ">", std::move($1), std::move($3));
    }
  | relational_expression LESS_EQUAL additive_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "<=", std::move($1), std::move($3));
    }
  | relational_expression GREATER_EQUAL additive_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), ">=", std::move($1), std::move($3));
    }
;

additive_expression:
    multiplicative_expression { $$ = std::move($1); }
  | additive_expression PLUS multiplicative_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "+", std::move($1), std::move($3));
    }
  | additive_expression MINUS multiplicative_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "-", std::move($1), std::move($3));
    }
;

multiplicative_expression:
    unary_expression { $$ = std::move($1); }
  | multiplicative_expression STAR unary_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "*", std::move($1), std::move($3));
    }
  | multiplicative_expression SLASH unary_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "/", std::move($1), std::move($3));
    }
  | multiplicative_expression PERCENT unary_expression {
        $$ = std::make_unique<optiq::cpp::BinaryExpressionNode>(
            loc(@$), "%", std::move($1), std::move($3));
    }
;

unary_expression:
    postfix_expression { $$ = std::move($1); }
  | INCREMENT unary_expression {
        $$ = std::make_unique<optiq::cpp::UnaryExpressionNode>(
            loc(@$), "++", std::move($2), true);
    }
  | DECREMENT unary_expression {
        $$ = std::make_unique<optiq::cpp::UnaryExpressionNode>(
            loc(@$), "--", std::move($2), true);
    }
  | unary_operator unary_expression {
        $$ = std::make_unique<optiq::cpp::UnaryExpressionNode>(
            loc(@$), std::move($1), std::move($2), true);
    }
;

unary_operator:
    PLUS        { $$ = "+"; }
  | MINUS       { $$ = "-"; }
  | LOGICAL_NOT { $$ = "!"; }
  | BITWISE_NOT { $$ = "~"; }
;

postfix_expression:
    primary_expression { $$ = std::move($1); }
  | postfix_expression LEFT_PAREN argument_list_opt RIGHT_PAREN {
        $$ = std::make_unique<optiq::cpp::FunctionCallNode>(
            loc(@$), std::move($1), std::move($3));
    }
  | postfix_expression INCREMENT {
        $$ = std::make_unique<optiq::cpp::UnaryExpressionNode>(
            loc(@$), "++", std::move($1), false);
    }
  | postfix_expression DECREMENT {
        $$ = std::make_unique<optiq::cpp::UnaryExpressionNode>(
            loc(@$), "--", std::move($1), false);
    }
;

argument_list_opt:
    %empty { $$ = optiq::cpp::NodeList{}; }
  | argument_list { $$ = std::move($1); }
;

argument_list:
    expression { $$ = single(std::move($1)); }
  | argument_list COMMA expression {
        $$ = std::move($1);
        $$.push_back(std::move($3));
    }
;

primary_expression:
    IDENTIFIER {
        $$ = std::make_unique<optiq::cpp::IdentifierNode>(loc(@1), std::move($1));
    }
  | INTEGER_LITERAL {
        $$ = std::make_unique<optiq::cpp::LiteralNode>(
            loc(@1), optiq::cpp::LiteralKind::Integer, std::move($1));
    }
  | FLOATING_LITERAL {
        $$ = std::make_unique<optiq::cpp::LiteralNode>(
            loc(@1), optiq::cpp::LiteralKind::Floating, std::move($1));
    }
  | CHARACTER_LITERAL {
        $$ = std::make_unique<optiq::cpp::LiteralNode>(
            loc(@1), optiq::cpp::LiteralKind::Character, std::move($1));
    }
  | STRING_LITERAL {
        $$ = std::make_unique<optiq::cpp::LiteralNode>(
            loc(@1), optiq::cpp::LiteralKind::String, std::move($1));
    }
  | TRUE {
        $$ = std::make_unique<optiq::cpp::LiteralNode>(
            loc(@1), optiq::cpp::LiteralKind::Boolean, "true");
    }
  | FALSE {
        $$ = std::make_unique<optiq::cpp::LiteralNode>(
            loc(@1), optiq::cpp::LiteralKind::Boolean, "false");
    }
  | LEFT_PAREN expression RIGHT_PAREN { $$ = std::move($2); }
;

%%

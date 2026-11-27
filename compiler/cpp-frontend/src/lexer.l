%{
#include "scanner_state.hpp"

#define YY_USER_ACTION yyextra->advance(yytext, static_cast<std::size_t>(yyleng));
%}

%option reentrant noyywrap nounput noinput nodefault
%option extra-type="optiq::cpp::ScannerState*"
%x BLOCK_COMMENT STRING_LITERAL CHARACTER_LITERAL

DIGIT [0-9]

%%

[ \t\r\n\f\v]+                  { }
"//"[^\n]*                     { }
"/*"                            { yyextra->beginBlockComment(); BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"            { BEGIN(INITIAL); }
<BLOCK_COMMENT>(\n|.)           { }
<BLOCK_COMMENT><<EOF>>          { yyextra->failUnterminatedBlockComment(); BEGIN(INITIAL); return 0; }

^[ \t]*"#"[^\n]*               { yyextra->addPreprocessor(yytext, static_cast<std::size_t>(yyleng)); }

\"                              { yyextra->beginLiteral(yytext, static_cast<std::size_t>(yyleng), '\"'); BEGIN(STRING_LITERAL); }
<STRING_LITERAL>\"             { yyextra->finishLiteral(); BEGIN(INITIAL); }
<STRING_LITERAL>\\[nt\\\"']     { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng), 1); }
<STRING_LITERAL>\\[^\n]         { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng)); yyextra->invalidEscape(yytext, static_cast<std::size_t>(yyleng)); }
<STRING_LITERAL>\\              { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng)); yyextra->invalidEscape(yytext, static_cast<std::size_t>(yyleng)); }
<STRING_LITERAL>\n              { yyextra->failUnterminatedLiteral(); BEGIN(INITIAL); }
<STRING_LITERAL>[^\"\\\n]+      { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng)); }
<STRING_LITERAL><<EOF>>         { yyextra->failUnterminatedLiteral(); BEGIN(INITIAL); return 0; }

\'                              { yyextra->beginLiteral(yytext, static_cast<std::size_t>(yyleng), '\''); BEGIN(CHARACTER_LITERAL); }
<CHARACTER_LITERAL>\'          { yyextra->finishLiteral(); BEGIN(INITIAL); }
<CHARACTER_LITERAL>\\[nt\\\"']  { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng), 1); }
<CHARACTER_LITERAL>\\[^\n]      { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng)); yyextra->invalidEscape(yytext, static_cast<std::size_t>(yyleng)); }
<CHARACTER_LITERAL>\\           { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng)); yyextra->invalidEscape(yytext, static_cast<std::size_t>(yyleng)); }
<CHARACTER_LITERAL>\n           { yyextra->failUnterminatedLiteral(); BEGIN(INITIAL); }
<CHARACTER_LITERAL>[^\'\\\n]+   { yyextra->appendLiteral(yytext, static_cast<std::size_t>(yyleng), static_cast<std::size_t>(yyleng)); }
<CHARACTER_LITERAL><<EOF>>      { yyextra->failUnterminatedLiteral(); BEGIN(INITIAL); return 0; }

{DIGIT}+\.{DIGIT}+[fF]?         { yyextra->addFloating(yytext, static_cast<std::size_t>(yyleng)); }
{DIGIT}+                        { yyextra->addInteger(yytext, static_cast<std::size_t>(yyleng)); }
{DIGIT}[A-Za-z0-9_.]*           { yyextra->fail("Malformed numeric literal: " + std::string(yytext, static_cast<std::size_t>(yyleng))); }
\.{DIGIT}[A-Za-z0-9_.]*         { yyextra->fail("Malformed numeric literal (digits are required before and after the decimal point): " + std::string(yytext, static_cast<std::size_t>(yyleng))); }
[A-Za-z_][A-Za-z0-9_]*          { yyextra->addIdentifier(yytext, static_cast<std::size_t>(yyleng)); }

"++"                            { yyextra->add(optiq::cpp::TokenType::Increment, yytext, yyleng); }
"--"                            { yyextra->add(optiq::cpp::TokenType::Decrement, yytext, yyleng); }
"+="                            { yyextra->add(optiq::cpp::TokenType::PlusAssignment, yytext, yyleng); }
"-="                            { yyextra->add(optiq::cpp::TokenType::MinusAssignment, yytext, yyleng); }
"*="                            { yyextra->add(optiq::cpp::TokenType::StarAssignment, yytext, yyleng); }
"/="                            { yyextra->add(optiq::cpp::TokenType::SlashAssignment, yytext, yyleng); }
"->"                            { yyextra->add(optiq::cpp::TokenType::Arrow, yytext, yyleng); }
"<<"                            { yyextra->add(optiq::cpp::TokenType::ShiftLeft, yytext, yyleng); }
">>"                            { yyextra->add(optiq::cpp::TokenType::ShiftRight, yytext, yyleng); }
"<="                            { yyextra->add(optiq::cpp::TokenType::LessEqual, yytext, yyleng); }
">="                            { yyextra->add(optiq::cpp::TokenType::GreaterEqual, yytext, yyleng); }
"=="                            { yyextra->add(optiq::cpp::TokenType::EqualEqual, yytext, yyleng); }
"!="                            { yyextra->add(optiq::cpp::TokenType::NotEqual, yytext, yyleng); }
"&&"                            { yyextra->add(optiq::cpp::TokenType::LogicalAnd, yytext, yyleng); }
"||"                            { yyextra->add(optiq::cpp::TokenType::LogicalOr, yytext, yyleng); }
"::"                            { yyextra->add(optiq::cpp::TokenType::Scope, yytext, yyleng); }

"+"                             { yyextra->add(optiq::cpp::TokenType::Plus, yytext, yyleng); }
"-"                             { yyextra->add(optiq::cpp::TokenType::Minus, yytext, yyleng); }
"*"                             { yyextra->add(optiq::cpp::TokenType::Star, yytext, yyleng); }
"/"                             { yyextra->add(optiq::cpp::TokenType::Slash, yytext, yyleng); }
"%"                             { yyextra->add(optiq::cpp::TokenType::Percent, yytext, yyleng); }
"<"                             { yyextra->add(optiq::cpp::TokenType::Less, yytext, yyleng); }
">"                             { yyextra->add(optiq::cpp::TokenType::Greater, yytext, yyleng); }
"="                             { yyextra->add(optiq::cpp::TokenType::Assignment, yytext, yyleng); }
"!"                             { yyextra->add(optiq::cpp::TokenType::LogicalNot, yytext, yyleng); }
"&"                             { yyextra->add(optiq::cpp::TokenType::BitwiseAnd, yytext, yyleng); }
"|"                             { yyextra->add(optiq::cpp::TokenType::BitwiseOr, yytext, yyleng); }
"^"                             { yyextra->add(optiq::cpp::TokenType::BitwiseXor, yytext, yyleng); }
"~"                             { yyextra->add(optiq::cpp::TokenType::BitwiseNot, yytext, yyleng); }
"."                             { yyextra->add(optiq::cpp::TokenType::Dot, yytext, yyleng); }

"("                             { yyextra->add(optiq::cpp::TokenType::LeftParen, yytext, yyleng); }
")"                             { yyextra->add(optiq::cpp::TokenType::RightParen, yytext, yyleng); }
"{"                             { yyextra->add(optiq::cpp::TokenType::LeftBrace, yytext, yyleng); }
"}"                             { yyextra->add(optiq::cpp::TokenType::RightBrace, yytext, yyleng); }
"["                             { yyextra->add(optiq::cpp::TokenType::LeftBracket, yytext, yyleng); }
"]"                             { yyextra->add(optiq::cpp::TokenType::RightBracket, yytext, yyleng); }
";"                             { yyextra->add(optiq::cpp::TokenType::Semicolon, yytext, yyleng); }
","                             { yyextra->add(optiq::cpp::TokenType::Comma, yytext, yyleng); }
":"                             { yyextra->add(optiq::cpp::TokenType::Colon, yytext, yyleng); }

.                               { yyextra->fail("Invalid character: " + std::string(yytext, static_cast<std::size_t>(yyleng))); }
<<EOF>>                         { return 0; }

%%

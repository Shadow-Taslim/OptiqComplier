%{
#include "scanner_state.hpp"
#define YY_USER_ACTION yyextra->advance(yytext, static_cast<std::size_t>(yyleng));
%}

%option reentrant noyywrap nounput noinput nodefault
%option extra-type="optiq::ScannerState*"
%x BLOCK_COMMENT

%%

[ \t\r\n]+                  { }
"--"[^\n]*                  { }
"//"[^\n]*                  { }
"/*"                        { yyextra->blockCommentStart = yyextra->current; BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"         { BEGIN(INITIAL); }
<BLOCK_COMMENT>(\n|.)       { }
<BLOCK_COMMENT><<EOF>>      { yyextra->fail("Unterminated block comment", yyextra->blockCommentStart); return 0; }

\'([^\'\n]|\'\')*\'       { yyextra->addString(yytext, static_cast<std::size_t>(yyleng)); }
\'[^\'\n]*                 { yyextra->fail("Unterminated string literal"); return 0; }

[0-9]+(\.[0-9]+)?[A-Za-z_][A-Za-z0-9_]* { yyextra->fail("Invalid numeric literal: " + std::string(yytext)); return 0; }
[0-9]+\.[0-9]+              { yyextra->addDecimal(yytext); }
[0-9]+                      { yyextra->addInteger(yytext); }
[A-Za-z_][A-Za-z0-9_]*      { yyextra->addKeyword(yytext); }

"("                         { yyextra->add(optiq::TokenKind::LeftParen, yytext, yyleng); }
")"                         { yyextra->add(optiq::TokenKind::RightParen, yytext, yyleng); }
"{"                         { yyextra->add(optiq::TokenKind::LeftBrace, yytext, yyleng); }
"}"                         { yyextra->add(optiq::TokenKind::RightBrace, yytext, yyleng); }
","                         { yyextra->add(optiq::TokenKind::Comma, yytext, yyleng); }
";"                         { yyextra->add(optiq::TokenKind::Semicolon, yytext, yyleng); }
"."                         { yyextra->add(optiq::TokenKind::Dot, yytext, yyleng); }
"="|"=="                    { yyextra->add(optiq::TokenKind::Equal, yytext, yyleng); }
"!="|"<>"                   { yyextra->add(optiq::TokenKind::NotEqual, yytext, yyleng); }
"<="                        { yyextra->add(optiq::TokenKind::LessEqual, yytext, yyleng); }
">="                        { yyextra->add(optiq::TokenKind::GreaterEqual, yytext, yyleng); }
"<"                         { yyextra->add(optiq::TokenKind::Less, yytext, yyleng); }
">"                         { yyextra->add(optiq::TokenKind::Greater, yytext, yyleng); }
"+"                         { yyextra->add(optiq::TokenKind::Plus, yytext, yyleng); }
"-"                         { yyextra->add(optiq::TokenKind::Minus, yytext, yyleng); }
"*"                         { yyextra->add(optiq::TokenKind::Star, yytext, yyleng); }
"/"                         { yyextra->add(optiq::TokenKind::Slash, yytext, yyleng); }

.                           { yyextra->fail("Invalid character: " + std::string(yytext)); return 0; }
<<EOF>>                     { return 0; }

%%

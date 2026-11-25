#include "optiq/lexer.hpp"

#include <stdexcept>

#include "scanner_state.hpp"

struct yy_buffer_state;
using YY_BUFFER_STATE = yy_buffer_state*;
using yyscan_t = void*;

int yylex_init_extra(optiq::ScannerState* state, yyscan_t* scanner);
YY_BUFFER_STATE yy_scan_string(const char* text, yyscan_t scanner);
void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);
int yylex(yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);

namespace optiq {

LexResult tokenize(const std::string& source) {
    ScannerState state;
    yyscan_t scanner = nullptr;
    if (yylex_init_extra(&state, &scanner) != 0) {
        throw std::runtime_error("Unable to initialize the Optiq lexer");
    }

    YY_BUFFER_STATE buffer = nullptr;
    try {
        buffer = yy_scan_string(source.c_str(), scanner);
        if (!buffer) throw std::runtime_error("Unable to create the lexer buffer");
        yylex(scanner);
        yy_delete_buffer(buffer, scanner);
        buffer = nullptr;
        yylex_destroy(scanner);
        scanner = nullptr;
    } catch (...) {
        if (buffer) yy_delete_buffer(buffer, scanner);
        if (scanner) yylex_destroy(scanner);
        throw;
    }

    if (!state.result.error) {
        state.result.tokens.push_back(Token{TokenKind::End, "", std::monostate{}, state.next});
    }
    return std::move(state.result);
}

std::string tokenKindName(TokenKind kind) {
    switch (kind) {
    case TokenKind::End: return "end of input";
    case TokenKind::Identifier: return "identifier";
    case TokenKind::IntegerLiteral: return "integer literal";
    case TokenKind::FloatLiteral: return "float literal";
    case TokenKind::IntType: return "int";
    case TokenKind::FloatType: return "float";
    case TokenKind::BoolType: return "bool";
    case TokenKind::If: return "if";
    case TokenKind::Else: return "else";
    case TokenKind::While: return "while";
    case TokenKind::Print: return "print";
    case TokenKind::True: return "true";
    case TokenKind::False: return "false";
    case TokenKind::LeftParen: return "(";
    case TokenKind::RightParen: return ")";
    case TokenKind::LeftBrace: return "{";
    case TokenKind::RightBrace: return "}";
    case TokenKind::Semicolon: return ";";
    case TokenKind::Assign: return "=";
    case TokenKind::Equal: return "==";
    case TokenKind::NotEqual: return "!=";
    case TokenKind::Less: return "<";
    case TokenKind::LessEqual: return "<=";
    case TokenKind::Greater: return ">";
    case TokenKind::GreaterEqual: return ">=";
    case TokenKind::Plus: return "+";
    case TokenKind::Minus: return "-";
    case TokenKind::Star: return "*";
    case TokenKind::Slash: return "/";
    case TokenKind::Percent: return "%";
    case TokenKind::LogicalAnd: return "&&";
    case TokenKind::LogicalOr: return "||";
    case TokenKind::Not: return "!";
    }
    return "unknown token";
}

} // namespace optiq

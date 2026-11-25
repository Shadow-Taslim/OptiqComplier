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

LexResult tokenize(const std::string& sql) {
    ScannerState state;
    yyscan_t scanner = nullptr;
    if (yylex_init_extra(&state, &scanner) != 0) {
        throw std::runtime_error("Unable to initialize the SQL lexer");
    }

    YY_BUFFER_STATE buffer = nullptr;
    try {
        buffer = yy_scan_string(sql.c_str(), scanner);
        if (!buffer) throw std::runtime_error("Unable to create the SQL lexer buffer");
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
    case TokenKind::DecimalLiteral: return "decimal literal";
    case TokenKind::StringLiteral: return "string literal";
    case TokenKind::Select: return "SELECT";
    case TokenKind::From: return "FROM";
    case TokenKind::As: return "AS";
    case TokenKind::Inner: return "INNER";
    case TokenKind::Join: return "JOIN";
    case TokenKind::On: return "ON";
    case TokenKind::Where: return "WHERE";
    case TokenKind::Group: return "GROUP";
    case TokenKind::By: return "BY";
    case TokenKind::Order: return "ORDER";
    case TokenKind::Asc: return "ASC";
    case TokenKind::Desc: return "DESC";
    case TokenKind::Limit: return "LIMIT";
    case TokenKind::And: return "AND";
    case TokenKind::Or: return "OR";
    case TokenKind::Not: return "NOT";
    case TokenKind::Count: return "COUNT";
    case TokenKind::Sum: return "SUM";
    case TokenKind::Average: return "AVG";
    case TokenKind::Minimum: return "MIN";
    case TokenKind::Maximum: return "MAX";
    case TokenKind::Let: return "LET";
    case TokenKind::If: return "IF";
    case TokenKind::Else: return "ELSE";
    case TokenKind::While: return "WHILE";
    case TokenKind::Print: return "PRINT";
    case TokenKind::True: return "TRUE";
    case TokenKind::False: return "FALSE";
    case TokenKind::LeftParen: return "(";
    case TokenKind::RightParen: return ")";
    case TokenKind::LeftBrace: return "{";
    case TokenKind::RightBrace: return "}";
    case TokenKind::Comma: return ",";
    case TokenKind::Semicolon: return ";";
    case TokenKind::Dot: return ".";
    case TokenKind::Equal: return "=";
    case TokenKind::NotEqual: return "!=";
    case TokenKind::Less: return "<";
    case TokenKind::LessEqual: return "<=";
    case TokenKind::Greater: return ">";
    case TokenKind::GreaterEqual: return ">=";
    case TokenKind::Plus: return "+";
    case TokenKind::Minus: return "-";
    case TokenKind::Star: return "*";
    case TokenKind::Slash: return "/";
    }
    return "unknown token";
}

} // namespace optiq

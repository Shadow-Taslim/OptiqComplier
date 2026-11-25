#include "optiq/parser.hpp"

#include <utility>

#include "optiq_parser.hpp"
#include "token_stream.hpp"

int yyparse(optiq::TokenStream& tokens, optiq::ParseResult& result);

namespace {

int parserToken(optiq::TokenKind kind) {
    using optiq::TokenKind;
    switch (kind) {
    case TokenKind::End: return 0;
    case TokenKind::Identifier: return IDENTIFIER;
    case TokenKind::IntegerLiteral: return INTEGER_LITERAL;
    case TokenKind::FloatLiteral: return FLOAT_LITERAL;
    case TokenKind::IntType: return INT;
    case TokenKind::FloatType: return FLOAT;
    case TokenKind::BoolType: return BOOL;
    case TokenKind::If: return IF;
    case TokenKind::Else: return ELSE;
    case TokenKind::While: return WHILE;
    case TokenKind::Print: return PRINT;
    case TokenKind::True: return TRUE;
    case TokenKind::False: return FALSE;
    case TokenKind::LeftParen: return LEFT_PAREN;
    case TokenKind::RightParen: return RIGHT_PAREN;
    case TokenKind::LeftBrace: return LEFT_BRACE;
    case TokenKind::RightBrace: return RIGHT_BRACE;
    case TokenKind::Semicolon: return SEMICOLON;
    case TokenKind::Assign: return ASSIGN;
    case TokenKind::Equal: return EQUAL;
    case TokenKind::NotEqual: return NOT_EQUAL;
    case TokenKind::Less: return LESS;
    case TokenKind::LessEqual: return LESS_EQUAL;
    case TokenKind::Greater: return GREATER;
    case TokenKind::GreaterEqual: return GREATER_EQUAL;
    case TokenKind::Plus: return PLUS;
    case TokenKind::Minus: return MINUS;
    case TokenKind::Star: return STAR;
    case TokenKind::Slash: return SLASH;
    case TokenKind::Percent: return PERCENT;
    case TokenKind::LogicalAnd: return AND;
    case TokenKind::LogicalOr: return OR;
    case TokenKind::Not: return NOT;
    }
    return 0;
}

} // namespace

int yylex(YYSTYPE* value, YYLTYPE* location, optiq::TokenStream& tokens) {
    const optiq::Token& token = tokens.next();
    location->first_line = token.location.line;
    location->first_column = token.location.column;
    location->last_line = token.location.line;
    location->last_column = token.location.column +
                            static_cast<int>(token.lexeme.size());

    switch (token.kind) {
    case optiq::TokenKind::Identifier:
        value->string = new std::string(std::get<std::string>(token.value));
        break;
    case optiq::TokenKind::IntegerLiteral:
        value->integerValue = std::get<std::int64_t>(token.value);
        break;
    case optiq::TokenKind::FloatLiteral:
        value->floatValue = std::get<double>(token.value);
        break;
    default:
        break;
    }
    return parserToken(token.kind);
}

void yyerror(YYLTYPE* location, optiq::TokenStream& tokens,
             optiq::ParseResult& result, const char* message) {
    (void)tokens;
    result.errors.push_back(optiq::Diagnostic{
        message, optiq::SourceLocation{location->first_line, location->first_column, 0}});
}

namespace optiq {

ParseResult parseProgram(const std::string& source) {
    ParseResult result;
    LexResult lexed = tokenize(source);
    if (lexed.error) {
        result.errors.push_back(std::move(*lexed.error));
        return result;
    }

    TokenStream tokens(lexed.tokens);
    const int status = yyparse(tokens, result);
    if (status != 0 && result.errors.empty()) {
        result.errors.push_back(Diagnostic{"Unable to parse input", SourceLocation{}});
    }
    if (status == 0 && !result.program && result.errors.empty()) {
        result.errors.push_back(Diagnostic{"Parser produced no program", SourceLocation{}});
    }
    return result;
}

} // namespace optiq

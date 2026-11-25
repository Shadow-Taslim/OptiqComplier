#include "optiq/parser.hpp"

#include <utility>

#include "sql_parser.hpp"
#include "token_stream.hpp"

int yyparse(optiq::TokenStream& tokens, optiq::ProgramParseResult& result);

namespace {

int parserToken(optiq::TokenKind kind) {
    using optiq::TokenKind;
    switch (kind) {
    case TokenKind::End: return 0;
    case TokenKind::Identifier: return IDENTIFIER;
    case TokenKind::IntegerLiteral: return INTEGER_LITERAL;
    case TokenKind::DecimalLiteral: return DECIMAL_LITERAL;
    case TokenKind::StringLiteral: return STRING_LITERAL;
    case TokenKind::Select: return SELECT;
    case TokenKind::From: return FROM;
    case TokenKind::As: return AS;
    case TokenKind::Inner: return INNER;
    case TokenKind::Join: return JOIN;
    case TokenKind::On: return ON;
    case TokenKind::Where: return WHERE;
    case TokenKind::Group: return GROUP;
    case TokenKind::By: return BY;
    case TokenKind::Order: return ORDER;
    case TokenKind::Asc: return ASC;
    case TokenKind::Desc: return DESC;
    case TokenKind::Limit: return LIMIT;
    case TokenKind::And: return AND;
    case TokenKind::Or: return OR;
    case TokenKind::Not: return NOT;
    case TokenKind::Count: return COUNT;
    case TokenKind::Sum: return SUM;
    case TokenKind::Average: return AVG;
    case TokenKind::Minimum: return MIN;
    case TokenKind::Maximum: return MAX;
    case TokenKind::Let: return LET;
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
    case TokenKind::Comma: return COMMA;
    case TokenKind::Semicolon: return SEMICOLON;
    case TokenKind::Dot: return DOT;
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
    case optiq::TokenKind::StringLiteral:
        value->string = new std::string(std::get<std::string>(token.value));
        break;
    case optiq::TokenKind::IntegerLiteral:
        value->integerValue = std::get<std::int64_t>(token.value);
        break;
    case optiq::TokenKind::DecimalLiteral:
        value->decimalValue = std::get<double>(token.value);
        break;
    default:
        break;
    }
    return parserToken(token.kind);
}

void yyerror(YYLTYPE* location, optiq::TokenStream& tokens,
             optiq::ProgramParseResult& result, const char* message) {
    (void)tokens;
    if (!result.error) {
        result.error = optiq::Diagnostic{
            message, optiq::SourceLocation{location->first_line, location->first_column, 0}};
    }
}

namespace optiq {

ProgramParseResult parseProgram(const std::string& source) {
    LexResult lexed = tokenize(source);
    if (lexed.error) {
        ProgramParseResult result;
        result.error = std::move(lexed.error);
        return result;
    }

    ProgramParseResult result;
    TokenStream tokens(lexed.tokens);
    const int status = yyparse(tokens, result);
    if (status != 0 && !result.error) {
        result.error = Diagnostic{"Unable to parse input", SourceLocation{}};
    }
    if (status == 0 && !result.program && !result.error) {
        result.error = Diagnostic{"Parser produced no program", SourceLocation{}};
    }
    return result;
}

ParseResult parseSql(const std::string& sql) {
    // Query statements in a program end with ';'; keep the legacy
    // semicolon-optional single-query behavior by appending one if needed.
    std::string source = sql;
    const auto last = source.find_last_not_of(" \t\r\n");
    if (last == std::string::npos || source[last] != ';') {
        source.push_back(';');
    }

    ProgramParseResult parsed = parseProgram(source);
    ParseResult result;
    if (parsed.error) {
        result.error = std::move(parsed.error);
        return result;
    }

    if (parsed.program->statements.size() != 1 ||
        parsed.program->statements[0]->kind != Statement::Kind::Query) {
        result.error = Diagnostic{"Expected a single SQL SELECT statement", SourceLocation{}};
        return result;
    }

    auto& only = static_cast<QueryStatement&>(*parsed.program->statements[0]);
    result.statement = std::move(only.query);
    return result;
}

} // namespace optiq

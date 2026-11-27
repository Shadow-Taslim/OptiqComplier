#include "optiq/cpp/parser.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "cpp_parser.hpp"
#include "optiq/cpp/lexer.hpp"
#include "parser_context.hpp"

namespace optiq::cpp {
namespace {

using BisonParser = syntax::Parser;
using BisonLocation = BisonParser::location_type;

BisonLocation bisonLocation(const SourceLocation& start, const std::string& lexeme = {}) {
    BisonLocation location;
    location.begin.line = static_cast<unsigned>(start.line);
    location.begin.column = static_cast<unsigned>(start.column);
    location.end = location.begin;
    for (char ch : lexeme) {
        if (ch == '\n') {
            ++location.end.line;
            location.end.column = 1;
        } else {
            ++location.end.column;
        }
    }
    return location;
}

} // namespace

SourceLocation computeEofLocation(const std::string& source) {
    SourceLocation location;
    for (char ch : source) {
        ++location.offset;
        if (ch == '\n') {
            ++location.line;
            location.column = 1;
        } else {
            ++location.column;
        }
    }
    return location;
}

namespace syntax {

Parser::symbol_type yylex(ParserContext& context) {
    if (context.nextToken >= context.tokens.size()) {
        return Parser::make_END(bisonLocation(context.eofLocation));
    }

    const Token& token = context.tokens[context.nextToken++];
    const auto location = bisonLocation(token.location, token.lexeme);

    if (token.type == TokenType::Keyword) {
        if (token.lexeme == "int") return Parser::make_INT(location);
        if (token.lexeme == "float") return Parser::make_FLOAT(location);
        if (token.lexeme == "double") return Parser::make_DOUBLE(location);
        if (token.lexeme == "char") return Parser::make_CHAR(location);
        if (token.lexeme == "bool") return Parser::make_BOOL(location);
        if (token.lexeme == "void") return Parser::make_VOID(location);
        if (token.lexeme == "if") return Parser::make_IF(location);
        if (token.lexeme == "else") return Parser::make_ELSE(location);
        if (token.lexeme == "while") return Parser::make_WHILE(location);
        if (token.lexeme == "for") return Parser::make_FOR(location);
        if (token.lexeme == "do") return Parser::make_DO(location);
        if (token.lexeme == "return") return Parser::make_RETURN(location);
        if (token.lexeme == "true") return Parser::make_TRUE(location);
        if (token.lexeme == "false") return Parser::make_FALSE(location);
        if (token.lexeme == "break") return Parser::make_BREAK(location);
        if (token.lexeme == "continue") return Parser::make_CONTINUE(location);
        throw std::logic_error("Unknown keyword in parser token adapter: " + token.lexeme);
    }

    if (token.type == TokenType::Identifier) {
        if (token.lexeme == "using") return Parser::make_USING(location);
        if (token.lexeme == "namespace") return Parser::make_NAMESPACE(location);
        if (token.lexeme == "std") return Parser::make_STD(location);
        if (token.lexeme == "cout") return Parser::make_COUT(location);
        if (token.lexeme == "cin") return Parser::make_CIN(location);
        if (token.lexeme == "endl") return Parser::make_ENDL(location);
        return Parser::make_IDENTIFIER(token.lexeme, location);
    }

    switch (token.type) {
    case TokenType::IntegerLiteral:
        return Parser::make_INTEGER_LITERAL(token.lexeme, location);
    case TokenType::FloatingLiteral:
        return Parser::make_FLOATING_LITERAL(token.lexeme, location);
    case TokenType::CharacterLiteral:
        return Parser::make_CHARACTER_LITERAL(token.lexeme, location);
    case TokenType::StringLiteral:
        return Parser::make_STRING_LITERAL(token.lexeme, location);
    case TokenType::Preprocessor:
        return Parser::make_PREPROCESSOR(token.lexeme, location);
    case TokenType::Plus: return Parser::make_PLUS(location);
    case TokenType::Minus: return Parser::make_MINUS(location);
    case TokenType::Star: return Parser::make_STAR(location);
    case TokenType::Slash: return Parser::make_SLASH(location);
    case TokenType::Percent: return Parser::make_PERCENT(location);
    case TokenType::Less: return Parser::make_LESS(location);
    case TokenType::Greater: return Parser::make_GREATER(location);
    case TokenType::LessEqual: return Parser::make_LESS_EQUAL(location);
    case TokenType::GreaterEqual: return Parser::make_GREATER_EQUAL(location);
    case TokenType::EqualEqual: return Parser::make_EQUAL_EQUAL(location);
    case TokenType::NotEqual: return Parser::make_NOT_EQUAL(location);
    case TokenType::LogicalAnd: return Parser::make_LOGICAL_AND(location);
    case TokenType::LogicalOr: return Parser::make_LOGICAL_OR(location);
    case TokenType::LogicalNot: return Parser::make_LOGICAL_NOT(location);
    case TokenType::Assignment: return Parser::make_ASSIGN(location);
    case TokenType::PlusAssignment: return Parser::make_PLUS_ASSIGN(location);
    case TokenType::MinusAssignment: return Parser::make_MINUS_ASSIGN(location);
    case TokenType::StarAssignment: return Parser::make_STAR_ASSIGN(location);
    case TokenType::SlashAssignment: return Parser::make_SLASH_ASSIGN(location);
    case TokenType::Increment: return Parser::make_INCREMENT(location);
    case TokenType::Decrement: return Parser::make_DECREMENT(location);
    case TokenType::BitwiseAnd: return Parser::make_BITWISE_AND(location);
    case TokenType::BitwiseOr: return Parser::make_BITWISE_OR(location);
    case TokenType::BitwiseXor: return Parser::make_BITWISE_XOR(location);
    case TokenType::BitwiseNot: return Parser::make_BITWISE_NOT(location);
    case TokenType::ShiftLeft: return Parser::make_SHIFT_LEFT(location);
    case TokenType::ShiftRight: return Parser::make_SHIFT_RIGHT(location);
    case TokenType::Arrow: return Parser::make_ARROW(location);
    case TokenType::Dot: return Parser::make_DOT(location);
    case TokenType::Scope: return Parser::make_SCOPE(location);
    case TokenType::LeftParen: return Parser::make_LEFT_PAREN(location);
    case TokenType::RightParen: return Parser::make_RIGHT_PAREN(location);
    case TokenType::LeftBrace: return Parser::make_LEFT_BRACE(location);
    case TokenType::RightBrace: return Parser::make_RIGHT_BRACE(location);
    case TokenType::LeftBracket: return Parser::make_LEFT_BRACKET(location);
    case TokenType::RightBracket: return Parser::make_RIGHT_BRACKET(location);
    case TokenType::Semicolon: return Parser::make_SEMICOLON(location);
    case TokenType::Comma: return Parser::make_COMMA(location);
    case TokenType::Colon: return Parser::make_COLON(location);
    case TokenType::Keyword:
    case TokenType::Identifier:
        break;
    }

    throw std::logic_error("Unhandled token in parser token adapter");
}

void Parser::error(const location_type& location, const std::string& message) {
    std::string detail = message;
    const std::string prefix = "syntax error, ";
    if (detail.rfind(prefix, 0) == 0) detail.erase(0, prefix.size());
    parserContext.addSyntaxError(SourceLocation{
        static_cast<std::size_t>(location.begin.line),
        static_cast<std::size_t>(location.begin.column), 0}, detail);
}

} // namespace syntax

ParseResult parse(const std::string& source) {
    ParseResult result;
    auto lexed = tokenize(source);
    result.lexicalDiagnostics = std::move(lexed.diagnostics);
    if (!result.lexicalDiagnostics.empty()) return result;

    ParserContext context(std::move(lexed.tokens), computeEofLocation(source));
    syntax::Parser parser(context);
    const int status = parser.parse();

    if (status != 0 && context.diagnostics.empty()) {
        context.addSyntaxError(context.eofLocation, "Parsing failed without a detailed diagnostic");
    }

    if (context.diagnostics.empty() && status == 0) {
        result.program = std::move(context.program);
    }
    result.syntaxDiagnostics = std::move(context.diagnostics);
    return result;
}

} // namespace optiq::cpp

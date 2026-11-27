#include "optiq/cpp/ast.hpp"

namespace optiq::cpp {

const char* nodeKindName(NodeKind kind) {
    switch (kind) {
    case NodeKind::Program: return "Program";
    case NodeKind::FunctionDeclaration: return "FunctionDeclaration";
    case NodeKind::FunctionDefinition: return "FunctionDefinition";
    case NodeKind::Parameter: return "Parameter";
    case NodeKind::VariableDeclaration: return "VariableDeclaration";
    case NodeKind::BlockStatement: return "BlockStatement";
    case NodeKind::IfStatement: return "IfStatement";
    case NodeKind::WhileStatement: return "WhileStatement";
    case NodeKind::ForStatement: return "ForStatement";
    case NodeKind::DoWhileStatement: return "DoWhileStatement";
    case NodeKind::ReturnStatement: return "ReturnStatement";
    case NodeKind::ExpressionStatement: return "ExpressionStatement";
    case NodeKind::EmptyStatement: return "EmptyStatement";
    case NodeKind::ErrorStatement: return "ErrorStatement";
    case NodeKind::AssignmentExpression: return "AssignmentExpression";
    case NodeKind::BinaryExpression: return "BinaryExpression";
    case NodeKind::UnaryExpression: return "UnaryExpression";
    case NodeKind::FunctionCall: return "FunctionCall";
    case NodeKind::Identifier: return "Identifier";
    case NodeKind::Literal: return "Literal";
    case NodeKind::StreamOutput: return "StreamOutput";
    case NodeKind::StreamInput: return "StreamInput";
    case NodeKind::Preprocessor: return "Preprocessor";
    case NodeKind::UsingNamespace: return "UsingNamespace";
    }
    return "Unknown";
}

const char* literalKindName(LiteralKind kind) {
    switch (kind) {
    case LiteralKind::Integer: return "integer";
    case LiteralKind::Floating: return "floating";
    case LiteralKind::Character: return "character";
    case LiteralKind::String: return "string";
    case LiteralKind::Boolean: return "boolean";
    }
    return "unknown";
}

} // namespace optiq::cpp

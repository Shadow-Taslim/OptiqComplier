#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "optiq/cpp/token.hpp"

namespace optiq::cpp {

enum class NodeKind {
    Program,
    FunctionDeclaration,
    FunctionDefinition,
    Parameter,
    VariableDeclaration,
    BlockStatement,
    IfStatement,
    WhileStatement,
    ForStatement,
    DoWhileStatement,
    ReturnStatement,
    ExpressionStatement,
    EmptyStatement,
    ErrorStatement,
    AssignmentExpression,
    BinaryExpression,
    UnaryExpression,
    FunctionCall,
    Identifier,
    Literal,
    StreamOutput,
    StreamInput,
    Preprocessor,
    UsingNamespace
};

class Node {
public:
    Node(NodeKind kind, SourceLocation location) : kind(kind), location(location) {}
    virtual ~Node() = default;

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(Node&&) = delete;

    NodeKind kind;
    SourceLocation location;
};

using NodePtr = std::unique_ptr<Node>;
using NodeList = std::vector<NodePtr>;

class Expression : public Node {
public:
    using Node::Node;
};

class Statement : public Node {
public:
    using Node::Node;
};

class ProgramNode final : public Node {
public:
    ProgramNode() : Node(NodeKind::Program, SourceLocation{}) {}
    NodeList declarations;
};

class ParameterNode final : public Node {
public:
    ParameterNode(SourceLocation location, std::string typeName, std::string name)
        : Node(NodeKind::Parameter, location), typeName(std::move(typeName)),
          name(std::move(name)) {}

    std::string typeName;
    std::string name;
};

class FunctionDeclarationNode : public Node {
public:
    FunctionDeclarationNode(NodeKind kind, SourceLocation location,
                            std::string returnType, std::string name,
                            NodeList parameters)
        : Node(kind, location), returnType(std::move(returnType)),
          name(std::move(name)), parameters(std::move(parameters)) {}

    FunctionDeclarationNode(SourceLocation location, std::string returnType,
                            std::string name, NodeList parameters)
        : FunctionDeclarationNode(NodeKind::FunctionDeclaration, location,
                                  std::move(returnType), std::move(name),
                                  std::move(parameters)) {}

    std::string returnType;
    std::string name;
    NodeList parameters;
};

class FunctionDefinitionNode final : public FunctionDeclarationNode {
public:
    FunctionDefinitionNode(SourceLocation location, std::string returnType,
                           std::string name, NodeList parameters, NodePtr body)
        : FunctionDeclarationNode(NodeKind::FunctionDefinition, location,
                                  std::move(returnType), std::move(name),
                                  std::move(parameters)),
          body(std::move(body)) {}

    NodePtr body;
};

class VariableDeclarationNode final : public Statement {
public:
    VariableDeclarationNode(SourceLocation location, std::string name,
                            NodePtr initializer = nullptr)
        : Statement(NodeKind::VariableDeclaration, location), name(std::move(name)),
          initializer(std::move(initializer)) {}

    std::string typeName;
    std::string name;
    NodePtr initializer;
};

class BlockStatementNode final : public Statement {
public:
    explicit BlockStatementNode(SourceLocation location)
        : Statement(NodeKind::BlockStatement, location) {}

    NodeList statements;
};

class ExpressionStatementNode final : public Statement {
public:
    ExpressionStatementNode(SourceLocation location, NodePtr expression)
        : Statement(NodeKind::ExpressionStatement, location),
          expression(std::move(expression)) {}

    NodePtr expression;
};

class EmptyStatementNode final : public Statement {
public:
    explicit EmptyStatementNode(SourceLocation location)
        : Statement(NodeKind::EmptyStatement, location) {}
};

class ErrorStatementNode final : public Statement {
public:
    explicit ErrorStatementNode(SourceLocation location)
        : Statement(NodeKind::ErrorStatement, location) {}
};

class IfStatementNode final : public Statement {
public:
    IfStatementNode(SourceLocation location, NodePtr condition, NodePtr thenBranch,
                    NodePtr elseBranch = nullptr)
        : Statement(NodeKind::IfStatement, location), condition(std::move(condition)),
          thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}

    NodePtr condition;
    NodePtr thenBranch;
    NodePtr elseBranch;
};

class WhileStatementNode final : public Statement {
public:
    WhileStatementNode(SourceLocation location, NodePtr condition, NodePtr body)
        : Statement(NodeKind::WhileStatement, location), condition(std::move(condition)),
          body(std::move(body)) {}

    NodePtr condition;
    NodePtr body;
};

class ForStatementNode final : public Statement {
public:
    ForStatementNode(SourceLocation location, NodePtr initialization,
                     NodePtr condition, NodePtr update, NodePtr body)
        : Statement(NodeKind::ForStatement, location),
          initialization(std::move(initialization)), condition(std::move(condition)),
          update(std::move(update)), body(std::move(body)) {}

    NodePtr initialization;
    NodePtr condition;
    NodePtr update;
    NodePtr body;
};

class DoWhileStatementNode final : public Statement {
public:
    DoWhileStatementNode(SourceLocation location, NodePtr body, NodePtr condition)
        : Statement(NodeKind::DoWhileStatement, location), body(std::move(body)),
          condition(std::move(condition)) {}

    NodePtr body;
    NodePtr condition;
};

class ReturnStatementNode final : public Statement {
public:
    ReturnStatementNode(SourceLocation location, NodePtr expression = nullptr)
        : Statement(NodeKind::ReturnStatement, location),
          expression(std::move(expression)) {}

    NodePtr expression;
};

class IdentifierNode final : public Expression {
public:
    IdentifierNode(SourceLocation location, std::string name)
        : Expression(NodeKind::Identifier, location), name(std::move(name)) {}

    std::string name;
};

enum class LiteralKind { Integer, Floating, Character, String, Boolean };

class LiteralNode final : public Expression {
public:
    LiteralNode(SourceLocation location, LiteralKind literalKind, std::string value)
        : Expression(NodeKind::Literal, location), literalKind(literalKind),
          value(std::move(value)) {}

    LiteralKind literalKind;
    std::string value;
};

class UnaryExpressionNode final : public Expression {
public:
    UnaryExpressionNode(SourceLocation location, std::string operation,
                        NodePtr operand, bool prefix)
        : Expression(NodeKind::UnaryExpression, location),
          operation(std::move(operation)), operand(std::move(operand)), prefix(prefix) {}

    std::string operation;
    NodePtr operand;
    bool prefix;
};

class BinaryExpressionNode final : public Expression {
public:
    BinaryExpressionNode(SourceLocation location, std::string operation,
                         NodePtr left, NodePtr right)
        : Expression(NodeKind::BinaryExpression, location),
          operation(std::move(operation)), left(std::move(left)), right(std::move(right)) {}

    std::string operation;
    NodePtr left;
    NodePtr right;
};

class AssignmentExpressionNode final : public Expression {
public:
    AssignmentExpressionNode(SourceLocation location, std::string operation,
                             NodePtr target, NodePtr value)
        : Expression(NodeKind::AssignmentExpression, location),
          operation(std::move(operation)), target(std::move(target)),
          value(std::move(value)) {}

    std::string operation;
    NodePtr target;
    NodePtr value;
};

class FunctionCallNode final : public Expression {
public:
    FunctionCallNode(SourceLocation location, NodePtr callee, NodeList arguments)
        : Expression(NodeKind::FunctionCall, location), callee(std::move(callee)),
          arguments(std::move(arguments)) {}

    NodePtr callee;
    NodeList arguments;
};

class StreamOutputNode final : public Statement {
public:
    StreamOutputNode(SourceLocation location, std::string stream, NodePtr firstValue)
        : Statement(NodeKind::StreamOutput, location), stream(std::move(stream)) {
        values.push_back(std::move(firstValue));
    }

    std::string stream;
    NodeList values;
};

class StreamInputNode final : public Statement {
public:
    StreamInputNode(SourceLocation location, std::string stream, NodePtr firstTarget)
        : Statement(NodeKind::StreamInput, location), stream(std::move(stream)) {
        targets.push_back(std::move(firstTarget));
    }

    std::string stream;
    NodeList targets;
};

class PreprocessorNode final : public Node {
public:
    PreprocessorNode(SourceLocation location, std::string directive)
        : Node(NodeKind::Preprocessor, location), directive(std::move(directive)) {}

    std::string directive;
};

class UsingNamespaceNode final : public Node {
public:
    UsingNamespaceNode(SourceLocation location, std::string name)
        : Node(NodeKind::UsingNamespace, location), name(std::move(name)) {}

    std::string name;
};

const char* nodeKindName(NodeKind kind);
const char* literalKindName(LiteralKind kind);

} // namespace optiq::cpp

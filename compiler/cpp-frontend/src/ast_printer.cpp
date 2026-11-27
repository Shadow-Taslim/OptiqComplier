#include "optiq/cpp/ast_printer.hpp"

#include <ostream>
#include <string>

namespace optiq::cpp {
namespace {

void indent(std::ostream& output, int depth) {
    for (int i = 0; i < depth; ++i) output << "  ";
}

void heading(const Node& node, std::ostream& output, int depth,
             const std::string& detail = {}) {
    indent(output, depth);
    output << nodeKindName(node.kind);
    if (!detail.empty()) output << ' ' << detail;
    output << " @" << node.location.line << ':' << node.location.column << '\n';
}

void printNode(const Node* node, std::ostream& output, int depth);

void printChild(const char* label, const NodePtr& node, std::ostream& output,
                int depth) {
    indent(output, depth);
    output << label << ":\n";
    if (node) {
        printNode(node.get(), output, depth + 1);
    } else {
        indent(output, depth + 1);
        output << "<none>\n";
    }
}

void printList(const char* label, const NodeList& nodes, std::ostream& output,
               int depth) {
    indent(output, depth);
    output << label << " [" << nodes.size() << "]:\n";
    for (const auto& node : nodes) printNode(node.get(), output, depth + 1);
}

void printNode(const Node* node, std::ostream& output, int depth) {
    if (!node) return;

    switch (node->kind) {
    case NodeKind::Program: {
        const auto& value = static_cast<const ProgramNode&>(*node);
        heading(value, output, depth);
        printList("declarations", value.declarations, output, depth + 1);
        break;
    }
    case NodeKind::FunctionDeclaration: {
        const auto& value = static_cast<const FunctionDeclarationNode&>(*node);
        heading(value, output, depth, value.returnType + " " + value.name);
        printList("parameters", value.parameters, output, depth + 1);
        break;
    }
    case NodeKind::FunctionDefinition: {
        const auto& value = static_cast<const FunctionDefinitionNode&>(*node);
        heading(value, output, depth, value.returnType + " " + value.name);
        printList("parameters", value.parameters, output, depth + 1);
        printChild("body", value.body, output, depth + 1);
        break;
    }
    case NodeKind::Parameter: {
        const auto& value = static_cast<const ParameterNode&>(*node);
        heading(value, output, depth, value.typeName + " " + value.name);
        break;
    }
    case NodeKind::VariableDeclaration: {
        const auto& value = static_cast<const VariableDeclarationNode&>(*node);
        heading(value, output, depth, value.typeName + " " + value.name);
        if (value.initializer) printChild("initializer", value.initializer, output, depth + 1);
        break;
    }
    case NodeKind::BlockStatement: {
        const auto& value = static_cast<const BlockStatementNode&>(*node);
        heading(value, output, depth);
        printList("statements", value.statements, output, depth + 1);
        break;
    }
    case NodeKind::IfStatement: {
        const auto& value = static_cast<const IfStatementNode&>(*node);
        heading(value, output, depth);
        printChild("condition", value.condition, output, depth + 1);
        printChild("then", value.thenBranch, output, depth + 1);
        printChild("else", value.elseBranch, output, depth + 1);
        break;
    }
    case NodeKind::WhileStatement: {
        const auto& value = static_cast<const WhileStatementNode&>(*node);
        heading(value, output, depth);
        printChild("condition", value.condition, output, depth + 1);
        printChild("body", value.body, output, depth + 1);
        break;
    }
    case NodeKind::ForStatement: {
        const auto& value = static_cast<const ForStatementNode&>(*node);
        heading(value, output, depth);
        printChild("initialization", value.initialization, output, depth + 1);
        printChild("condition", value.condition, output, depth + 1);
        printChild("update", value.update, output, depth + 1);
        printChild("body", value.body, output, depth + 1);
        break;
    }
    case NodeKind::DoWhileStatement: {
        const auto& value = static_cast<const DoWhileStatementNode&>(*node);
        heading(value, output, depth);
        printChild("body", value.body, output, depth + 1);
        printChild("condition", value.condition, output, depth + 1);
        break;
    }
    case NodeKind::ReturnStatement: {
        const auto& value = static_cast<const ReturnStatementNode&>(*node);
        heading(value, output, depth);
        if (value.expression) printChild("value", value.expression, output, depth + 1);
        break;
    }
    case NodeKind::ExpressionStatement: {
        const auto& value = static_cast<const ExpressionStatementNode&>(*node);
        heading(value, output, depth);
        printChild("expression", value.expression, output, depth + 1);
        break;
    }
    case NodeKind::EmptyStatement:
    case NodeKind::ErrorStatement:
        heading(*node, output, depth);
        break;
    case NodeKind::AssignmentExpression: {
        const auto& value = static_cast<const AssignmentExpressionNode&>(*node);
        heading(value, output, depth, value.operation);
        printChild("target", value.target, output, depth + 1);
        printChild("value", value.value, output, depth + 1);
        break;
    }
    case NodeKind::BinaryExpression: {
        const auto& value = static_cast<const BinaryExpressionNode&>(*node);
        heading(value, output, depth, value.operation);
        printChild("left", value.left, output, depth + 1);
        printChild("right", value.right, output, depth + 1);
        break;
    }
    case NodeKind::UnaryExpression: {
        const auto& value = static_cast<const UnaryExpressionNode&>(*node);
        heading(value, output, depth,
                std::string(value.prefix ? "prefix " : "postfix ") + value.operation);
        printChild("operand", value.operand, output, depth + 1);
        break;
    }
    case NodeKind::FunctionCall: {
        const auto& value = static_cast<const FunctionCallNode&>(*node);
        heading(value, output, depth);
        printChild("callee", value.callee, output, depth + 1);
        printList("arguments", value.arguments, output, depth + 1);
        break;
    }
    case NodeKind::Identifier: {
        const auto& value = static_cast<const IdentifierNode&>(*node);
        heading(value, output, depth, value.name);
        break;
    }
    case NodeKind::Literal: {
        const auto& value = static_cast<const LiteralNode&>(*node);
        heading(value, output, depth,
                std::string(literalKindName(value.literalKind)) + " " + value.value);
        break;
    }
    case NodeKind::StreamOutput: {
        const auto& value = static_cast<const StreamOutputNode&>(*node);
        heading(value, output, depth, value.stream);
        printList("values", value.values, output, depth + 1);
        break;
    }
    case NodeKind::StreamInput: {
        const auto& value = static_cast<const StreamInputNode&>(*node);
        heading(value, output, depth, value.stream);
        printList("targets", value.targets, output, depth + 1);
        break;
    }
    case NodeKind::Preprocessor: {
        const auto& value = static_cast<const PreprocessorNode&>(*node);
        heading(value, output, depth, value.directive);
        break;
    }
    case NodeKind::UsingNamespace: {
        const auto& value = static_cast<const UsingNamespaceNode&>(*node);
        heading(value, output, depth, value.name);
        break;
    }
    }
}

} // namespace

void printAst(const Node& root, std::ostream& output) {
    printNode(&root, output, 0);
}

} // namespace optiq::cpp

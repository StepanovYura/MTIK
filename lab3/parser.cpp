#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <regex>

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    CONST_INT,
    CONST_FLOAT,
    CONST_STRING,
    CONST_BOOL,
    OPERATOR,
    DELIMITER,
    UNKNOWN,
    ERROR
};

struct Token {
    TokenType type;
    std::string value;
    size_t position;
};

enum class NodeType {
    Program,
    IncludeDirective,
    FunctionDef,
    Parameter,
    Block,
    VarDecl,
    AssignStmt,
    IfStmt,
    ElseStmt,
    ForStmt,
    ReturnStmt,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    ExpressionStmt,
    Identifier,
    Literal,
    Type
};

struct ASTNode {
    NodeType type;
    std::string value;

    std::vector<std::shared_ptr<ASTNode>> children;

    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right;

    ASTNode(NodeType t, const std::string& v = "")
        : type(t), value(v) {}
};

class Parser {
private:
    std::vector<Token> tokens;
    size_t current = 0;

private:
    Token peek() const {
        if (current >= tokens.size())
            return tokens.back();
        return tokens[current];
    }

    Token previous() const {
        return tokens[current - 1];
    }

    bool isAtEnd() const {
        return current >= tokens.size();
    }

    Token advance() {
        if (!isAtEnd())
            current++;
        return previous();
    }

    bool check(TokenType type, const std::string& value = "") const {
        if (isAtEnd())
            return false;
        if (peek().type != type)
            return false;
        if (!value.empty() && peek().value != value)
            return false;
        return true;
    }

    bool match(TokenType type, const std::string& value = "") {
        if (check(type, value)) {
            advance();
            return true;
        }
        return false;
    }

    Token consume(TokenType type, const std::string& value, const std::string& errorMessage) {
        if (check(type, value))
            return advance();
        syntaxError(errorMessage);
        return peek();
    }

    void syntaxError(const std::string& message) {
        std::cerr << "\nSyntax Error: " << message
                  << "\nNear token: " << peek().value
                  << "\nPosition: " << peek().position
                  << std::endl;
        throw std::runtime_error("Parsing failed");
    }

private:
    std::shared_ptr<ASTNode> parsePrimary() {
        if (match(TokenType::CONST_INT)) {
            return std::make_shared<ASTNode>(
                NodeType::Literal,
                previous().value
            );
        }
        if (match(TokenType::CONST_FLOAT)) {
            return std::make_shared<ASTNode>(
                NodeType::Literal,
                previous().value
            );
        }
        if (match(TokenType::CONST_STRING)) {
            return std::make_shared<ASTNode>(
                NodeType::Literal,
                previous().value
            );
        }
        if (match(TokenType::CONST_BOOL)) {
            return std::make_shared<ASTNode>(
                NodeType::Literal,
                previous().value
            );
        }
        if (match(TokenType::IDENTIFIER)) {
            std::string name = previous().value;
            // std::cout
            while (match(TokenType::OPERATOR, "::")) {
                Token next =
                    consume(TokenType::IDENTIFIER,
                            "",
                            "Expected identifier after ::");
                name += "::" + next.value;
            }

            // function call
            if (match(TokenType::DELIMITER, "(")) {
                auto callNode =
                    std::make_shared<ASTNode>(
                        NodeType::CallExpr,
                        name
                    );

                if (!check(TokenType::DELIMITER, ")")) {
                    do {
                        callNode->children.push_back(
                            parseExpression()
                        );
                    }
                    while (match(TokenType::DELIMITER, ","));
                }

                consume(TokenType::DELIMITER,
                        ")",
                        "Expected ')' after arguments");
                return callNode;
            }

            return std::make_shared<ASTNode>(
                NodeType::Identifier,
                name
            );
        }

        if (match(TokenType::DELIMITER, "(")) {
            auto expr = parseExpression();
            consume(TokenType::DELIMITER,
                    ")",
                    "Expected ')'");
            return expr;
        }

        syntaxError("Unexpected token in expression");
        return nullptr;
    }

    std::shared_ptr<ASTNode> parsePostfix() {
        auto node = parsePrimary();
        
        while (match(TokenType::OPERATOR, "++")) {
            auto unary =
                std::make_shared<ASTNode>(
                    NodeType::UnaryExpr,
                    "++"
                );
            unary->left = node;
            node = unary;
        }

        return node;
    }

    std::shared_ptr<ASTNode> parseUnary() {
        if (match(TokenType::OPERATOR, "-")) {
            auto unary =
                std::make_shared<ASTNode>(
                    NodeType::UnaryExpr,
                    "-"
                );
            unary->left = parseUnary();
            return unary;
        }

        if (match(TokenType::OPERATOR, "++")) {
            auto unary =
                std::make_shared<ASTNode>(
                    NodeType::UnaryExpr,
                    "++"
                );
            unary->left = parseUnary();
            return unary;
        }

        return parsePostfix();
    }

    std::shared_ptr<ASTNode> parseMultiplication() {
        auto left = parseUnary();
        while (check(TokenType::OPERATOR, "*") ||
               check(TokenType::OPERATOR, "/")) {

            std::string op = advance().value;
            auto right = parseUnary();
            auto binary =
                std::make_shared<ASTNode>(
                    NodeType::BinaryExpr,
                    op
                );
            binary->left = left;
            binary->right = right;
            left = binary;
        }

        return left;
    }

    std::shared_ptr<ASTNode> parseAddition() {
        auto left = parseMultiplication();
        while (check(TokenType::OPERATOR, "+") ||
               check(TokenType::OPERATOR, "-")) {

            std::string op = advance().value;
            auto right = parseMultiplication();
            auto binary =
                std::make_shared<ASTNode>(
                    NodeType::BinaryExpr,
                    op
                );
            binary->left = left;
            binary->right = right;
            left = binary;
        }

        return left;
    }

    std::shared_ptr<ASTNode> parseComparison() {
        auto left = parseAddition();
        while (check(TokenType::OPERATOR, "<") ||
               check(TokenType::OPERATOR, ">") ||
               check(TokenType::OPERATOR, "<=") ||
               check(TokenType::OPERATOR, ">=") ||
               check(TokenType::OPERATOR, "==") ||
               check(TokenType::OPERATOR, "!=")) {

            std::string op = advance().value;
            auto right = parseAddition();
            auto binary =
                std::make_shared<ASTNode>(
                    NodeType::BinaryExpr,
                    op
                );
            binary->left = left;
            binary->right = right;
            left = binary;
        }

        return left;
    }

    std::shared_ptr<ASTNode> parseLogical() {
        auto left = parseComparison();
        while (check(TokenType::OPERATOR, "&&") ||
               check(TokenType::OPERATOR, "||")) {

            std::string op = advance().value;
            auto right = parseComparison();
            auto binary =
                std::make_shared<ASTNode>(
                    NodeType::BinaryExpr,
                    op
                );
            binary->left = left;
            binary->right = right;
            left = binary;
        }

        return left;
    }

    std::shared_ptr<ASTNode> parseAssignment() {
        auto left = parseLogical();
        if (match(TokenType::OPERATOR, "=")) {
            auto right = parseAssignment();
            auto assign =
                std::make_shared<ASTNode>(
                    NodeType::AssignStmt,
                    "="
                );
            assign->left = left;
            assign->right = right;

            return assign;
        }

        return left;
    }

    std::shared_ptr<ASTNode> parseExpression() {
        return parseAssignment();
    }

private:
    std::shared_ptr<ASTNode> parseReturn() {
        consume(TokenType::KEYWORD,
                "return",
                "Expected return");
        auto node =
            std::make_shared<ASTNode>(
                NodeType::ReturnStmt
            );

        if (!check(TokenType::DELIMITER, ";")) {
            node->children.push_back(
                parseExpression()
            );
        }
        consume(TokenType::DELIMITER,
                ";",
                "Expected ';'");
        return node;
    }

    std::shared_ptr<ASTNode> parseVarDecl() {
        Token typeTok =
            consume(TokenType::KEYWORD,
                    "",
                    "Expected type");
        Token nameTok =
            consume(TokenType::IDENTIFIER,
                    "",
                    "Expected variable name");

        auto node =
            std::make_shared<ASTNode>(
                NodeType::VarDecl,
                nameTok.value
            );

        node->children.push_back(
            std::make_shared<ASTNode>(
                NodeType::Type,
                typeTok.value
            )
        );

        if (match(TokenType::OPERATOR, "=")) {
            node->children.push_back(
                parseExpression()
            );
        }

        consume(TokenType::DELIMITER,
                ";",
                "Expected ';'");

        return node;
    }

    std::shared_ptr<ASTNode> parseBlock() {
        consume(TokenType::DELIMITER,
                "{",
                "Expected '{'");

        auto block =
            std::make_shared<ASTNode>(
                NodeType::Block
            );

        while (!check(TokenType::DELIMITER, "}")) {
            if (isAtEnd()) {
                syntaxError(
                    "Expected '}' before end of file"
                );
            }

            block->children.push_back(
                parseStatement()
            );
        }

        consume(TokenType::DELIMITER,
                "}",
                "Expected '}'");

        return block;
    }

    std::shared_ptr<ASTNode> parseIf() {
        consume(TokenType::KEYWORD,
                "if",
                "Expected if");

        consume(TokenType::DELIMITER,
                "(",
                "Expected '('");

        auto condition = parseExpression();

        consume(TokenType::DELIMITER,
                ")",
                "Expected ')'");

        auto node =
            std::make_shared<ASTNode>(
                NodeType::IfStmt
            );

        node->children.push_back(condition);
        node->children.push_back(parseBlock());

        if (match(TokenType::KEYWORD, "else")) {
            auto elseNode =
                std::make_shared<ASTNode>(
                    NodeType::ElseStmt
                );

            elseNode->children.push_back(
                parseBlock()
            );
            node->children.push_back(elseNode);
        }

        return node;
    }

    std::shared_ptr<ASTNode> parseFor() {
        consume(TokenType::KEYWORD,
                "for",
                "Expected for");

        consume(TokenType::DELIMITER,
                "(",
                "Expected '('");

        auto node =
            std::make_shared<ASTNode>(
                NodeType::ForStmt
            );

        // init
        if (check(TokenType::KEYWORD, "int")) {
            node->children.push_back(
                parseVarDecl()
            );
        }
        else {
            auto init = parseExpression();
            consume(TokenType::DELIMITER,
                    ";",
                    "Expected ';'");

            node->children.push_back(init);
        }

        // condition
        node->children.push_back(
            parseExpression()
        );

        consume(TokenType::DELIMITER,
                ";",
                "Expected ';'");

        // increment
        node->children.push_back(
            parseExpression()
        );

        consume(TokenType::DELIMITER,
                ")",
                "Expected ')'");

        // body
        node->children.push_back(
            parseBlock()
        );

        return node;
    }

    std::shared_ptr<ASTNode> parseExpressionStatement() {
        if (check(TokenType::IDENTIFIER)) {
            if (current + 1 < tokens.size()) {
                Token next = tokens[current + 1];

                bool valid =
                    (next.type == TokenType::OPERATOR &&
                    (next.value == "=" ||
                    next.value == "::" ||
                    next.value == "++" ||
                    next.value == "<<")) ||

                    (next.type == TokenType::DELIMITER &&
                    next.value == "(");

                if (!valid) {
                    syntaxError(
                        "Unexpected identifier '" +
                        peek().value +
                        "'"
                    );
                }
            }
        }

        auto expr = parseExpression();

        // поддержка std::cout << ...
        while (match(TokenType::OPERATOR, "<<")) {
            auto right = parseExpression();

            auto binary =
                std::make_shared<ASTNode>(
                    NodeType::BinaryExpr,
                    "<<"
                );
            binary->left = expr;
            binary->right = right;
            expr = binary;
        }

        consume(TokenType::DELIMITER,
                ";",
                "Expected ';'");

        auto stmt =
            std::make_shared<ASTNode>(
                NodeType::ExpressionStmt
            );
        stmt->children.push_back(expr);

        return stmt;
    }

    std::shared_ptr<ASTNode> parseStatement() {
        if (check(TokenType::KEYWORD, "int")) {
            return parseVarDecl();
        }

        if (check(TokenType::KEYWORD, "if")) {
            return parseIf();
        }

        if (check(TokenType::KEYWORD, "for")) {
            return parseFor();
        }

        if (check(TokenType::KEYWORD, "return")) {
            return parseReturn();
        }

        return parseExpressionStatement();
    }

    std::shared_ptr<ASTNode> parseFunction() {
        Token returnType =
            consume(TokenType::KEYWORD,
                    "",
                    "Expected return type");

        Token functionName =
            consume(TokenType::IDENTIFIER,
                    "",
                    "Expected function name");

        auto function =
            std::make_shared<ASTNode>(
                NodeType::FunctionDef,
                functionName.value
            );

        function->children.push_back(
            std::make_shared<ASTNode>(
                NodeType::Type,
                returnType.value
            )
        );

        consume(TokenType::DELIMITER,
                "(",
                "Expected '('");

        // parameters
        if (!check(TokenType::DELIMITER, ")")) {
            do {
                Token paramType =
                    consume(TokenType::KEYWORD,
                            "",
                            "Expected parameter type");

                Token paramName =
                    consume(TokenType::IDENTIFIER,
                            "",
                            "Expected parameter name");

                auto param =
                    std::make_shared<ASTNode>(
                        NodeType::Parameter
                    );

                param->children.push_back(
                    std::make_shared<ASTNode>(
                        NodeType::Type,
                        paramType.value
                    )
                );

                param->children.push_back(
                    std::make_shared<ASTNode>(
                        NodeType::Identifier,
                        paramName.value
                    )
                );

                function->children.push_back(param);

            } while (match(TokenType::DELIMITER, ","));
        }

        consume(TokenType::DELIMITER,
                ")",
                "Expected ')'");

        function->children.push_back(
            parseBlock()
        );

        return function;
    }

    std::shared_ptr<ASTNode> parseInclude() {

        consume(TokenType::OPERATOR,
                "#",
                "Expected '#'");

        consume(TokenType::KEYWORD,
                "include",
                "Expected include");

        consume(TokenType::OPERATOR,
                "<",
                "Expected '<'");

        Token lib =
            consume(TokenType::IDENTIFIER,
                    "",
                    "Expected library name");

        consume(TokenType::OPERATOR,
                ">",
                "Expected '>'");

        auto include =
            std::make_shared<ASTNode>(
                NodeType::IncludeDirective,
                lib.value
            );

        return include;
    }

public:
    std::shared_ptr<ASTNode> parse(
        const std::vector<Token>& inputTokens
    ) {

        tokens = inputTokens;
        current = 0;

        auto program =
            std::make_shared<ASTNode>(
                NodeType::Program
            );

        while (!isAtEnd()) {
            if (check(TokenType::OPERATOR, "#")) {
                program->children.push_back(
                    parseInclude()
                );
                continue;
            }

            program->children.push_back(
                parseFunction()
            );
        }

        return program;
    }
};

std::string nodeTypeToString(NodeType type) {
    switch (type) {

        case NodeType::Program:
            return "Program";

        case NodeType::IncludeDirective:
            return "IncludeDirective";

        case NodeType::FunctionDef:
            return "FunctionDef";

        case NodeType::Parameter:
            return "Parameter";

        case NodeType::Block:
            return "Block";

        case NodeType::VarDecl:
            return "VarDecl";

        case NodeType::AssignStmt:
            return "AssignStmt";

        case NodeType::IfStmt:
            return "IfStmt";

        case NodeType::ElseStmt:
            return "ElseStmt";

        case NodeType::ForStmt:
            return "ForStmt";

        case NodeType::ReturnStmt:
            return "ReturnStmt";

        case NodeType::BinaryExpr:
            return "BinaryExpr";

        case NodeType::UnaryExpr:
            return "UnaryExpr";

        case NodeType::CallExpr:
            return "CallExpr";

        case NodeType::ExpressionStmt:
            return "ExpressionStmt";

        case NodeType::Identifier:
            return "Identifier";

        case NodeType::Literal:
            return "Literal";

        case NodeType::Type:
            return "Type";

        default:
            return "Unknown";
    }
}

void printTree(const std::shared_ptr<ASTNode>& node,
               const std::string& prefix = "",
               bool isLast = true) {

    std::cout << prefix;
    std::cout << (isLast ? "└── " : "├── ");
    std::cout << nodeTypeToString(node->type);
    if (!node->value.empty()) {
        std::cout << " : " << node->value;
    }
    std::cout << std::endl;
    std::vector<std::shared_ptr<ASTNode>> allChildren;

    if (node->left)
        allChildren.push_back(node->left);

    if (node->right)
        allChildren.push_back(node->right);

    for (auto& child : node->children)
        allChildren.push_back(child);

    for (size_t i = 0; i < allChildren.size(); ++i) {
        bool last = (i == allChildren.size() - 1);
        printTree(
            allChildren[i],
            prefix + (isLast ? "    " : "│   "),
            last
        );
    }
}

void printJSON(const std::shared_ptr<ASTNode>& node,
               std::ostream& out,
               int indent = 0) {

    std::string ind(indent, ' ');
    out << ind << "{\n";
    out << ind << "  \"type\": \""
        << nodeTypeToString(node->type)
        << "\"";

    if (!node->value.empty()) {
        out << ",\n";

        out << ind << "  \"value\": \""
            << node->value
            << "\"";
    }

    bool hasExtra =
        node->left ||
        node->right ||
        !node->children.empty();

    if (hasExtra)
        out << ",\n";
    else
        out << "\n";

    bool needComma = false;

    if (node->left) {
        out << ind << "  \"left\":\n";
        printJSON(node->left, out, indent + 4);
        needComma = true;
    }

    if (node->right) {
        if (needComma)
            out << ",\n";

        out << ind << "  \"right\":\n";
        printJSON(node->right, out, indent + 4);
        needComma = true;
    }

    if (!node->children.empty()) {
        if (needComma)
            out << ",\n";

        out << ind << "  \"children\": [\n";

        for (size_t i = 0; i < node->children.size(); ++i) {
            printJSON(node->children[i], out, indent + 4);

            if (i + 1 != node->children.size())
                out << ",\n";
        }
        out << "\n" << ind << "  ]\n";
    }
    out << ind << "}";
}

TokenType parseTokenType(const std::string& str) {

    if (str == "KEYWORD")
        return TokenType::KEYWORD;

    if (str == "IDENTIFIER")
        return TokenType::IDENTIFIER;

    if (str == "CONST_INT")
        return TokenType::CONST_INT;

    if (str == "CONST_FLOAT")
        return TokenType::CONST_FLOAT;

    if (str == "CONST_STRING")
        return TokenType::CONST_STRING;

    if (str == "CONST_BOOL")
        return TokenType::CONST_BOOL;

    if (str == "OPERATOR")
        return TokenType::OPERATOR;

    if (str == "DELIMITER")
        return TokenType::DELIMITER;

    return TokenType::UNKNOWN;
}

std::vector<Token> parseTokenFile(const std::string& text) {
    std::vector<Token> tokens;

    std::regex pattern(
        R"(\(\s*([^,]+?)\s*,\s*(.+?)\s*\))"
    );

    auto begin =
        std::sregex_iterator(
            text.begin(),
            text.end(),
            pattern
        );

    auto end = std::sregex_iterator();
    size_t pos = 0;

    for (auto it = begin; it != end; ++it) {
        std::string typeStr = (*it)[1];
        std::string value = (*it)[2];

        tokens.push_back({
            parseTokenType(typeStr),
            value,
            pos++
        });
    }

    return tokens;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout
            << "Usage: "
            << argv[0]
            << " <token_file>"
            << std::endl;

        return 1;
    }

    std::ifstream file(argv[1]);

    if (!file.is_open()) {
        std::cout
            << "Cannot open file: "
            << argv[1]
            << std::endl;

        return 1;
    }

    std::stringstream buffer;

    buffer << file.rdbuf();

    std::string tokenText = buffer.str();

    std::vector<Token> tokens =
        parseTokenFile(tokenText);

    try {

        Parser parser;
        auto ast = parser.parse(tokens);

        std::cout << "\n=========== AST TREE ===========\n\n";
        printTree(ast);

        std::cout << "\nParsing completed successfully.\n";
        std::ofstream out("ast_output.json");

        printJSON(ast, out);
        out.close();
        std::cout
            << "\nJSON AST saved to ast_output.json\n";
    }
    catch (const std::exception& e) {
        std::cerr
            << "\nParser error: "
            << e.what()
            << std::endl;
    }

    return 0;
}
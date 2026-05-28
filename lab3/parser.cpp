#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <regex>

// Типы лексем, распознаваемых лексическим анализатором
enum class TokenType {
    KEYWORD,        // ключевое слово (int, if, for, return и т.д.)
    IDENTIFIER,     // идентификатор (имя переменной, функции)
    CONST_INT,      // целочисленная константа
    CONST_FLOAT,    // вещественная константа
    CONST_STRING,   // строковая константа
    CONST_BOOL,     // булева константа (true/false)
    OPERATOR,       // оператор (+, -, =, <, >, &&, || и т.д.)
    DELIMITER,      // разделитель (;, (), {}, ,)
    UNKNOWN,        // неизвестный токен
    ERROR           // ошибочный токен
};

// Структура, представляющая лексему (токен)
struct Token {
    TokenType type;     // тип токена
    std::string value;  // текстовое значение
    size_t position;    // позиция в исходном потоке (для диагностики)
};

// Типы узлов абстрактного синтаксического дерева (AST)
enum class NodeType {
    Program,            // корневой узел всей программы
    IncludeDirective,   // директива #include
    FunctionDef,        // определение функции
    Parameter,          // параметр функции
    Block,              // блок операторов в фигурных скобках
    VarDecl,            // объявление переменной
    AssignStmt,         // оператор присваивания
    IfStmt,             // оператор if
    ElseStmt,           // оператор else
    ForStmt,            // оператор for
    ReturnStmt,         // оператор return
    BinaryExpr,         // бинарное выражение (a + b, x < y и т.д.)
    UnaryExpr,          // унарное выражение (++i, -x)
    CallExpr,           // вызов функции
    ExpressionStmt,     // выражение как оператор (например, a = b;)
    Identifier,         // идентификатор
    Literal,            // литерал (число, строка, true/false)
    Type                // тип данных (int, float и т.д.)
};

// Узел AST: тип, значение и дочерние узлы (через children, left/right)
struct ASTNode {
    NodeType type;      // тип узла
    std::string value;  // дополнительное значение (имя переменной, оператор и т.д.)

    // Список дочерних узлов для узлов с произвольным количеством детей
    std::vector<std::shared_ptr<ASTNode>> children;

    // Для бинарных и унарных выражений используется разделение на left/right
    std::shared_ptr<ASTNode> left;
    std::shared_ptr<ASTNode> right;

    ASTNode(NodeType t, const std::string& v = "")
        : type(t), value(v) {}
};

// Класс синтаксического анализатора (парсера)
class Parser {
private:
    std::vector<Token> tokens; // входной поток токенов
    size_t current = 0;        // текущая позиция в потоке

private:
    // --- Вспомогательные методы для работы с потоком токенов ---

    // Возвращает текущий токен без перемещения
    Token peek() const {
        if (current >= tokens.size())
            return tokens.back();
        return tokens[current];
    }

    // Возвращает предыдущий токен (после вызова advance)
    Token previous() const {
        return tokens[current - 1];
    }

    // Проверяет, достигнут ли конец потока
    bool isAtEnd() const {
        return current >= tokens.size();
    }

    // Перемещается к следующему токену и возвращает предыдущий
    Token advance() {
        if (!isAtEnd())
            current++;
        return previous();
    }

    // Проверяет, соответствует ли текущий токен заданному типу и (опционально) значению
    bool check(TokenType type, const std::string& value = "") const {
        if (isAtEnd())
            return false;
        if (peek().type != type)
            return false;
        if (!value.empty() && peek().value != value)
            return false;
        return true;
    }

    // Если текущий токен соответствует типу и значению – переходит к следующему и возвращает true
    bool match(TokenType type, const std::string& value = "") {
        if (check(type, value)) {
            advance();
            return true;
        }
        return false;
    }

    // Потребляет токен заданного типа и значения; иначе вызывает синтаксическую ошибку
    Token consume(TokenType type, const std::string& value, const std::string& errorMessage) {
        if (check(type, value))
            return advance();
        syntaxError(errorMessage);
        return peek();
    }

    // Выводит сообщение об ошибке и генерирует исключение
    void syntaxError(const std::string& message) {
        std::cerr << "\nSyntax Error: " << message
                  << "\nNear token: " << peek().value
                  << "\nPosition: " << peek().position
                  << std::endl;
        throw std::runtime_error("Parsing failed");
    }

private:
    // --- Методы разбора грамматических конструкций (рекурсивный спуск) ---

    // Разбор первичного выражения: литералы, идентификаторы, вызовы функций, выражения в скобках
    std::shared_ptr<ASTNode> parsePrimary() {
        // Литералы: целые, вещественные, строки, булевы константы
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
        // Идентификатор, возможно с квалификацией пространства имён (::)
        if (match(TokenType::IDENTIFIER)) {
            std::string name = previous().value;
            // Обработка оператора разрешения области видимости ::
            while (match(TokenType::OPERATOR, "::")) {
                Token next =
                    consume(TokenType::IDENTIFIER,
                            "",
                            "Expected identifier after ::");
                name += "::" + next.value;
            }

            // Если после идентификатора идёт '(' – это вызов функции
            if (match(TokenType::DELIMITER, "(")) {
                auto callNode =
                    std::make_shared<ASTNode>(
                        NodeType::CallExpr,
                        name
                    );
                // Разбор аргументов (список выражений, разделённых запятыми)
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
            // Обычный идентификатор (переменная)
            return std::make_shared<ASTNode>(
                NodeType::Identifier,
                name
            );
        }

        // Выражение в скобках: '(' выражение ')'
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

    // Постфиксные операторы (пока только ++)
    std::shared_ptr<ASTNode> parsePostfix() {
        auto node = parsePrimary();
        // Постфиксный инкремент
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

    // Унарные операторы: -, ++
    std::shared_ptr<ASTNode> parseUnary() {
        // Унарный минус
        if (match(TokenType::OPERATOR, "-")) {
            auto unary =
                std::make_shared<ASTNode>(
                    NodeType::UnaryExpr,
                    "-"
                );
            unary->left = parseUnary();
            return unary;
        }
        // Префиксный инкремент
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

    // Разбор умножения и деления (левоассоциативно)
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

    // Разбор сложения и вычитания
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

    // Разбор операций сравнения (<, >, <=, >=, ==, !=)
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

    // Разбор логических операций && и ||
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

    // Разбор присваивания (правоассоциативно)
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

    // Точка входа для разбора любого выражения
    std::shared_ptr<ASTNode> parseExpression() {
        return parseAssignment();
    }

private:
    // --- Разбор операторов языка ---

    // Оператор return
    std::shared_ptr<ASTNode> parseReturn() {
        consume(TokenType::KEYWORD,
                "return",
                "Expected return");
        auto node =
            std::make_shared<ASTNode>(
                NodeType::ReturnStmt
            );
        // После return может идти выражение (необязательно)
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

    // Объявление переменной (пока только int)
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
        // Под-узел для типа
        node->children.push_back(
            std::make_shared<ASTNode>(
                NodeType::Type,
                typeTok.value
            )
        );
        // Возможная инициализация
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

    // Блок операторов в фигурных скобках
    std::shared_ptr<ASTNode> parseBlock() {
        consume(TokenType::DELIMITER,
                "{",
                "Expected '{'");
        auto block =
            std::make_shared<ASTNode>(
                NodeType::Block
            );
        // Парсим операторы до закрывающей скобки
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

    // Оператор if (с возможной веткой else)
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
        node->children.push_back(parseBlock()); // тело then
        // Необязательный else
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

    // Оператор for
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
        // Инициализация: либо объявление переменной, либо выражение
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
        // Условие продолжения
        node->children.push_back(
            parseExpression()
        );
        consume(TokenType::DELIMITER,
                ";",
                "Expected ';'");
        // Инкремент
        node->children.push_back(
            parseExpression()
        );
        consume(TokenType::DELIMITER,
                ")",
                "Expected ')'");
        // Тело цикла
        node->children.push_back(
            parseBlock()
        );
        return node;
    }

    // Оператор-выражение (например, a = b; или cout << ...)
    std::shared_ptr<ASTNode> parseExpressionStatement() {
        // Простая проверка на недопустимый голый идентификатор
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
        // Поддержка цепочек с оператором << (например, cout << x)
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

    // Точка входа для разбора любого оператора (выбор по первому токену)
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

    // Разбор определения функции
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
        // Тип возвращаемого значения
        function->children.push_back(
            std::make_shared<ASTNode>(
                NodeType::Type,
                returnType.value
            )
        );
        consume(TokenType::DELIMITER,
                "(",
                "Expected '('");
        // Параметры функции (список тип имя, ...)
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
        // Тело функции – блок
        function->children.push_back(
            parseBlock()
        );
        return function;
    }

    // Разбор директивы #include
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
    // Главный метод парсинга всей программы
    std::shared_ptr<ASTNode> parse(
        const std::vector<Token>& inputTokens
    ) {
        tokens = inputTokens;
        current = 0;
        auto program =
            std::make_shared<ASTNode>(
                NodeType::Program
            );
        // Разбор последовательности директив include и определений функций
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

// Преобразование типа узла AST в строку для вывода
std::string nodeTypeToString(NodeType type) {
    switch (type) {
        case NodeType::Program:            return "Program";
        case NodeType::IncludeDirective:   return "IncludeDirective";
        case NodeType::FunctionDef:        return "FunctionDef";
        case NodeType::Parameter:          return "Parameter";
        case NodeType::Block:              return "Block";
        case NodeType::VarDecl:            return "VarDecl";
        case NodeType::AssignStmt:         return "AssignStmt";
        case NodeType::IfStmt:             return "IfStmt";
        case NodeType::ElseStmt:           return "ElseStmt";
        case NodeType::ForStmt:            return "ForStmt";
        case NodeType::ReturnStmt:         return "ReturnStmt";
        case NodeType::BinaryExpr:         return "BinaryExpr";
        case NodeType::UnaryExpr:          return "UnaryExpr";
        case NodeType::CallExpr:           return "CallExpr";
        case NodeType::ExpressionStmt:     return "ExpressionStmt";
        case NodeType::Identifier:         return "Identifier";
        case NodeType::Literal:            return "Literal";
        case NodeType::Type:               return "Type";
        default:                           return "Unknown";
    }
}

// Рекурсивная печать AST в виде дерева
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
    // Собираем всех детей (left, right и children)
    std::vector<std::shared_ptr<ASTNode>> allChildren;
    if (node->left)
        allChildren.push_back(node->left);
    if (node->right)
        allChildren.push_back(node->right);
    for (auto& child : node->children)
        allChildren.push_back(child);
    // Рекурсивный вывод каждого ребёнка
    for (size_t i = 0; i < allChildren.size(); ++i) {
        bool last = (i == allChildren.size() - 1);
        printTree(
            allChildren[i],
            prefix + (isLast ? "    " : "│   "),
            last
        );
    }
}

// Вывод AST в формате JSON
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

// Преобразование строки в перечисление TokenType (для загрузки токенов из файла)
TokenType parseTokenType(const std::string& str) {
    if (str == "KEYWORD")      return TokenType::KEYWORD;
    if (str == "IDENTIFIER")   return TokenType::IDENTIFIER;
    if (str == "CONST_INT")    return TokenType::CONST_INT;
    if (str == "CONST_FLOAT")  return TokenType::CONST_FLOAT;
    if (str == "CONST_STRING") return TokenType::CONST_STRING;
    if (str == "CONST_BOOL")   return TokenType::CONST_BOOL;
    if (str == "OPERATOR")     return TokenType::OPERATOR;
    if (str == "DELIMITER")    return TokenType::DELIMITER;
    return TokenType::UNKNOWN;
}

// Парсинг файла, содержащего текстовое представление токенов (формат: (ТИП, значение) )
std::vector<Token> parseTokenFile(const std::string& text) {
    std::vector<Token> tokens;
    // Регулярное выражение для захвата пар (тип, значение)
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

// Точка входа в программу: чтение файла с токенами, парсинг, вывод AST в консоль и JSON
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
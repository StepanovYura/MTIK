#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <iomanip>
#include <string>

// Узел абстрактного синтаксического дерева (AST) после загрузки из JSON
struct ASTNode {
    std::string type;                  // тип узла (FunctionDef, Block, BinaryExpr и т.д.)
    std::string value;                 // значение (имя переменной, оператор, литерал)
    std::vector<ASTNode> children;     // дочерние узлы (для последовательностей)
    std::unique_ptr<ASTNode> left;     // левый операнд (для бинарных/унарных выражений)
    std::unique_ptr<ASTNode> right;    // правый операнд (для бинарных выражений)
};

// Простой парсер JSON, восстанавливающий AST из текстового файла, сгенерированного парсером
class SimpleJSONParser {
private:
    std::string text;   // исходный JSON-текст
    size_t pos = 0;     // текущая позиция разбора

private:
    // Пропуск пробельных символов
    void skipWhitespace() {
        while (pos < text.size() && isspace(text[pos])) {
            pos++;
        }
    }

    // Проверка, что в текущей позиции находится заданная строка, и её пропуск
    bool match(const std::string& s) {
        skipWhitespace();
        if (text.substr(pos, s.size()) == s) {
            pos += s.size();
            return true;
        }
        return false;
    }

    // Разбор JSON-строки (заключена в двойные кавычки)
    std::string parseString() {
        skipWhitespace();
        if (text[pos] != '"')
            return "";
        pos++;
        std::string result;
        while (pos < text.size() && text[pos] != '"') {
            // Обработка escape-последовательностей (сохраняются как есть)
            if (text[pos] == '\\' && pos + 1 < text.size()) {
                result += text[pos];
                pos++;
                result += text[pos];
                pos++;
                continue;
            }
            result += text[pos++];
        }
        pos++; // пропустить закрывающую кавычку
        return result;
    }

    // Пропуск неизвестной структуры (используется при игнорировании нераспознанных полей)
    void skipUnknown() {
        skipWhitespace();
        if (text[pos] == '"') {
            parseString();
            return;
        }
        if (text[pos] == '{') {
            skipBraces('{', '}');
            return;
        }
        if (text[pos] == '[') {
            skipBraces('[', ']');
            return;
        }
        while (pos < text.size() &&
               text[pos] != ',' &&
               text[pos] != '}' &&
               text[pos] != ']') {
            pos++;
        }
    }

    // Пропуск вложенной структуры с заданными открывающей и закрывающей скобками
    void skipBraces(char open, char close) {
        int depth = 0;
        while (pos < text.size()) {
            if (text[pos] == open)
                depth++;
            else if (text[pos] == close) {
                depth--;
                if (depth == 0) {
                    pos++;
                    break;
                }
            }
            pos++;
        }
    }

    // Разбор JSON-массива (используется для поля children)
    std::vector<ASTNode> parseArray() {
        std::vector<ASTNode> arr;
        if (!match("["))
            return arr;
        while (pos < text.size()) {
            skipWhitespace();
            if (match("]"))
                break;
            arr.push_back(parseObject());
            skipWhitespace();
            match(",");
        }
        return arr;
    }

    // Разбор JSON-объекта, представляющего один узел AST
    ASTNode parseObject() {
        ASTNode node;
        if (!match("{"))
            return node;
        while (pos < text.size()) {
            skipWhitespace();
            if (match("}"))
                break;
            std::string key = parseString();
            match(":");
            if (key == "type") {
                node.type = parseString();
            }
            else if (key == "value") {
                node.value = parseString();
            }
            else if (key == "children") {
                node.children = parseArray();
            }
            else if (key == "left") {
                node.left = std::make_unique<ASTNode>(parseObject());
            }
            else if (key == "right") {
                node.right = std::make_unique<ASTNode>(parseObject());
            }
            else {
                skipUnknown(); // игнорировать незнакомые поля
            }
            skipWhitespace();
            match(",");
        }
        return node;
    }

public:
    // Основной метод: разбор всего JSON и возврат корня AST
    ASTNode parse(const std::string& input) {
        text = input;
        pos = 0;
        return parseObject();
    }
};

// Информация о символе (переменной, функции) для таблицы символов
struct Symbol {
    std::string name;          // имя символа
    std::string type;          // тип данных (int, float, bool, void и т.д.)
    std::string scope;         // область видимости (Global, имя функции, Block)
    bool initialized = false;  // была ли переменная инициализирована
    bool isFunction = false;   // является ли символ функцией
    std::string returnType;    // тип возвращаемого значения (для функций)
    std::vector<std::string> paramTypes; // типы параметров (для функций)
};

// Упрощённая запись символа для вывода таблицы (плоская структура)
struct SymbolEntry {
    std::string name;
    std::string type;
    std::string scope;
    bool initialized;
};

// Триада – внутреннее представление трёхадресного кода
struct Triad {
    std::string op;    // операция (+, -, =, ifFalse, call, return и т.д.)
    std::string arg1;  // первый операнд (имя переменной, константа или ссылка на триаду)
    std::string arg2;  // второй операнд или метка перехода
};

// Результат вычисления выражения (используется при семантическом разборе)
struct ExprResult {
    std::string value;  // представление результата (константа, имя переменной, ^N)
    std::string type;   // тип результата (int, float, bool, string, unknown)
};

// Класс семантического анализатора: проверка типов, построение таблицы символов, генерация триад
class SemanticAnalyzer {
private:
    // Стек областей видимости (каждая область – отображение имя → Symbol)
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    // Стек имён областей (для отладочного вывода)
    std::vector<std::string> scopeStack;
    // Плоский список символов для вывода итоговой таблицы
    std::vector<SymbolEntry> symbolLog;
    // Сгенерированные триады
    std::vector<Triad> triads;
    // Список семантических ошибок
    std::vector<std::string> errors;
    // Счётчик для индексации новых триад
    int triadIndex = 0;

private:
    // Вход в новую область видимости
    void enterScope(const std::string& name = "") {
        scopes.push_back({});
        if (name.empty())
            scopeStack.push_back("Block");
        else
            scopeStack.push_back(name);
    }

    // Выход из текущей области
    void exitScope() {
        if (!scopes.empty())
            scopes.pop_back();
        if (!scopeStack.empty())
            scopeStack.pop_back();
    }

    // Текущее имя области (для вывода)
    std::string currentScope() const {
        if (scopeStack.empty())
            return "Global";
        return scopeStack.back();
    }

    // Объявление символа в текущей области (с проверкой повторного объявления)
    bool declareSymbol(const std::string& name,
                       const std::string& type,
                       bool initialized = false) {
        auto& scope = scopes.back();
        if (scope.count(name)) {
            errors.push_back(
                "Semantic Error: redeclaration of '" +
                name + "'"
            );
            return false;
        }
        scope[name] = {
            name,
            type,
            currentScope(),
            initialized
        };
        symbolLog.push_back({
            name,
            type,
            currentScope(),
            initialized
        });
        return true;
    }

    // Поиск символа по имени с учётом вложенных областей (от текущей к глобальной)
    Symbol* resolve(const std::string& name) {
        for (int i = scopes.size() - 1; i >= 0; --i) {
            auto it = scopes[i].find(name);
            if (it != scopes[i].end())
                return &it->second;
        }
        errors.push_back(
            "Semantic Error: undeclared identifier '" +
            name + "'"
        );
        return nullptr;
    }

    // Проверка совместимости типов (с учётом неявного приведения int → float)
    bool compatibleTypes(const std::string& left,
                         const std::string& right) {
        if (left == right)
            return true;
        if (left == "float" && right == "int")
            return true;
        return false;
    }

    // Проверка, является ли тип числовым
    bool isNumeric(const std::string& type) {
        return type == "int" || type == "float";
    }

    // Проверка, является ли тип булевым
    bool isBool(const std::string& type) {
        return type == "bool";
    }

    // Добавление новой триады, возвращает её индекс
    int emit(const std::string& op,
             const std::string& arg1 = "",
             const std::string& arg2 = "") {
        triads.push_back({op, arg1, arg2});
        return triadIndex++;
    }

private:
    // Рекурсивный семантический разбор выражения, возвращает тип и представление результата
    ExprResult evaluateExpression(const ASTNode& node) {
        // Литерал: число, строка, булево значение
        if (node.type == "Literal") {
            // Булевы константы
            if (node.value == "true" || node.value == "false") {
                return {node.value, "bool"};
            }
            // Числа с плавающей точкой (содержат точку)
            if (node.value.find('.') != std::string::npos) {
                return {node.value, "float"};
            }
            // Строковые литералы (содержат нецифровые символы, кроме точки)
            bool isNumber = true;
            for (char c : node.value) {
                if (!isdigit(c) && c != '.') {
                    isNumber = false;
                    break;
                }
            }
            if (!isNumber) {
                return {node.value, "string"};
            }
            // Целочисленный литерал
            return {node.value, "int"};
        }
        // Идентификатор: поиск в таблице символов, проверка инициализации
        if (node.type == "Identifier") {
            Symbol* sym = resolve(node.value);
            if (!sym) {
                return {node.value, "unknown"};
            }
            if (!sym->initialized) {
                errors.push_back(
                    "Semantic Error: variable '" +
                    node.value +
                    "' used uninitialized"
                );
            }
            return {node.value, sym->type};
        }
        // Унарное выражение (например, ++x)
        if (node.type == "UnaryExpr") {
            auto expr = evaluateExpression(*node.left);
            if (node.value == "++") {
                if (!isNumeric(expr.type)) {
                    errors.push_back(
                        "Semantic Error: operator ++ "
                        "requires numeric operand"
                    );
                }
            }
            int idx = emit(node.value, expr.value, "");
            return {"^" + std::to_string(idx), expr.type};
        }
        // Бинарное выражение (арифметика, сравнения, логика)
        if (node.type == "BinaryExpr") {
            auto left = evaluateExpression(*node.left);
            auto right = evaluateExpression(*node.right);

            // Арифметические операции
            if (node.value == "+" ||
                node.value == "-" ||
                node.value == "*" ||
                node.value == "/") {
                if (!isNumeric(left.type) || !isNumeric(right.type)) {
                    errors.push_back(
                        "Semantic Error: arithmetic "
                        "operator '" +
                        node.value +
                        "' requires numeric operands"
                    );
                }
                if (node.value == "/" && right.value == "0") {
                    errors.push_back(
                        "Semantic Warning: division by zero"
                    );
                }
            }
            // Логические операции (&&, ||) – требуют bool
            if (node.value == "&&" || node.value == "||") {
                if (!isBool(left.type) || !isBool(right.type)) {
                    errors.push_back(
                        "Semantic Error: logical "
                        "operator '" +
                        node.value +
                        "' requires bool operands"
                    );
                }
            }
            // Операции сравнения – возвращают bool
            if (node.value == "<" ||
                node.value == ">" ||
                node.value == "<=" ||
                node.value == ">=" ||
                node.value == "==" ||
                node.value == "!=") {
                if (!compatibleTypes(left.type, right.type)) {
                    errors.push_back(
                        "Semantic Error: incompatible "
                        "types in comparison"
                    );
                }
                int idx = emit(node.value, left.value, right.value);
                return {"^" + std::to_string(idx), "bool"};
            }
            // Общий случай бинарной операции (арифметика)
            int idx = emit(node.value, left.value, right.value);
            std::string resultType =
                (left.type == "float" || right.type == "float")
                ? "float"
                : left.type;
            return {"^" + std::to_string(idx), resultType};
        }
        // Оператор присваивания
        if (node.type == "AssignStmt") {
            auto left = evaluateExpression(*node.left);
            auto right = evaluateExpression(*node.right);
            if (!compatibleTypes(left.type, right.type)) {
                errors.push_back(
                    "Semantic Error: cannot assign '" +
                    right.type +
                    "' to '" +
                    left.type +
                    "'"
                );
            }
            emit("=", left.value, right.value);
            // Пометить переменную как инициализированную
            Symbol* sym = resolve(left.value);
            if (sym)
                sym->initialized = true;
            return left;
        }
        // Вызов функции
        if (node.type == "CallExpr") {
            Symbol* func = resolve(node.value);
            if (!func || !func->isFunction) {
                errors.push_back(
                    "Semantic Error: '" +
                    node.value +
                    "' is not a function"
                );
                return {node.value, "unknown"};
            }
            // Проверка количества аргументов
            if (node.children.size() != func->paramTypes.size()) {
                errors.push_back(
                    "Semantic Error: function '" +
                    node.value +
                    "' expects " +
                    std::to_string(func->paramTypes.size()) +
                    " arguments"
                );
            }
            // Проверка типов аргументов
            for (size_t i = 0;
                 i < node.children.size() && i < func->paramTypes.size();
                 ++i) {
                auto arg = evaluateExpression(node.children[i]);
                if (!compatibleTypes(func->paramTypes[i], arg.type)) {
                    errors.push_back(
                        "Semantic Error: argument type "
                        "mismatch in function '" +
                        node.value + "'"
                    );
                }
                emit("param", arg.value, "");
            }
            int idx = emit("call", node.value, "");
            return {"^" + std::to_string(idx), func->returnType};
        }
        // Неизвестный тип узла
        return {"unknown", "unknown"};
    }

private:
    // Анализ одного оператора (объявление, выражение, return, if, for)
    void analyzeStatement(const ASTNode& node) {
        // Объявление переменной
        if (node.type == "VarDecl") {
            std::string name = node.value;
            std::string type = node.children[0].value;
            bool initialized = node.children.size() > 1;
            if (!declareSymbol(name, type, initialized)) {
                return;
            }
            if (initialized) {
                auto expr = evaluateExpression(node.children[1]);
                if (!compatibleTypes(type, expr.type)) {
                    errors.push_back(
                        "Semantic Error: cannot initialize '" +
                        type +
                        "' with '" +
                        expr.type +
                        "'"
                    );
                }
                emit("=", name, expr.value);
            }
            return;
        }
        // Оператор-выражение (например, a = b; или cout << ...)
        if (node.type == "ExpressionStmt") {
            if (!node.children.empty()) {
                evaluateExpression(node.children[0]);
            }
            return;
        }
        // Оператор return
        if (node.type == "ReturnStmt") {
            // Поиск текущей функции (по области видимости)
            Symbol* currentFunc = nullptr;
            for (int i = scopes.size() - 1; i >= 0; --i) {
                for (auto& [_, sym] : scopes[i]) {
                    if (sym.isFunction) {
                        currentFunc = &sym;
                    }
                }
                if (currentFunc)
                    break;
            }
            if (!currentFunc) {
                errors.push_back(
                    "Semantic Error: return "
                    "outside function"
                );
                return;
            }
            // return без выражения
            if (node.children.empty()) {
                if (currentFunc->returnType != "void") {
                    errors.push_back(
                        "Semantic Error: non-void "
                        "function must return value"
                    );
                }
                emit("return", "", "");
                return;
            }
            // return с выражением
            auto expr = evaluateExpression(node.children[0]);
            if (!compatibleTypes(currentFunc->returnType, expr.type)) {
                errors.push_back(
                    "Semantic Error: return type mismatch"
                );
            }
            emit("return", expr.value, "");
            return;
        }
        // Оператор if (с возможной веткой else)
        if (node.type == "IfStmt") {
            auto cond = evaluateExpression(node.children[0]);
            if (!isBool(cond.type) && !isNumeric(cond.type)) {
                errors.push_back(
                    "Semantic Error: if condition "
                    "must be bool"
                );
            }
            // Генерация условного перехода (ifFalse)
            int ifFalse = emit("ifFalse", cond.value, "");
            enterScope("if");
            analyzeBlock(node.children[1]);  // тело then
            exitScope();
            // Если есть else
            if (node.children.size() > 2) {
                int go = emit("goto", "", "");
                triads[ifFalse].arg2 = std::to_string(triadIndex);
                analyzeStatement(node.children[2]); // else-блок
                triads[go].arg1 = std::to_string(triadIndex);
            } else {
                triads[ifFalse].arg2 = std::to_string(triadIndex);
            }
            return;
        }
        // Узел ElseStmt (обрабатывается как блок внутри IfStmt)
        if (node.type == "ElseStmt") {
            enterScope("else");
            analyzeBlock(node.children[0]);
            exitScope();
            return;
        }
        // Оператор for
        if (node.type == "ForStmt") {
            enterScope("for");
            // Инициализация
            analyzeStatement(node.children[0]);
            int start = triadIndex;
            // Условие
            auto cond = evaluateExpression(node.children[1]);
            int ifFalse = emit("ifFalse", cond.value, "");
            // Тело цикла
            analyzeBlock(node.children[3]);
            // Инкремент
            evaluateExpression(node.children[2]);
            // Безусловный переход на проверку условия
            emit("goto", std::to_string(start), "");
            triads[ifFalse].arg2 = std::to_string(triadIndex);
            exitScope();
            return;
        }
    }

    // Анализ блока (последовательность операторов)
    void analyzeBlock(const ASTNode& block) {
        for (const auto& stmt : block.children) {
            analyzeStatement(stmt);
        }
    }

    // Анализ определения функции
    void analyzeFunction(const ASTNode& node) {
        std::string name = node.value;
        std::string returnType = node.children[0].value;
        if (!declareSymbol(name, returnType + "(func)", true)) {
            return;
        }
        Symbol* func = resolve(name);
        func->isFunction = true;
        func->returnType = returnType;
        emit("func", name, "");
        enterScope(name);
        // Добавление параметров в таблицу символов
        for (const auto& child : node.children) {
            if (child.type == "Parameter") {
                std::string paramType = child.children[0].value;
                std::string paramName = child.children[1].value;
                func->paramTypes.push_back(paramType);
                declareSymbol(paramName, paramType, true);
            }
        }
        // Анализ тела функции (блока)
        for (const auto& child : node.children) {
            if (child.type == "Block") {
                analyzeBlock(child);
            }
        }
        exitScope();
    }

public:
    // Главная точка входа: анализ всего AST (корень типа Program)
    void analyze(const ASTNode& root) {
        enterScope("Global");
        // Предопределённые символы для потокового ввода-вывода (упрощение)
        declareSymbol("std::cout", "ostream", true);
        declareSymbol("std::endl", "manipulator", true);
        for (const auto& child : root.children) {
            if (child.type == "FunctionDef") {
                analyzeFunction(child);
            }
        }
        exitScope();
    }

public:
    int getErrorCount() const {
        return errors.size();
    }

    void printErrors(std::ostream& out) {
        out << "=========== ERRORS ===========\n\n";
        for (const auto& err : errors) {
            out << err << std::endl;
        }
    }

    void printSymbolTable(std::ostream& out) {
        out << "=========== SYMBOL TABLE ===========\n\n";
        out << std::left
            << std::setw(15) << "Name"
            << std::setw(15) << "Type"
            << std::setw(20) << "Scope"
            << std::setw(15) << "Initialized"
            << "\n";
        out << std::string(65, '-') << "\n";
        for (const auto& s : symbolLog) {
            out << std::left
                << std::setw(15) << s.name
                << std::setw(15) << s.type
                << std::setw(20) << s.scope
                << std::setw(15) << (s.initialized ? "yes" : "no")
                << "\n";
        }
    }

    void printTriads(std::ostream& out) {
        out << "=========== TRIADS ===========\n\n";
        for (size_t i = 0; i < triads.size(); ++i) {
            out << i
                << ": ("
                << triads[i].op
                << ", "
                << triads[i].arg1
                << ", "
                << triads[i].arg2
                << ")"
                << std::endl;
        }
    }
};

// Точка входа: чтение JSON-файла с AST, семантический анализ, вывод результатов
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout
            << "Usage: "
            << argv[0]
            << " <ast_json_file>"
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
    std::string jsonText = buffer.str();

    SimpleJSONParser parser;
    ASTNode root = parser.parse(jsonText);

    SemanticAnalyzer analyzer;
    analyzer.analyze(root);

    std::ofstream errorsFile("semantic_errors.txt");
    std::ofstream symbolsFile("symbol_table.txt");
    std::ofstream triadsFile("triads.txt");

    if (analyzer.getErrorCount() == 0) {
        std::cout
            << "\nSemantic analysis "
            << "completed successfully.\n";
        errorsFile << "No semantic errors.\n";
    } else {
        std::cout
            << "\nSemantic analysis finished "
            << "with errors.\n";
        analyzer.printErrors(errorsFile);
        analyzer.printErrors(std::cout);
    }

    analyzer.printSymbolTable(symbolsFile);
    analyzer.printTriads(triadsFile);

    errorsFile.close();
    symbolsFile.close();
    triadsFile.close();

    std::cout
        << "\nGenerated files:\n"
        << "- semantic_errors.txt\n"
        << "- symbol_table.txt\n"
        << "- triads.txt\n";

    return 0;
}
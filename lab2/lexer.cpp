#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

// Типы токенов
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

// Структура токена
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
public:
    Lexer(const std::string& source)
        : source(source), pos(0), line(1), col(1) {}

    std::vector<Token> tokenize() {
        tokens.clear();
        while (pos < source.size()) {
            char ch = source[pos];
            if (std::isspace(ch)) {
                skipWhitespace();
                continue;
            }
            if (std::isalpha(ch) || ch == '_') {
                readIdentifierOrKeyword();
            } else if (std::isdigit(ch)) {
                readNumber();
            } else if (ch == '"') {
                readString();
            } else {
                readOperatorOrDelimiter();
            }
        }
        return tokens;
    }

    void printTokens() const {
        // Определяем максимальную длину лексемы
        size_t maxLen = 8; // минимальная длина заголовка "Лексема"
        for (const auto& tok : tokens) {
            if (tok.value.length() > maxLen) maxLen = tok.value.length();
        }
        const int widthLexeme = static_cast<int>(maxLen) + 2;  // +2 для отступа
        const int widthType = 16;  // достаточно для "IDENTIFIER", "CONST_STRING" и т.п.

        // Заголовок
        std::cout << "\n" << std::left << std::setw(widthLexeme) << "Лексема"
                << " | " << std::setw(widthType) << "Тип" << "\n";
        // Разделительная линия
        std::cout << std::string(widthLexeme + widthType + 3, '-') << "\n";

        // Данные
        for (const auto& tok : tokens) {
            std::cout << std::left << std::setw(widthLexeme) << tok.value
                    << " | " << std::setw(widthType) << tokenTypeToString(tok.type) << "\n";
        }
    }

    void printTokenSequence() const {
        std::cout << "\n[";
        for (size_t i = 0; i < tokens.size(); ++i) {
            std::cout << "(" << tokenTypeToString(tokens[i].type) << ", " << tokens[i].value << ")";
            if (i != tokens.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }

    bool hasErrors() const { return !errors.empty(); }
    void printErrors() const {
        for (const auto& err : errors) {
            std::cout << err << std::endl;
        }
    }

private:
    std::string source;
    size_t pos;
    int line;
    int col;
    std::vector<Token> tokens;
    std::vector<std::string> errors;

    // Таблицы лексем
    const std::unordered_set<std::string> keywords = {
        "int", "if", "else", "for", "return", "include"
    };
    const std::unordered_map<std::string, TokenType> operators = {
        {"=", TokenType::OPERATOR}, {"+", TokenType::OPERATOR}, {"-", TokenType::OPERATOR},
        {"*", TokenType::OPERATOR}, {"/", TokenType::OPERATOR}, {"<", TokenType::OPERATOR},
        {">", TokenType::OPERATOR}, {"&&", TokenType::OPERATOR}, {"||", TokenType::OPERATOR},
        {"==", TokenType::OPERATOR}, {"!=", TokenType::OPERATOR}, {"<=", TokenType::OPERATOR},
        {">=", TokenType::OPERATOR}, {"++", TokenType::OPERATOR}, {"--", TokenType::OPERATOR},
        {"<<", TokenType::OPERATOR}, {">>", TokenType::OPERATOR}, {"::", TokenType::OPERATOR},
        {"#", TokenType::OPERATOR}
    };
    const std::unordered_set<char> delimiterChars = {
        ';', ',', '(', ')', '{', '}', '[', ']', '.'
    };
    const std::unordered_set<std::string> boolConstants = {"true", "false"};

    std::string tokenTypeToString(TokenType type) const {
        switch (type) {
            case TokenType::KEYWORD:      return "KEYWORD";
            case TokenType::IDENTIFIER:   return "IDENTIFIER";
            case TokenType::CONST_INT:    return "CONST_INT";
            case TokenType::CONST_FLOAT:  return "CONST_FLOAT";
            case TokenType::CONST_STRING: return "CONST_STRING";
            case TokenType::CONST_BOOL:   return "CONST_BOOL";
            case TokenType::OPERATOR:     return "OPERATOR";
            case TokenType::DELIMITER:    return "DELIMITER";
            default:                      return "UNKNOWN";
        }
    }

    void skipWhitespace() {
        while (pos < source.size() && std::isspace(source[pos])) {
            if (source[pos] == '\n') { line++; col = 1; }
            else col++;
            pos++;
        }
    }

    void readIdentifierOrKeyword() {
        int startLine = line, startCol = col;
        std::string ident;
        while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_')) {
            ident += source[pos];
            if (source[pos] == '\n') { line++; col = 1; }
            else col++;
            pos++;
        }
        // Проверка, что идентификатор не начинается с цифры (уже гарантировано условием вызова)
        if (std::isdigit(ident[0])) {
            errors.push_back("Ошибка: идентификатор '" + ident + "' начинается с цифры (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, ident, startLine, startCol);
            return;
        }
        // Проверка, является ли ключевым словом
        if (keywords.count(ident)) {
            tokens.emplace_back(TokenType::KEYWORD, ident, startLine, startCol);
        } else if (boolConstants.count(ident)) {
            tokens.emplace_back(TokenType::CONST_BOOL, ident, startLine, startCol);
        } else {
            tokens.emplace_back(TokenType::IDENTIFIER, ident, startLine, startCol);
        }
    }

    void readNumber() {
        int startLine = line, startCol = col;
        std::string num;
        bool hasDot = false;
        while (pos < source.size() && (std::isdigit(source[pos]) || source[pos] == '.')) {
            if (source[pos] == '.') {
                if (hasDot) {
                    errors.push_back("Ошибка: две точки подряд в числе '" + num + "' (строка " +
                                     std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
                    while (pos < source.size() && (std::isdigit(source[pos]) || source[pos] == '.')) {
                        num += source[pos];
                        pos++;
                    }
                    tokens.emplace_back(TokenType::ERROR, num, startLine, startCol);
                    return;
                }
                hasDot = true;
            }
            num += source[pos];
            pos++;
        }
        // После числа может быть буква – это ошибка
        if (pos < source.size() && std::isalpha(source[pos])) {
            errors.push_back("Ошибка: недопустимый символ в числовой константе '" + num + source[pos] + "' (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            while (pos < source.size() && std::isalpha(source[pos])) {
                num += source[pos];
                pos++;
            }
            tokens.emplace_back(TokenType::ERROR, num, startLine, startCol);
            return;
        }
        if (hasDot)
            tokens.emplace_back(TokenType::CONST_FLOAT, num, startLine, startCol);
        else
            tokens.emplace_back(TokenType::CONST_INT, num, startLine, startCol);
    }

    void readString() {
        int startLine = line, startCol = col;
        std::string str;
        pos++; // пропустить открывающую кавычку
        col++;
        bool closed = false;
        while (pos < source.size()) {
            char ch = source[pos];
            if (ch == '"') {
                closed = true;
                pos++;
                col++;
                break;
            }
            if (ch == '\\') {
                // Экранирование – упрощённо
                str += ch;
                pos++; col++;
                if (pos < source.size()) {
                    str += source[pos];
                    pos++; col++;
                }
            } else {
                str += ch;
                pos++; col++;
            }
        }
        if (!closed) {
            errors.push_back("Ошибка: незакрытая строковая константа (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, str, startLine, startCol);
        } else {
            tokens.emplace_back(TokenType::CONST_STRING, str, startLine, startCol);
        }
    }

    void readOperatorOrDelimiter() {
        int startLine = line, startCol = col;
        char ch = source[pos];
        // Сначала проверяем, не является ли символ разделителем
        if (delimiterChars.count(ch)) {
            tokens.emplace_back(TokenType::DELIMITER, std::string(1, ch), startLine, startCol);
            pos++; col++;
            return;
        }
        // Пытаемся считать оператор (максимальное сопоставление)
        std::string op;
        while (pos < source.size() && !std::isspace(source[pos]) && !delimiterChars.count(source[pos]) &&
               !std::isalnum(source[pos]) && source[pos] != '_') {
            op += source[pos];
            pos++;
        }
        // Проверяем, известен ли оператор
        if (operators.count(op)) {
            tokens.emplace_back(TokenType::OPERATOR, op, startLine, startCol);
        } else {
            errors.push_back("Ошибка: неизвестный оператор '" + op + "' (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, op, startLine, startCol);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    // Чтение очищенного файла (результат ЛР1)
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Лексический анализ
    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    // Вывод результатов
    lexer.printTokens();
    lexer.printTokenSequence();

    if (lexer.hasErrors()) {
        std::cout << "\nЛексические ошибки:\n";
        lexer.printErrors();
        std::cout << "Лексический анализ завершён с ошибками.\n";
    } else {
        std::cout << "\nЛексический анализ завершён успешно. Обнаружено " << tokens.size() << " токенов. Ошибок не найдено.\n";
    }

    return 0;
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <unordered_map>
#include <unordered_set>

enum class TokenType {
    KEYWORD, IDENTIFIER, CONST_INT, CONST_FLOAT, CONST_STRING,
    CONST_BOOL, OPERATOR, DELIMITER, UNKNOWN, ERROR
};

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
    Lexer(const std::string& source) : source(source), pos(0), line(1), col(1) {}

    std::vector<Token> tokenize() {
        tokens.clear();
        while (pos < source.size()) {
            char ch = source[pos];
            if (std::isspace(static_cast<unsigned char>(ch))) {
                skipWhitespace();
                continue;
            }
            if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
                readIdentifierOrKeyword();
            } else if (std::isdigit(static_cast<unsigned char>(ch))) {
                readNumber();
            } else if (ch == '"') {
                readString();
            } else {
                readOperatorOrDelimiter();
            }
        }
        validateIncludeDirective();
        return tokens;
    }

    // Перегруженные методы для вывода в поток
    void printTokens(std::ostream& os) const {
        size_t maxLen = 8;
        for (const auto& tok : tokens)
            if (tok.value.length() > maxLen) maxLen = tok.value.length();
        const int widthLexeme = static_cast<int>(maxLen) + 2;
        const int widthType = 16;
        os << "\n" << std::left << std::setw(widthLexeme) << "Лексема"
           << " | " << std::setw(widthType) << "Тип" << "\n";
        os << std::string(widthLexeme + widthType + 3, '-') << "\n";
        for (const auto& tok : tokens)
            os << std::left << std::setw(widthLexeme) << tok.value
               << " | " << std::setw(widthType) << tokenTypeToString(tok.type) << "\n";
    }
    void printTokens() const { printTokens(std::cout); }

    void printTokenSequence(std::ostream& os) const {
        os << "\n[\n\t";
        for (size_t i = 0; i < tokens.size(); ++i) {
            os << "(" << tokenTypeToString(tokens[i].type) << ", " << tokens[i].value << ")";
            if (i != tokens.size() - 1) os << ", \n\t";
        }
        os << "\n]\n";
    }
    void printTokenSequence() const { printTokenSequence(std::cout); }

    bool hasErrors() const { return !errors.empty(); }

    void printErrors(std::ostream& os) const {
        for (const auto& err : errors) os << err << std::endl;
    }
    void printErrors() const { printErrors(std::cout); }

    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file) {
            std::cerr << "Не удалось создать файл: " << filename << std::endl;
            return;
        }
        printTokenSequence(file);
        
        std::cout << "Результат также сохранён в файл: " << filename << std::endl;
    }

private:
    std::string source;
    size_t pos;
    int line, col;
    std::vector<Token> tokens;
    std::vector<std::string> errors;

    const std::unordered_set<std::string> keywords = {"int", "if", "else", "for", "return", "include"};
    const std::unordered_map<std::string, TokenType> operators = {
        {"=", TokenType::OPERATOR}, {"+", TokenType::OPERATOR}, {"-", TokenType::OPERATOR},
        {"*", TokenType::OPERATOR}, {"/", TokenType::OPERATOR}, {"<", TokenType::OPERATOR},
        {">", TokenType::OPERATOR}, {"&&", TokenType::OPERATOR}, {"||", TokenType::OPERATOR},
        {"==", TokenType::OPERATOR}, {"!=", TokenType::OPERATOR}, {"<=", TokenType::OPERATOR},
        {">=", TokenType::OPERATOR}, {"++", TokenType::OPERATOR}, {"--", TokenType::OPERATOR},
        {"<<", TokenType::OPERATOR}, {">>", TokenType::OPERATOR}, {"::", TokenType::OPERATOR},
        {"#", TokenType::OPERATOR}
    };
    const std::unordered_set<char> delimiterChars = {';', ',', '(', ')', '{', '}', '[', ']', '.'};
    const std::unordered_set<std::string> boolConstants = {"true", "false"};
    const std::unordered_set<char> invalidChars = {'@', '$', '`', '~', '^'};

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
        while (pos < source.size() && std::isspace(static_cast<unsigned char>(source[pos]))) {
            if (source[pos] == '\n') { line++; col = 1; }
            else col++;
            pos++;
        }
    }

    void readIdentifierOrKeyword() {
        int startLine = line, startCol = col;
        std::string ident;
        while (pos < source.size() && (std::isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_')) {
            ident += source[pos];
            if (source[pos] == '\n') { line++; col = 1; }
            else col++;
            pos++;
        }
        if (std::isdigit(static_cast<unsigned char>(ident[0]))) {
            errors.push_back("Ошибка: идентификатор '" + ident + "' начинается с цифры (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, ident, startLine, startCol);
            return;
        }
        if (keywords.count(ident))
            tokens.emplace_back(TokenType::KEYWORD, ident, startLine, startCol);
        else if (boolConstants.count(ident))
            tokens.emplace_back(TokenType::CONST_BOOL, ident, startLine, startCol);
        else
            tokens.emplace_back(TokenType::IDENTIFIER, ident, startLine, startCol);
    }

    void readNumber() {
        int startLine = line, startCol = col;
        std::string num;
        bool hasDot = false;
        while (pos < source.size() && (std::isdigit(static_cast<unsigned char>(source[pos])) || source[pos] == '.')) {
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
        // Если точка есть, но после неё нет цифр (последний символ - точка)
        if (hasDot && num.back() == '.') {
            errors.push_back("Ошибка: некорректная вещественная константа – после точки ожидаются цифры (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, num, startLine, startCol);
            return;
        }
        if (pos < source.size() && std::isalpha(static_cast<unsigned char>(source[pos]))) {
            errors.push_back("Ошибка: недопустимый символ в числовой константе '" + num + source[pos] + "' (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            while (pos < source.size() && std::isalpha(static_cast<unsigned char>(source[pos]))) {
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
        pos++; col++;
        bool closed = false;
        while (pos < source.size()) {
            char ch = source[pos];
            if (ch == '"') {
                closed = true;
                pos++; col++;
                break;
            }
            if (ch == '\\') {
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
        if (invalidChars.count(ch)) {
            errors.push_back("Ошибка: недопустимый символ '" + std::string(1, ch) + "' (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, std::string(1, ch), startLine, startCol);
            pos++; col++;
            return;
        }
        if (delimiterChars.count(ch)) {
            tokens.emplace_back(TokenType::DELIMITER, std::string(1, ch), startLine, startCol);
            pos++; col++;
            return;
        }
        std::string op;
        while (pos < source.size() && !std::isspace(static_cast<unsigned char>(source[pos])) &&
               !delimiterChars.count(source[pos]) &&
               !std::isalnum(static_cast<unsigned char>(source[pos])) && source[pos] != '_') {
            op += source[pos];
            pos++;
        }
        if (operators.count(op))
            tokens.emplace_back(TokenType::OPERATOR, op, startLine, startCol);
        else {
            errors.push_back("Ошибка: неизвестный оператор '" + op + "' (строка " +
                             std::to_string(startLine) + ", колонка " + std::to_string(startCol) + ")");
            tokens.emplace_back(TokenType::ERROR, op, startLine, startCol);
        }
    }

    void validateIncludeDirective() {
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type == TokenType::OPERATOR && tokens[i].value == "#") {
                if (i + 1 >= tokens.size()) {
                    errors.push_back("Ошибка: после '#' ожидается директива include (конец файла)");
                    continue;
                }
                const Token& next = tokens[i+1];
                if (next.type != TokenType::KEYWORD || next.value != "include") {
                    errors.push_back("Ошибка: после '#' должно следовать ключевое слово 'include', найдено '" +
                                     next.value + "' (строка " + std::to_string(next.line) +
                                     ", колонка " + std::to_string(next.column) + ")");
                }
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    Lexer lexer(source);
    auto tokens = lexer.tokenize();

    lexer.printTokens();
    lexer.printTokenSequence();

    if (lexer.hasErrors()) {
        std::cout << "\nЛексические ошибки:\n";
        lexer.printErrors();
        std::cout << "Лексический анализ завершён с ошибками.\n";
    } else {
        std::cout << "\nЛексический анализ завершён успешно. Обнаружено " << tokens.size() << " токенов. Ошибок не найдено.\n";
    }

    std::string outFilename = std::string("lexer_output_") + std::string(argv[1]).substr(0, 17) + ".txt";
    lexer.saveToFile(outFilename);

    return 0;
}
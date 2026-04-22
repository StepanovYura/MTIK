// preprocessor.cpp – модуль очистки кода
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>
#include <vector>

class Preprocessor {
public:
    // Чтение файла
    static std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Не удалось открыть файл: " + filename);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // Удаление комментариев с проверкой незакрытых
    static std::string removeComments(const std::string& code, bool& error) {
        error = false;
        // Проверка на незакрытый многострочный комментарий
        std::regex openComment("/\\*");
        std::regex closeComment("\\*/");
        auto openBegin = std::sregex_iterator(code.begin(), code.end(), openComment);
        auto openEnd = std::sregex_iterator();
        auto closeBegin = std::sregex_iterator(code.begin(), code.end(), closeComment);
        int openCount = std::distance(openBegin, openEnd);
        int closeCount = std::distance(closeBegin, std::sregex_iterator());
        if (openCount > closeCount) {
            std::cerr << "Ошибка: незакрытый многострочный комментарий" << std::endl;
            error = true;
            return code;
        }

        // Удаление многострочных комментариев
        std::regex multiline("/\\*[\\s\\S]*?\\*/");
        std::string result = std::regex_replace(code, multiline, "");

        // Удаление однострочных комментариев
        std::regex singleline("//[^\\n]*");
        result = std::regex_replace(result, singleline, "");

        return result;
    }

    // Удаление лишних пробелов и пустых строк
    static std::string cleanWhitespace(const std::string& code) {
        std::string result;
        std::istringstream stream(code);
        std::string line;
        std::vector<std::string> lines;

        // Обрабатываем каждую строку
        while (std::getline(stream, line)) {
            // Удаляем начальные и конечные пробелы/табуляции
            std::regex leading("[ \t]+");
            std::regex trailing("[ \t]+$");
            line = std::regex_replace(line, leading, "", std::regex_constants::format_first_only);
            line = std::regex_replace(line, trailing, "", std::regex_constants::format_first_only);

            // Заменяем последовательности пробелов/табуляций на один пробел
            std::regex spaces("[ \t]+");
            line = std::regex_replace(line, spaces, " ");

            // Пропускаем пустые строки
            if (!line.empty()) {
                lines.push_back(line);
            }
        }

        // Собираем результат с переносами строк
        // for (const auto& l : lines) {
        //     result += l + "\n";
        // }
        for (size_t i = 0; i < lines.size(); ++i) {
            result += lines[i] + "\n";
        }

        return result;
    }

    // Дополнительная проверка на недопустимые символы (ASCII 0-8,11-12,14-31)
    static void checkInvalidChars(const std::string& code) {
        std::regex invalid("[\\x00-\\x08\\x0B\\x0C\\x0E-\\x1F]");
        if (std::regex_search(code, invalid)) {
            std::cerr << "Предупреждение: обнаружены недопустимые управляющие символы" << std::endl;
        }
    }

    // Полный цикл обработки
    static std::string process(const std::string& filename) {
        std::string source = readFile(filename);
        bool commentError = false;
        std::string noComments = removeComments(source, commentError);
        if (commentError) {
            return "";
        }
        checkInvalidChars(noComments);
        std::string cleaned = cleanWhitespace(noComments);
        return cleaned;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Использование: " << argv[0] << " <входной_файл>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    try {
        std::string output = Preprocessor::process(filename);
        if (!output.empty()) {
            std::cout << output;
            std::cerr << "Ошибок не выявлено" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
#include "json.h"

using namespace std;

namespace json {

    Number LoadNumber(std::istream& input) {
        using namespace std::literals;

        std::string parsed_num;

        // Считывает в parsed_num очередной символ из input
        auto read_char = [&parsed_num, &input] {
            parsed_num += static_cast<char>(input.get());
            if (!input) {
                throw ParsingError("Failed to read number from stream"s);
            }
        };

        // Считывает одну или более цифр в parsed_num из input
        auto read_digits = [&input, read_char] {
            if (!std::isdigit(input.peek())) {
                throw ParsingError("A digit is expected"s);
            }
            while (std::isdigit(input.peek())) {
                read_char();
            }
        };

        if (input.peek() == '-') {
            read_char();
        }
        // Парсим целую часть числа
        if (input.peek() == '0') {
            read_char();
            // После 0 в JSON не могут идти другие цифры
        }
        else {
            read_digits();
        }

        bool is_int = true;
        // Парсим дробную часть числа
        if (input.peek() == '.') {
            read_char();
            read_digits();
            is_int = false;
        }

        // Парсим экспоненциальную часть числа
        if (int ch = input.peek(); ch == 'e' || ch == 'E') {
            read_char();
            if (ch = input.peek(); ch == '+' || ch == '-') {
                read_char();
            }
            read_digits();
            is_int = false;
        }

        try {
            if (is_int) {
                // Сначала пробуем преобразовать строку в int
                try {
                    return std::stoi(parsed_num);
                }
                catch (...) {
                    // В случае неудачи, например, при переполнении,
                    // код ниже попробует преобразовать строку в double
                }
            }
            return std::stod(parsed_num);
        }
        catch (...) {
            throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
        }
    }

    // Считывает содержимое строкового литерала JSON-документа
    // Функцию следует использовать после считывания открывающего символа ":
    std::string LoadString(std::istream& input) {
        using namespace std::literals;

        auto it = std::istreambuf_iterator<char>(input);
        auto end = std::istreambuf_iterator<char>();
        std::string s;
        while (true) {
            if (it == end) {
                // Поток закончился до того, как встретили закрывающую кавычку?
                throw ParsingError("String parsing error");
            }
            const char ch = *it;
            if (ch == '"') {
                // Встретили закрывающую кавычку
                ++it;
                break;
            }
            else if (ch == '\\') {
                // Встретили начало escape-последовательности
                ++it;
                if (it == end) {
                    // Поток завершился сразу после символа обратной косой черты
                    throw ParsingError("String parsing error");
                }
                const char escaped_char = *(it);
                // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                }
            }
            else if (ch == '\n' || ch == '\r') {
                // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                throw ParsingError("Unexpected end of line"s);
            }
            else {
                // Просто считываем очередной символ и помещаем его в результирующую строку
                s.push_back(ch);
            }
            ++it;
        }

        return s;
    }

    namespace {

        Node LoadNode(istream& input);

        Node LoadArray(istream& input) {
            Array result;

            char c;
            for (; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            if (c != ']') {
                throw ParsingError("Unexpected end of array"s);
            }

            return Node(move(result));
        }

        /*
        Node LoadInt(istream& input) {
            int result = 0;
            while (isdigit(input.peek())) {
                result *= 10;
                result += input.get() - '0';
            }
            return Node(result);
        }

        Node LoadString(istream& input) {
            string line;
            getline(input, line, '"');
            return Node(move(line));
        }
        */
        Node LoadDict(istream& input) {
            Dict result;
            char c;
            for (; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                string key = LoadString(input);
                input >> c;
                result.insert({ move(key), LoadNode(input) });
            }

            if (c != '}') {
                throw ParsingError("Unexpected end of dictionary"s);
            }

            return Node(move(result));
        }

        Node LoadBool(istream& input) {
            std::string s;
            int char_amount = 0;
            if (input.peek() == 't') char_amount = 4;
            if (input.peek() == 'f') char_amount = 5;

            char c;
            for (int i = 0; i < char_amount && input >> c; ++i) {
                s.push_back(c);
            }

            if (s == "true") return Node{ true };
            if (s == "false") return Node{ false };

            throw ParsingError("Boolean parsing error"s);

            return Node{};
        }

        Node LoadNull(istream& input) {
            std::string s;

            char c;
            for (int i = 0; i < 4 && input >> c; ++i) {
                s.push_back(c);
            }

            if (s == "null") return Node{};

            throw ParsingError("null parsing error"s);

            return Node{};
        }


        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == ']') {
                throw ParsingError("Unexpected end of array"s);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '}') {
                throw ParsingError("Unexpected end of dictionary"s);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if (std::isdigit(c) || c == '-') { // interpret as null
                input.putback(c);

                const Number num = LoadNumber(input);
                const auto int_ptr = std::get_if<int>(&num);
                if (int_ptr) {
                    return Node{ *int_ptr };
                }
                else {
                    return std::get<double>(num);
                }
            }
            else if (c == 't' || c == 'f') {
                input.putback(c);
                return LoadBool(input);
            }
            else {
                input.putback(c);
                return LoadNull(input);
            }
        }

    }  // namespace

    // ---Constructs:

    

    // ---Get-functions:
    int Node::AsInt() const {
        try {
            return get<int>(*this);
        }
        catch (const bad_variant_access&) {
            throw std::logic_error("wrong type");
        }
    }

    bool Node::AsBool() const {
        try {
            return get<bool>(*this);
        }
        catch (const bad_variant_access&) {
            throw std::logic_error("wrong type");
        }
    }

    // Возвращает значение типа double, если внутри хранится double либо int.
    // В последнем случае возвращается приведённое в double значение.
    double Node::AsDouble() const {
        try {
            const auto* double_val = std::get_if<double>(&*this);
            if (double_val) return *double_val;
            return get<int>(*this);
        }
        catch (const bad_variant_access&) {
            throw std::logic_error("wrong type");
        }
    }

    const std::string& Node::AsString() const {
        try {
            return get<std::string>(*this);
        }
        catch (const bad_variant_access&) {
            throw std::logic_error("wrong type");
        }
    }

    const Array& Node::AsArray() const {
        try {
            return get<Array>(*this);
        }
        catch (const bad_variant_access&) {
            throw std::logic_error("wrong type");
        }
    }

    const Dict& Node::AsMap() const {
        try {
            return get<Dict>(*this);
        }
        catch (const bad_variant_access&) {
            throw std::logic_error("wrong type");
        }
    }

    Document::Document(Node root)
        : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input) };
    }

    // ---Print functions:
    // Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
    struct PrintContext {
        std::ostream& out;
        int indent_step = 4;
        int indent = 0;

        void PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        // Возвращает новый контекст вывода с увеличенным смещением
        PrintContext Indented() const {
            return { out, indent_step, indent_step + indent };
        }
    };
    //void PrintValue(const Dict& value, std::ostream& out);
    // Шаблон, подходящий для вывода double и int
    // binary '<<': no operator found which takes a right - hand operand of type 'const Value' (or there is no acceptable conversion)
    template <typename Value>
    void PrintValue(const Value& value, const PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << value;
    }

    // Перегрузка функции PrintValue для вывода значений null
    void PrintValue(std::nullptr_t, const PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << "null"sv;
    }

    void PrintValue(bool value, const PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << std::boolalpha << value;
    }

    void PrintValue(const std::string& value, const PrintContext& ctx) {

        ctx.PrintIndent();
        ctx.out << "\"";

        auto it = value.begin();
        auto end = value.end();

        while (true) {

            if (it == end) break;

            const char ch = *it;
            if (ch == '\n' || /*ch == '\t' ||*/ ch == '\r' || ch == '"' || ch == '\\') {
                // Встретили символ для экранирования
                ctx.out << '\\';

                // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                switch (ch) {
                case '\n':
                    ctx.out << 'n';
                    break;
                case '\t':
                    ctx.out << 't';
                    break;
                case '\r':
                    ctx.out << 'r';
                    break;
                case '"':
                    ctx.out << '"';
                    break;
                case '\\':
                    ctx.out << '\\';
                    break;
                }
            }
            else {
                ctx.out << ch;
            }
            ++it;
        }
        ctx.out << "\"";
    }

    void PrintValue(const Array& value, const PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << "["sv;
        bool first = true;
        for (const auto& element : value) {
            if (!first) {
                ctx.out << ", "sv;
            }
            first = false;
            ctx.out << '\n';
            std::visit(
                [&ctx](const auto& value) { PrintValue(value, ctx.Indented()); },
                element.GetValue());
        }
        ctx.out << '\n';
        ctx.PrintIndent();
        ctx.out << "]"sv;
    }

    void PrintValue(const Dict& value, const PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << "{"sv;
        bool first = true;
        for (const auto& pair : value) {
            if (!first) {
                ctx.out << ", "sv;
            }
            first = false;
            ctx.out << '\n';
            PrintValue(pair.first, ctx.Indented());
            ctx.out << ": ";

            if (pair.second.IsArray()) {
                std::visit(
                    [&ctx](const auto& value) { PrintValue(value, ctx.Indented()); },
                    pair.second.GetValue());
            }
            else {
                std::visit(
                    [&ctx](const auto& value) { PrintValue(value, { ctx.out }); },
                    pair.second.GetValue());
            }

        }
        ctx.out << '\n';
        ctx.PrintIndent();
        ctx.out << "}"sv;
    }

    std::ostream& operator<<(std::ostream& out, const Dict& dict) {
        PrintValue(dict, PrintContext{ out });
        return out;
    }

    void PrintNode(const Node& node, std::ostream& out) {
        PrintContext ctx{ out };
        std::visit(
            [&ctx](const auto& value) { PrintValue(value, ctx); },
            node.GetValue());
    }

    void Print(const Document& doc, std::ostream& output) {

        const auto& root = doc.GetRoot();
        PrintNode(root, output);

    }


    // ---Checker functions:

    bool Node::IsInt() const {
        return holds_alternative<int>(*this);
    }
    bool Node::IsDouble() const { // Возвращает true, если в Node хранится int либо double.
        return holds_alternative<int>(*this) || holds_alternative<double>(*this);
    }
    bool Node::IsPureDouble() const { // Возвращает true, если в Node хранится double.
        return holds_alternative<double>(*this);
    }
    bool Node::IsBool() const {
        return holds_alternative<bool>(*this);
    }
    bool Node::IsString() const {
        return holds_alternative<std::string>(*this);
    }
    bool Node::IsNull() const {
        return holds_alternative<std::nullptr_t>(*this);
    }
    bool Node::IsArray() const {
        return holds_alternative<Array>(*this);
    }
    bool Node::IsMap() const {
        return holds_alternative<Dict>(*this);
    }


}  // namespace json
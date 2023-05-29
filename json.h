#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

    class Node;
    // Сохраните объявления Dict и Array без изменения
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    using Number = std::variant<int, double>;

    using Value = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;
    class Node final : private Value {
    public:

        // Делаем доступными все конструкторы родительского класса variant
        using variant::variant;

        const Value& GetValue() const { return *this; }

        bool IsInt() const;
        bool IsDouble() const; // Возвращает true, если в Node хранится int либо double.
        bool IsPureDouble() const; // Возвращает true, если в Node хранится double.
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        bool AsBool() const;
        double AsDouble() const; // Возвращает значение типа double, если внутри хранится double либо int.В последнем случае возвращается приведённое в double значение.
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        bool operator==(const Node& other) const {
            return *this == other;
        }

        bool operator!=(const Node& other) const {
            return !(*this == other);
        }


    private:
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;
        bool operator==(const Document& other) const {
            return root_ == other.root_;
        }
        bool operator!=(const Document& other) const {
            return root_ != other.root_;
        }

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

}  // namespace json
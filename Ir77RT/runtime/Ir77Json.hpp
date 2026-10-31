#pragma once

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../interface/IIr77Json.hpp"

namespace NSIr77RT {
// A JSON value built in code and written once -- replies, dumps, messages. Reading stays simdjson's.
//   json["key"] = value;  json["list"].Push(value);  json["raw"] = Ir77Json::Raw(text);  json.Write()
// Objects keep their keys in the order first set. Floats are written in the shortest form that reads back the same; not finite -> null.
class Ir77Json : public IIr77Json {
   public:
    Ir77Json() {}

    Ir77Json(bool const value) {
        m_kind = Ir77JsonKind::Bool;
        m_bool = value;
    }

    template <typename T>
        requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>)
    Ir77Json(T const value) {
        m_kind = Ir77JsonKind::Number;
        m_text = ToNumber(value);
    }

    Ir77Json(std::string const& value) {
        m_kind = Ir77JsonKind::String;
        m_text = value;
    }

    Ir77Json(std::string_view const value) {
        m_kind = Ir77JsonKind::String;
        m_text = std::string{value};
    }

    Ir77Json(char const* const value) {
        m_kind = Ir77JsonKind::String;
        m_text = value ? value : "";
    }

    Ir77Json(Ir77Json const& json) = default;

    Ir77Json(Ir77Json&& json) = default;

   public:
    Ir77Json& operator=(Ir77Json const& json) = default;

    Ir77Json& operator=(Ir77Json&& json) = default;

    // an object's member -- made (null) when it isn't there; a null value becomes an object first
    Ir77Json& operator[](std::string_view const& key) {
        if (m_kind == Ir77JsonKind::Null) m_kind = Ir77JsonKind::Object;

        for (std::size_t i = 0; i < m_keys.size(); i++)
            if (m_keys[i] == key) return m_items[i];

        m_keys.emplace_back(key);
        m_items.emplace_back();
        return m_items.back();
    }

   public:
    static Ir77Json Object() {
        Ir77Json json{};
        json.m_kind = Ir77JsonKind::Object;
        return json;
    }

    static Ir77Json Array() {
        Ir77Json json{};
        json.m_kind = Ir77JsonKind::Array;
        return json;
    }

    // text that's already JSON -- written as it is, never checked
    static Ir77Json Raw(std::string const& text) {
        Ir77Json json{};
        json.m_kind = Ir77JsonKind::Raw;
        json.m_text = text.empty() ? "null" : text;
        return json;
    }

    // an array of numbers -- vectors, quaternions, colors
    template <typename T>
    static Ir77Json Numbers(std::initializer_list<T> const values) {
        Ir77Json json = Array();
        for (T const value : values) json.m_items.emplace_back(value);
        return json;
    }

    template <typename T>
    static Ir77Json Numbers(T const* const values, std::size_t const count) {
        Ir77Json json = Array();
        for (std::size_t i = 0; i < count; i++) json.m_items.emplace_back(values[i]);
        return json;
    }

    // an object's member, or nullptr
    Ir77Json const* Find(std::string_view const& key) const {
        if (m_kind != Ir77JsonKind::Object) return nullptr;

        for (std::size_t i = 0; i < m_keys.size(); i++)
            if (m_keys[i] == key) return &m_items[i];

        return nullptr;
    }

    bool Has(std::string_view const& key) const { return Find(key) != nullptr; }

    void Remove(std::string_view const& key) {
        if (m_kind != Ir77JsonKind::Object) return;

        for (std::size_t i = 0; i < m_keys.size(); i++) {
            if (m_keys[i] != key) continue;

            m_keys.erase(m_keys.begin() + static_cast<std::ptrdiff_t>(i));
            m_items.erase(m_items.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
    }

    // an array's next element -- a null value becomes an array first
    Ir77Json& Push(IIr77Json const& value) { return Push(static_cast<Ir77Json const&>(value)); }

    Ir77Json& Push(Ir77Json const& value) {
        if (m_kind == Ir77JsonKind::Null) m_kind = Ir77JsonKind::Array;

        m_items.push_back(value);
        return m_items.back();
    }

    // every member of other (an object) set on this one -- a key both have takes other's value whole
    void Merge(IIr77Json const& other) {
        Ir77Json const& from = static_cast<Ir77Json const&>(other);
        if (from.m_kind != Ir77JsonKind::Object) return;
        if (m_kind != Ir77JsonKind::Object) *this = Object();

        for (std::size_t i = 0; i < from.m_keys.size(); i++) (*this)[from.m_keys[i]] = from.m_items[i];
    }

    Ir77JsonKind Kind() const { return m_kind; }

    bool IsNull() const { return m_kind == Ir77JsonKind::Null; }

    // members (object) or elements (array); 0 otherwise
    std::size_t Size() const { return m_items.size(); }

    void Write(std::string& out) const {
        switch (m_kind) {
            case Ir77JsonKind::Null:
                out += "null";
                break;
            case Ir77JsonKind::Bool:
                out += m_bool ? "true" : "false";
                break;
            case Ir77JsonKind::Number:
            case Ir77JsonKind::Raw:
                out += m_text;
                break;
            case Ir77JsonKind::String:
                out += Quote(m_text);
                break;
            case Ir77JsonKind::Array:
                out += '[';
                for (std::size_t i = 0; i < m_items.size(); i++) {
                    if (i) out += ',';
                    m_items[i].Write(out);
                }
                out += ']';
                break;
            case Ir77JsonKind::Object:
                out += '{';
                for (std::size_t i = 0; i < m_items.size(); i++) {
                    if (i) out += ',';
                    out += Quote(m_keys[i]);
                    out += ':';
                    m_items[i].Write(out);
                }
                out += '}';
                break;
        }
    }

    std::string Write() const {
        std::string out{};
        Write(out);
        return out;
    }

    // a JSON string, quotes included -- quotes, backslashes and control characters escaped
    static std::string Quote(std::string_view const text) {
        std::string out{};
        out.reserve(text.size() + 2);
        out += '"';

        for (char const c : text) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char escaped[8]{};
                        std::snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                        out += escaped;
                    } else {
                        out += c;
                    }
            }
        }

        out += '"';
        return out;
    }

   private:
    template <typename T>
    static std::string ToNumber(T const value) {
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(value)) return "null";
        }

        char text[64]{};
        auto const [end, error] = std::to_chars(text, text + sizeof(text), value);
        return error == std::errc{} ? std::string{text, end} : std::string{"null"};
    }

   private:
    Ir77JsonKind m_kind{Ir77JsonKind::Null};

    bool m_bool{false};

    std::string m_text{};  // a number's text, a string's characters, raw JSON

    std::vector<std::string> m_keys{};  // an object's keys -- parallel to m_items

    std::vector<Ir77Json> m_items{};  // an array's elements, an object's values
};
}  // namespace NSIr77RT

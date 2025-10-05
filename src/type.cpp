#include "type.hpp"

#include <cctype>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace curious::dsl::capnpgen
{

Type::Type() = default;

Type::Type(std::string_view line)
{
    *this = from_line(line);
}

Type::Type(const Type& other)
{
    CopyFrom(other);
}

Type& Type::operator=(const Type& other)
{
    if (this != &other)
    {
        CopyFrom(other);
    }
    return *this;
}

// ---- observers ----
Type::Kind Type::kind() const noexcept
{
    return _kind;
}

bool Type::is_primitive() const noexcept
{
    return _kind == Kind::Primitive;
}

bool Type::is_custom() const noexcept
{
    return _kind == Kind::Custom;
}

bool Type::is_enum() const noexcept
{
    return _kind == Kind::Enum;
}

bool Type::is_list() const noexcept
{
    return _kind == Kind::List;
}

bool Type::is_map() const noexcept
{
    return _kind == Kind::Map;
}

const std::string& Type::field_name() const noexcept
{
    return _fieldName;
}

const std::string& Type::custom_name() const noexcept
{
    return _customName;
}

const std::vector<std::string>& Type::enum_values() const noexcept
{
    return _enumValues;
}

const Type* Type::element() const noexcept
{
    return _elem.get();
}

const Type* Type::key() const noexcept
{
    return _key.get();
}

const Type* Type::value() const noexcept
{
    return _value.get();
}

std::string Type::cpp_type() const
{
    switch (_kind)
    {
        case Kind::Primitive: return dsl_to_cpp_map.at(_primitive);
        case Kind::Custom:
        case Kind::Enum:      return _customName;
        case Kind::List:      return "std::vector<" + _elem->cpp_type() + ">";
        case Kind::Map:       return "std::unordered_map<" + _key->cpp_type() + "," + _value->cpp_type() + ">";
    }
    return {};
}

std::string Type::capnp_type() const
{
    switch (_kind)
    {
        case Kind::Primitive: return dsl_to_capnp_map.at(_primitive);
        case Kind::Custom:
        case Kind::Enum:      return _customName;
        case Kind::List:      return "List(" + _elem->capnp_type() + ")";
        case Kind::Map:       return "Map(" + _key->capnp_type() + "," + _value->capnp_type() + ")";
    }
    return {};
}

// ------- parsing one field line -------
Type Type::from_line(std::string_view line)
{
    struct Parser
    {
        std::string_view s;
        std::size_t i { 0 };

        void ws()
        {
            while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0)
            {
                ++i;
            }
        }

        bool eat(char c)
        {
            ws();
            if (i < s.size() && s[i] == c)
            {
                ++i;
                return true;
            }
            return false;
        }

        void must(char c)
        {
            if (!eat(c))
            {
                throw std::runtime_error(std::string("expected '") + c + "'");
            }
        }

        std::string ident()
        {
            ws();
            std::size_t j = i;
            while (j < s.size())
            {
                char c = s[j];
                if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == ':')
                {
                    ++j;
                }
                else
                {
                    break;
                }
            }
            if (j == i)
            {
                throw std::runtime_error("expected identifier");
            }
            std::string out(s.substr(i, j - i));
            i = j;
            ws();
            return out;
        }

        static std::string lower(std::string x)
        {
            for (char& c : x)
            {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return x;
        }

        static bool is_list_kw(const std::string& low)
        {
            return low == "list" || low == "vector" || low == "std::vector";
        }

        static bool is_map_kw(const std::string& low)
        {
            return low == "map" || low == "unordered_map" || low == "std::map" || low == "std::unordered_map";
        }

        static bool resolve_primitive(const std::string& name, DslType& out)
        {
            if (auto it = string_to_dsl_map.find(name); it != string_to_dsl_map.end())
            {
                out = it->second;
                return true;
            }

            std::string low;
            low.reserve(name.size());
            for (char c : name)
            {
                low.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            }
            if (auto it2 = string_to_dsl_map.find(low); it2 != string_to_dsl_map.end())
            {
                out = it2->second;
                return true;
            }
            return false;
        }

        Type parse_type()
        {
            std::string id = ident();
            std::string low = lower(id);

            if (is_list_kw(low))
            {
                must('<');
                Type t;
                t._kind = Type::Kind::List;
                t._elem = std::make_unique<Type>(parse_type());
                must('>');
                ws();
                return t;
            }

            if (is_map_kw(low))
            {
                must('<');
                Type t;
                t._kind = Type::Kind::Map;
                t._key = std::make_unique<Type>(parse_type());
                must(',');
                t._value = std::make_unique<Type>(parse_type());
                must('>');
                ws();
                return t;
            }

            Type t;
            DslType prim {};
            if (resolve_primitive(id, prim))
            {
                t._kind = Type::Kind::Primitive;
                t._primitive = prim;
            }
            else
            {
                t._kind = Type::Kind::Custom;
                t._customName = id;
            }
            ws();
            return t;
        }

        std::string parse_field_name()
        {
            return ident();
        }
    };

    Parser p { line, 0 };
    p.ws();

    Type T = p.parse_type();
    T._fieldName = p.parse_field_name();

    p.ws();
    if (p.i < p.s.size() && p.s[p.i] == ';')
    {
        ++p.i;
    }

    return T;
}

// ---- private ----
void Type::CopyFrom(const Type& other)
{
    _kind = other._kind;
    _primitive = other._primitive;
    _customName = other._customName;
    _enumValues = other._enumValues;
    _fieldName = other._fieldName;

    _elem.reset(other._elem ? new Type(*other._elem) : nullptr);
    _key.reset(other._key ? new Type(*other._key) : nullptr);
    _value.reset(other._value ? new Type(*other._value) : nullptr);
}

} // namespace curious::dsl::capnpgen

#include "type.hpp"

#include <cctype>
#include <stdexcept>

namespace curious::dsl::capnpgen
{

/// @brief Helper class to parse type declarations from DSL syntax.
class Type::TypeParser
{
public:
    /// @brief Construct a parser for the given line.
    /// @param line The DSL line to parse.
    explicit TypeParser(std::string_view line)
        : _source(line)
    {
    }

    /// @brief Parse the complete type and field name.
    /// @return A fully constructed Type object.
    Type parse()
    {
        _skip_whitespace();
        Type result = _parse_type();
        result._fieldName = _parse_field_name();
        _skip_whitespace();

        // Optional trailing semicolon
        if (_position < _source.size() && _source[_position] == ';')
        {
            ++_position;
        }

        return result;
    }

private:
    std::string_view _source;
    std::size_t _position{0};

    /// @brief Skip whitespace at current position.
    void _skip_whitespace()
    {
        while (_position < _source.size() &&
               std::isspace(static_cast<unsigned char>(_source[_position])))
        {
            ++_position;
        }
    }

    /// @brief Consume a specific character, throwing if not found.
    /// @param expected The character that must be at the current position.
    void _expect_char(char expected)
    {
        _skip_whitespace();
        if (_position >= _source.size() || _source[_position] != expected)
        {
            throw std::runtime_error(std::string("Expected '") + expected + "'");
        }
        ++_position;
    }

    /// @brief Try to consume a specific character.
    /// @param c The character to try consuming.
    /// @return True if the character was consumed.
    bool _try_consume(char c)
    {
        _skip_whitespace();
        if (_position < _source.size() && _source[_position] == c)
        {
            ++_position;
            return true;
        }
        return false;
    }

    /// @brief Read an identifier at the current position.
    /// @return The identifier string.
    std::string _read_identifier()
    {
        _skip_whitespace();
        std::size_t start = _position;

        while (_position < _source.size())
        {
            char c = _source[_position];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':')
            {
                ++_position;
            }
            else
            {
                break;
            }
        }

        if (_position == start)
        {
            throw std::runtime_error("Expected identifier");
        }

        std::string identifier(_source.substr(start, _position - start));
        _skip_whitespace();
        return identifier;
    }

    /// @brief Convert a string to lowercase.
    /// @param str The string to convert.
    /// @return A lowercase version of the string.
    static std::string _to_lower(const std::string& str)
    {
        std::string result;
        result.reserve(str.size());
        for (char c : str)
        {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return result;
    }

    /// @brief Check if an identifier is a list keyword.
    /// @param identifier_lower The lowercase identifier.
    /// @return True if it represents a list type.
    static bool _is_list_keyword(const std::string& identifier_lower)
    {
        return identifier_lower == "list" ||
               identifier_lower == "vector" ||
               identifier_lower == "std::vector";
    }

    /// @brief Check if an identifier is a map keyword.
    /// @param identifier_lower The lowercase identifier.
    /// @return True if it represents a map type.
    static bool _is_map_keyword(const std::string& identifier_lower)
    {
        return identifier_lower == "map" ||
               identifier_lower == "unordered_map" ||
               identifier_lower == "std::map" ||
               identifier_lower == "std::unordered_map";
    }

    /// @brief Try to resolve an identifier as a primitive type.
    /// @param identifier The identifier to check.
    /// @param out_type Output parameter for the resolved DslType.
    /// @return True if resolved as a primitive.
    static bool _try_resolve_primitive(const std::string& identifier, DslType& out_type)
    {
        // Try exact match first
        auto it = string_to_dsl_map.find(identifier);
        if (it != string_to_dsl_map.end())
        {
            out_type = it->second;
            return true;
        }

        // Try case-insensitive match
        std::string lower = _to_lower(identifier);
        auto it2 = string_to_dsl_map.find(lower);
        if (it2 != string_to_dsl_map.end())
        {
            out_type = it2->second;
            return true;
        }

        return false;
    }

    /// @brief Parse a type (possibly nested).
    /// @return A Type object.
    Type _parse_type()
    {
        std::string identifier = _read_identifier();
        std::string identifier_lower = _to_lower(identifier);

        // Handle list types
        if (_is_list_keyword(identifier_lower))
        {
            _expect_char('<');
            Type list_type;
            list_type._kind = Type::Kind::List;
            list_type._elementType = std::make_unique<Type>(_parse_type());
            _expect_char('>');
            return list_type;
        }

        // Handle map types
        if (_is_map_keyword(identifier_lower))
        {
            _expect_char('<');
            Type map_type;
            map_type._kind = Type::Kind::Map;
            map_type._keyType = std::make_unique<Type>(_parse_type());
            _expect_char(',');
            map_type._valueType = std::make_unique<Type>(_parse_type());
            _expect_char('>');
            return map_type;
        }

        // Try to resolve as primitive
        Type result;
        DslType primitive_type;
        if (_try_resolve_primitive(identifier, primitive_type))
        {
            result._kind = Type::Kind::Primitive;
            result._primitiveType = primitive_type;
        }
        else
        {
            // Must be custom type
            result._kind = Type::Kind::Custom;
            result._customName = identifier;
        }

        return result;
    }

    /// @brief Parse the field name.
    /// @return The field name string.
    std::string _parse_field_name()
    {
        return _read_identifier();
    }
};

// ---- Type constructors and assignment ----

Type::Type() = default;

Type::Type(std::string_view line)
{
    *this = parse_from_line(line);
}

Type::Type(const Type& other)
{
    _copy_from(other);
}

Type& Type::operator=(const Type& other)
{
    if (this != &other)
    {
        _copy_from(other);
    }
    return *this;
}

// ---- Type observers ----

Type::Kind Type::get_kind() const noexcept
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

const std::string& Type::get_field_name() const noexcept
{
    return _fieldName;
}

const std::string& Type::get_custom_name() const noexcept
{
    return _customName;
}

const std::vector<std::string>& Type::get_enum_values() const noexcept
{
    return _enumValues;
}

const Type* Type::get_element_type() const noexcept
{
    return _elementType.get();
}

const Type* Type::get_key_type() const noexcept
{
    return _keyType.get();
}

const Type* Type::get_value_type() const noexcept
{
    return _valueType.get();
}

// ---- Type conversion methods ----

std::string Type::get_cpp_type() const
{
    switch (_kind)
    {
        case Kind::Primitive:
            return dsl_to_cpp_map.at(_primitiveType);

        case Kind::Custom:
        case Kind::Enum:
            return _customName;

        case Kind::List:
            return "std::vector<" + _elementType->get_cpp_type() + ">";

        case Kind::Map:
            return "std::unordered_map<" + _keyType->get_cpp_type() + ", " +
                   _valueType->get_cpp_type() + ">";
    }

    return {};
}

std::string Type::get_capnp_type() const
{
    switch (_kind)
    {
        case Kind::Primitive:
            return dsl_to_capnp_map.at(_primitiveType);

        case Kind::Custom:
        case Kind::Enum:
            return _customName;

        case Kind::List:
            return "List(" + _elementType->get_capnp_type() + ")";

        case Kind::Map:
            return "Map(" + _keyType->get_capnp_type() + ", " +
                   _valueType->get_capnp_type() + ")";
    }

    return {};
}

// ---- Static parsing method ----

Type Type::parse_from_line(std::string_view line)
{
    TypeParser parser(line);
    return parser.parse();
}

// ---- Private helper methods ----

void Type::_copy_from(const Type& other)
{
    _kind = other._kind;
    _primitiveType = other._primitiveType;
    _customName = other._customName;
    _enumValues = other._enumValues;
    _fieldName = other._fieldName;

    // Deep copy unique_ptr members
    _elementType.reset(other._elementType ? new Type(*other._elementType) : nullptr);
    _keyType.reset(other._keyType ? new Type(*other._keyType) : nullptr);
    _valueType.reset(other._valueType ? new Type(*other._valueType) : nullptr);
}

} // namespace curious::dsl::capnpgen

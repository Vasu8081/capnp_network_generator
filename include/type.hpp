#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "mappings.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Represents a parsed DSL type (primitive, custom, enum, list, or map).
/// @details Provides conversion utilities to C++ and Cap'n Proto type names.
class Type
{
    // Forward declaration for friend class
    class TypeParser;
    friend class TypeParser;

public:
    /// @brief The category of DSL type.
    enum class Kind
    {
        Primitive, ///< Built-in primitive type (e.g., int, bool).
        Custom,    ///< User-defined/custom struct type.
        Enum,      ///< Enumeration type.
        List,      ///< Sequence type (e.g., vector).
        Map        ///< Key-value type (e.g., unordered_map).
    };

    /// @brief Default constructor (creates a primitive placeholder).
    Type();

    /// @brief Construct by parsing a single DSL line.
    /// @param line A DSL field declaration (e.g., "vector<int> numbers;").
    explicit Type(std::string_view line);

    /// @brief Copy constructor.
    Type(const Type& other);

    /// @brief Copy assignment operator.
    Type& operator=(const Type& other);

    /// @brief Move constructor.
    Type(Type&&) noexcept = default;

    /// @brief Move assignment operator.
    Type& operator=(Type&&) noexcept = default;

    /// @brief Destructor.
    ~Type() = default;

    /// @brief Get the kind of this type.
    /// @return The type category.
    Kind get_kind() const noexcept;

    /// @brief Check if this is a primitive type.
    /// @return True if the kind is Primitive.
    bool is_primitive() const noexcept;

    /// @brief Check if this is a custom type.
    /// @return True if the kind is Custom.
    bool is_custom() const noexcept;

    /// @brief Check if this is an enum type.
    /// @return True if the kind is Enum.
    bool is_enum() const noexcept;

    /// @brief Check if this is a list type.
    /// @return True if the kind is List.
    bool is_list() const noexcept;

    /// @brief Check if this is a map type.
    /// @return True if the kind is Map.
    bool is_map() const noexcept;

    /// @brief Get the name of the parsed field.
    /// @return The field name.
    const std::string& get_field_name() const noexcept;

    /// @brief Get the name of the custom/enum type (if applicable).
    /// @return The custom type name.
    const std::string& get_custom_name() const noexcept;

    /// @brief Get the enum values (only populated if this represents an enum).
    /// @return A vector of enum value names.
    const std::vector<std::string>& get_enum_values() const noexcept;

    /// @brief Get the element type (valid only if kind==List).
    /// @return Pointer to the element type, or nullptr if not a list.
    const Type* get_element_type() const noexcept;

    /// @brief Get the map key type (valid only if kind==Map).
    /// @return Pointer to the key type, or nullptr if not a map.
    const Type* get_key_type() const noexcept;

    /// @brief Get the map value type (valid only if kind==Map).
    /// @return Pointer to the value type, or nullptr if not a map.
    const Type* get_value_type() const noexcept;

    /// @brief Get the corresponding C++ type string (e.g., std::vector<int>).
    /// @return The C++ type representation.
    std::string get_cpp_type() const;

    /// @brief Get the corresponding Cap'n Proto type string.
    /// @return The Cap'n Proto type representation.
    std::string get_capnp_type() const;

    /// @brief Parse a DSL type+field declaration from a single line.
    /// @param line Example: "vector<int> numbers;"
    /// @return A constructed Type object.
    static Type parse_from_line(std::string_view line);

private:
    /// @brief The kind of type.
    Kind _kind{Kind::Primitive};

    /// @brief Primitive mapping (if kind==Primitive).
    DslType _primitiveType{DslType::Custom};

    /// @brief Custom or enum type name.
    std::string _customName;

    /// @brief Enum values (if kind==Enum).
    std::vector<std::string> _enumValues;

    /// @brief Element type (if kind==List).
    std::unique_ptr<Type> _elementType;

    /// @brief Key type (if kind==Map).
    std::unique_ptr<Type> _keyType;

    /// @brief Value type (if kind==Map).
    std::unique_ptr<Type> _valueType;

    /// @brief Field name in DSL struct.
    std::string _fieldName;

    /// @brief Deep copy helper used by copy constructor and assignment.
    /// @param other The type to copy from.
    void _copy_from(const Type& other);
};

} // namespace curious::dsl::capnpgen

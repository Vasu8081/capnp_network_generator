#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

#include "mappings.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Represents a parsed DSL type (primitive, custom, enum, list, or map).
/// Provides conversion utilities to C++ and Cap’n Proto type names.
class Type final
{
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
    explicit Type(std::string_view line);

    /// @name Copy / Move Semantics
    /// @{
    Type(const Type& other);
    Type& operator=(const Type& other);
    Type(Type&&) noexcept = default;
    Type& operator=(Type&&) noexcept = default;
    /// @}

    /// @name Observers
    /// @{
    Kind kind() const noexcept;
    bool is_primitive() const noexcept;
    bool is_custom() const noexcept;
    bool is_enum() const noexcept;
    bool is_list() const noexcept;
    bool is_map() const noexcept;

    /// @brief Name of the parsed field.
    const std::string& field_name() const noexcept;

    /// @brief Name of the custom/enum type (if applicable).
    const std::string& custom_name() const noexcept;

    /// @brief Enum values (only populated if this represents an enum).
    const std::vector<std::string>& enum_values() const noexcept;

    /// @brief Element type (valid only if kind()==List).
    const Type* element() const noexcept;

    /// @brief Map key type (valid only if kind()==Map).
    const Type* key() const noexcept;

    /// @brief Map value type (valid only if kind()==Map).
    const Type* value() const noexcept;
    /// @}

    /// @brief Get the corresponding C++ type string (e.g., `std::vector<int>`).
    std::string cpp_type() const;

    /// @brief Get the corresponding Cap’n Proto type string.
    std::string capnp_type() const;

    /// @brief Apply visitor-style pattern matching.
    /// @tparam FPrim Function for primitive
    /// @tparam FCustom Function for custom
    /// @tparam FEnum Function for enum
    /// @tparam FList Function for list
    /// @tparam FMap Function for map
    template <class FPrim, class FCustom, class FEnum, class FList, class FMap>
    decltype(auto) match(FPrim fprim, FCustom fcustom, FEnum fenum, FList flist, FMap fmap) const
    {
        switch (_kind)
        {
            case Kind::Primitive: return fprim(_primitive);
            case Kind::Custom:    return fcustom(_customName);
            case Kind::Enum:      return fenum(_customName, _enumValues);
            case Kind::List:      return flist(*_elem);
            case Kind::Map:       return fmap(*_key, *_value);
        }
        throw std::logic_error("unknown Type::Kind");
    }

    /// @brief Parse a DSL type+field declaration from a single line.
    /// @param line Example: `"vector<int> numbers;"`
    /// @return A constructed Type object.
    static Type from_line(std::string_view line);

private:
    // ---- Private Data Members ----
    Kind _kind { Kind::Primitive };           ///< Kind of type.
    DslType  _primitive { DslType::Custom };           ///< Primitive mapping (if kind==Primitive).
    std::string _customName;                  ///< Custom or enum type name.
    std::vector<std::string> _enumValues;     ///< Enum values (if kind==Enum).
    std::unique_ptr<Type> _elem;              ///< Element type (if kind==List).
    std::unique_ptr<Type> _key;               ///< Key type (if kind==Map).
    std::unique_ptr<Type> _value;             ///< Value type (if kind==Map).
    std::string _fieldName;                   ///< Field name in DSL struct.

    // ---- Private Helpers ----
    /// @brief Deep copy helper used by copy constructor and assignment.
    void CopyFrom(const Type& other);
};

} // namespace curious::dsl::capnpgen

#pragma once

#include "type.hpp"
#include <string>
#include <sstream>
#include <set>

namespace curious::dsl::capnpgen
{

/// @brief Helper class to generate C++ conversion code for different types.
class TypeConverter
{
public:
    /// @brief Generate C++ code to convert from Cap'n Proto to C++ for a field.
    /// @param field The field type to convert.
    /// @param reader_expr The Cap'n Proto reader expression (e.g., "reader.getFieldName()").
    /// @param target_var The C++ variable to assign to (e.g., "field_name").
    /// @param indent The indentation level.
    /// @param known_enums Optional set of known enum type names for proper handling.
    /// @return Generated C++ code as a string.
    static std::string generate_from_capnp_code(const Type& field,
                                                  const std::string& reader_expr,
                                                  const std::string& target_var,
                                                  int indent = 1,
                                                  const std::set<std::string>& known_enums = {});

    /// @brief Generate C++ code to convert from C++ to Cap'n Proto for a field.
    /// @param field The field type to convert.
    /// @param builder_expr The Cap'n Proto builder expression (e.g., "builder").
    /// @param source_var The C++ variable to read from (e.g., "field_name").
    /// @param field_name_capnp The Cap'n Proto field name (e.g., "fieldName").
    /// @param indent The indentation level.
    /// @param known_enums Optional set of known enum type names for proper handling.
    /// @return Generated C++ code as a string.
    static std::string generate_to_capnp_code(const Type& field,
                                                const std::string& builder_expr,
                                                const std::string& source_var,
                                                const std::string& field_name_capnp,
                                                int indent = 1,
                                                const std::set<std::string>& known_enums = {});

    /// @brief Get the C++ default value expression for a type.
    /// @param field The field type.
    /// @return C++ default value expression (e.g., "0", "{}", "\"\"").
    static std::string get_default_value(const Type& field);

private:
    /// @brief Generate indentation string.
    /// @param level The indentation level.
    /// @return String containing spaces for indentation.
    static std::string indent(int level);

    /// @brief Convert field name to Cap'n Proto getter/setter name.
    /// @param field_name The C++ field name.
    /// @return Cap'n Proto method name (camelCase with capital first letter).
    static std::string to_capnp_method_name(const std::string& field_name);

    /// @brief Check if a type is an enum (considering known_enums set).
    /// @param type The type to check.
    /// @param known_enums Set of known enum type names.
    /// @return True if the type is an enum.
    static bool is_enum_type(const Type& type, const std::set<std::string>& known_enums);
};

} // namespace curious::dsl::capnpgen

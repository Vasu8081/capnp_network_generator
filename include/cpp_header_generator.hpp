#pragma once

#include <sstream>
#include <string>
#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates C++ header files (.hpp) for each message from a parsed DSL Schema.
/// @details Creates complete C++ wrapper classes with Cap'n Proto conversion methods.
class CppHeaderGenerator
{
public:
    /// @brief Create a generator and immediately write header files to disk.
    /// @param schema Parsed DSL schema.
    /// @param output_directory Destination directory for header files.
    CppHeaderGenerator(const Schema& schema, const std::string& output_directory);

private:
    /// @brief Reference to the schema being generated.
    const Schema& _schema;

    /// @brief Output directory path.
    std::string _outputDirectory;

    /// @brief Resolve the output directory path.
    /// @param path The user-provided path.
    /// @return The resolved output directory path.
    static std::string _resolve_output_directory(const std::string& path);

    /// @brief Generate a header file for a single message.
    /// @param message The message to generate a header for.
    void _generate_header_for_message(const Message& message);

    /// @brief Read user-defined section from an existing header file.
    /// @param file_path The path to the existing header file.
    /// @param start_marker The start marker string.
    /// @param end_marker The end marker string.
    /// @return The user-defined section content, or empty string if not found.
    static std::string _read_user_section(const std::string& file_path,
                                           const char* start_marker,
                                           const char* end_marker);

    /// @brief Generate the complete header file content for a message.
    /// @param message The message to generate for.
    /// @param user_includes User-defined includes to preserve.
    /// @param user_methods User-defined methods to preserve.
    /// @param user_protected User-defined protected members to preserve.
    /// @param user_private User-defined private members to preserve.
    /// @return The complete header file content.
    std::string _generate_header_content(const Message& message,
                                          const std::string& user_includes,
                                          const std::string& user_methods,
                                          const std::string& user_protected,
                                          const std::string& user_private);

    /// @brief Generate field declarations section.
    /// @param message The message.
    /// @return Generated field declarations.
    std::string _generate_field_declarations(const Message& message);

    /// @brief Convert field name to Cap'n Proto method name.
    /// @param field_name The C++ field name.
    /// @return Cap'n Proto method name (camelCase with capital first letter).
    static std::string _to_capnp_method_name(const std::string& field_name);

    /// @brief Get the parent class name for a message.
    /// @param message The message.
    /// @return The parent class name, or "MessageBase" if no parent.
    std::string _get_parent_class_name(const Message& message);

    /// @brief Generate to_capnp_struct code for a single field.
    /// @param content Output stream.
    /// @param field The field.
    void _generate_to_capnp_struct_field(std::ostringstream& content, const Type& field) const;

    /// @brief Generate from_capnp_struct code for a single field.
    /// @param content Output stream.
    /// @param field The field.
    void _generate_from_capnp_struct_field(std::ostringstream& content, const Type& field) const;
};

} // namespace curious::dsl::capnpgen

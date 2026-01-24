#pragma once

#include <string>
#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates C++ source files (.cpp) for each message from a parsed DSL Schema.
/// @details Creates complete C++ implementations with Cap'n Proto conversion logic.
class CppSourceGenerator
{
public:
    /// @brief Create a generator and immediately write source files to disk.
    /// @param schema Parsed DSL schema.
    /// @param output_directory Destination directory for source files.
    /// @param capnp_header_name Name of the generated Cap'n Proto header file.
    /// @param include_prefix Prefix for header includes (e.g., "network/" for #include "network/Message.hpp").
    CppSourceGenerator(const Schema& schema,
                       const std::string& output_directory,
                       const std::string& capnp_header_name = "network_msg.capnp.h",
                       const std::string& include_prefix = "");

private:
    /// @brief Reference to the schema being generated.
    const Schema& _schema;

    /// @brief Output directory path.
    std::string _outputDirectory;

    /// @brief Cap'n Proto header file name.
    std::string _capnpHeaderName;

    /// @brief Include prefix for header files (e.g., "network/").
    std::string _includePrefix;

    /// @brief Resolve the output directory path.
    /// @param path The user-provided path.
    /// @return The resolved output directory path.
    static std::string _resolve_output_directory(const std::string& path);

    /// @brief Generate a source file for a single message.
    /// @param message The message to generate a source for.
    void _generate_source_for_message(const Message& message);

    /// @brief Read user-defined section from an existing source file.
    /// @param file_path The path to the existing source file.
    /// @param start_marker The start marker string.
    /// @param end_marker The end marker string.
    /// @return The user-defined section content, or empty string if not found.
    static std::string _read_user_section(const std::string& file_path,
                                           const char* start_marker,
                                           const char* end_marker);

    /// @brief Generate the complete source file content for a message.
    /// @param message The message to generate for.
    /// @param user_impl_includes User-defined implementation includes to preserve.
    /// @param user_constructor User-defined constructor logic to preserve.
    /// @param user_to_capnp User-defined to_capnp logic to preserve.
    /// @param user_from_capnp User-defined from_capnp logic to preserve.
    /// @param user_copy_from User-defined copy_from logic to preserve.
    /// @param user_impl User-defined implementations to preserve.
    /// @return The complete source file content.
    std::string _generate_source_content(const Message& message,
                                          const std::string& user_impl_includes,
                                          const std::string& user_constructor,
                                          const std::string& user_to_capnp,
                                          const std::string& user_from_capnp,
                                          const std::string& user_copy_from,
                                          const std::string& user_impl);

    /// @brief Generate constructor implementation.
    /// @param message The message.
    /// @param user_constructor User-defined constructor code.
    /// @return Generated constructor code.
    std::string _generate_constructor(const Message& message,
                                       const std::string& user_constructor);

    /// @brief Generate to_capnp implementation.
    /// @param message The message.
    /// @param user_to_capnp User-defined to_capnp code.
    /// @return Generated to_capnp code.
    std::string _generate_to_capnp(const Message& message,
                                    const std::string& user_to_capnp);

    /// @brief Generate from_capnp implementation.
    /// @param message The message.
    /// @param user_from_capnp User-defined from_capnp code.
    /// @return Generated from_capnp code.
    std::string _generate_from_capnp(const Message& message,
                                      const std::string& user_from_capnp);

    /// @brief Generate copy_from implementation.
    /// @param message The message.
    /// @param user_copy_from User-defined copy_from code.
    /// @return Generated copy_from code.
    std::string _generate_copy_from(const Message& message,
                                     const std::string& user_copy_from);

    /// @brief Get the parent class name for a message.
    /// @param message The message.
    /// @return The parent class name, or empty if no parent.
    std::string _get_parent_class_name(const Message& message);

    /// @brief Convert field name to Cap'n Proto method name.
    /// @param field_name The C++ field name.
    /// @return Cap'n Proto method name (camelCase with capital first letter).
    static std::string _to_capnp_method_name(const std::string& field_name);

    /// @brief Get the Cap'n Proto struct name for a message.
    /// @param message_name The message name.
    /// @return The Cap'n Proto struct name.
    std::string _get_capnp_struct_name(const std::string& message_name);

    /// @brief Check if a type name is a schema-defined enum.
    /// @param type_name The type name to check.
    /// @return True if it's a schema enum.
    bool _is_schema_enum(const std::string& type_name) const;

    /// @brief Generate to_capnp code for a single field.
    /// @param field The field to generate code for.
    /// @param builder_expr The builder expression (e.g., "root").
    /// @return Generated code.
    std::string _generate_field_to_capnp(const Type& field, const std::string& builder_expr);

    /// @brief Generate from_capnp code for a single field.
    /// @param field The field to generate code for.
    /// @param reader_expr The reader expression (e.g., "root").
    /// @return Generated code.
    std::string _generate_field_from_capnp(const Type& field, const std::string& reader_expr);
};

} // namespace curious::dsl::capnpgen

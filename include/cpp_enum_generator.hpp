#pragma once

#include <string>
#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates a single enums.hpp file containing all enum definitions.
/// @details Creates a header file with all enums from the schema for use in C++ wrapper classes.
class CppEnumGenerator
{
public:
    /// @brief Create a generator and immediately write the enums header file to disk.
    /// @param schema Parsed DSL schema containing enum definitions.
    /// @param output_directory Destination directory for the enums.hpp file.
    /// @param include_prefix Include prefix for the generated file (e.g., "network/").
    CppEnumGenerator(const Schema& schema, const std::string& output_directory, const std::string& include_prefix = "");

private:
    /// @brief Reference to the schema being generated.
    const Schema& _schema;

    /// @brief Output directory path.
    std::string _outputDirectory;

    /// @brief Include prefix for the generated file.
    std::string _includePrefix;

    /// @brief Resolve the output directory path.
    /// @param path The user-provided path.
    /// @return The resolved output directory path.
    static std::string _resolve_output_directory(const std::string& path);

    /// @brief Read user-defined section from an existing enums header file.
    /// @param file_path The path to the existing file.
    /// @param start_marker The start marker string.
    /// @param end_marker The end marker string.
    /// @return The user-defined section content, or empty string if not found.
    static std::string _read_user_section(const std::string& file_path, const char* start_marker, const char* end_marker);

    /// @brief Generate the complete enums.hpp file content.
    /// @param user_includes User-defined includes to preserve.
    /// @param user_definitions User-defined enum definitions to preserve.
    /// @return The complete header file content.
    std::string _generate_enums_header_content(const std::string& user_includes, const std::string& user_definitions);

    /// @brief Generate a single enum class definition.
    /// @param enum_decl The enum to generate.
    /// @return The enum class definition code.
    std::string _generate_enum_class(const EnumDecl& enum_decl);

    /// @brief Generate the MessageType enum from schema messages.
    /// @return The MessageType enum class definition code.
    std::string _generate_message_type_enum();
};

} // namespace curious::dsl::capnpgen

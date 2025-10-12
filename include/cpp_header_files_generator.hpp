#pragma once

#include <string>

#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates C++ header files for each message from a parsed DSL Schema.
/// @details Construction performs the generation and writes to disk.
class CppHeaderFileGenerator
{
public:
    /// @brief Create a generator and immediately write the files to disk.
    /// @param schema Parsed DSL schema.
    /// @param output_path Destination directory for header files.
    CppHeaderFileGenerator(const Schema& schema, const std::string& output_path);

private:
    /// @brief Reference to the schema being generated.
    const Schema& _schema;

    /// @brief Output directory path.
    std::string _outputPath;

    /// @brief Resolve the output directory path.
    /// @param path The user-provided path.
    /// @return The resolved output directory path.
    static std::string _resolve_output_directory(const std::string& path);

    /// @brief Generate a header file for a single message.
    /// @param message The message to generate a header for.
    void _generate_header_for_message(const Message& message) const;

    /// @brief Read user-defined includes from an existing header file.
    /// @param file_path The path to the existing header file.
    /// @return The user-defined includes section, or empty string if not found.
    static std::string _read_user_defined_includes(const std::string& file_path);

    /// @brief Read user-defined properties from an existing header file.
    /// @param file_path The path to the existing header file.
    /// @return The user-defined properties section, or empty string if not found.
    static std::string _read_user_defined_properties(const std::string& file_path);

    /// @brief Write the complete header file content for a message.
    /// @param message The message to write.
    /// @param user_includes User-defined includes to preserve.
    /// @param user_properties User-defined properties to preserve.
    /// @return The complete header file content.
    std::string _generate_header_content(const Message& message, const std::string& user_includes, const std::string& user_properties) const;
};

} // namespace curious::dsl::capnpgen

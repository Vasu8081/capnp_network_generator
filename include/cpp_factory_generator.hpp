#pragma once

#include <string>
#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates the factory_builder.h file containing the message factory class.
/// @details Creates the FactoryBuilder class that creates messages by MessageType enum value.
class CppFactoryGenerator
{
public:
    /// @brief Create a generator and immediately write the factory_builder.h file to disk.
    /// @param schema Parsed DSL schema containing message definitions.
    /// @param output_directory Destination directory for the factory_builder.h file.
    /// @param include_prefix Include prefix for the generated file (e.g., "network/").
    CppFactoryGenerator(const Schema& schema, const std::string& output_directory, const std::string& include_prefix = "");

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

    /// @brief Generate the complete factory_builder.h file content.
    /// @return The complete header file content.
    std::string _generate_factory_content();

    /// @brief Convert a message name to its corresponding enum value name.
    /// @param message_name The PascalCase message name.
    /// @return The camelCase enum value name.
    static std::string _to_enum_value_name(const std::string& message_name);
};

} // namespace curious::dsl::capnpgen

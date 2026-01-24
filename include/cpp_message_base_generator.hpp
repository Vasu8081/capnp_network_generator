#pragma once

#include <string>
#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates the MessageBase.hpp file containing the base class for all messages.
/// @details Creates the MessageBase class with SerializedData struct and serialize_fast() method.
class CppMessageBaseGenerator
{
public:
    /// @brief Create a generator and immediately write the MessageBase.hpp file to disk.
    /// @param schema Parsed DSL schema containing namespace information.
    /// @param output_directory Destination directory for the MessageBase.hpp file.
    /// @param include_prefix Include prefix for the generated file (e.g., "network/").
    CppMessageBaseGenerator(const Schema& schema, const std::string& output_directory, const std::string& include_prefix = "");

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

    /// @brief Generate the complete MessageBase.hpp file content.
    /// @return The complete header file content.
    std::string _generate_message_base_content();
};

} // namespace curious::dsl::capnpgen

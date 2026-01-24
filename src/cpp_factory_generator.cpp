#include "cpp_factory_generator.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "string_utils.hpp"

namespace curious::dsl::capnpgen
{

// ---- Constructor ----

CppFactoryGenerator::CppFactoryGenerator(const Schema& schema, const std::string& output_directory, const std::string& include_prefix)
    : _schema(schema)
    , _outputDirectory(_resolve_output_directory(output_directory))
    , _includePrefix(include_prefix)
{
    namespace fs = std::filesystem;

    // Construct output file path
    fs::path output_file_path = fs::path(_outputDirectory) / "factory_builder.h";

    // Generate content
    std::string content = _generate_factory_content();

    // Write to file
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to create factory_builder.h file: " + output_file_path.string());
    }

    output_file << content;
}

// ---- Private static methods ----

std::string CppFactoryGenerator::_resolve_output_directory(const std::string& path)
{
    namespace fs = std::filesystem;
    fs::create_directories(path);
    return path;
}

std::string CppFactoryGenerator::_to_enum_value_name(const std::string& message_name)
{
    if (message_name.empty())
    {
        return message_name;
    }

    // Convert PascalCase to camelCase (first letter lowercase)
    std::string result = message_name;
    result[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[0])));
    return result;
}

// ---- Private instance methods ----

std::string CppFactoryGenerator::_generate_factory_content()
{
    std::ostringstream content;

    // Use wrapper_namespace_name if specified, otherwise fall back to namespace_name
    const std::string& raw_ns = _schema.wrapper_namespace_name.empty() ?
                                  _schema.namespace_name : _schema.wrapper_namespace_name;
    const std::string ns = raw_ns.empty() ?
                             "curious::net" :
                             string_utils::to_cpp_namespace(raw_ns);

    // Header guard
    content << "#pragma once\n\n";
    content << "#ifndef FACTORY_BUILDER_H\n";
    content << "#define FACTORY_BUILDER_H\n\n";

    // System includes
    content << "#include <memory>\n";
    content << "#include <string>\n";
    content << "#include <stdexcept>\n";
    content << "#include <capnp/message.h>\n\n";

    // Include enums
    content << "#include <" << _includePrefix << "enums.hpp>\n";

    // Collect and sort message names
    std::vector<std::string> message_names;
    for (const auto& [name, msg] : _schema.messages)
    {
        message_names.push_back(name);
    }
    std::sort(message_names.begin(), message_names.end());

    // Include all message headers
    for (const auto& name : message_names)
    {
        content << "#include <" << _includePrefix << name << ".hpp>\n";
    }

    content << "\n";

    // Open namespace
    content << "namespace " << ns << " {\n\n";

    // FactoryBuilder class
    content << "/// @brief Factory class for creating message instances by type.\n";
    content << "/// @details Auto-generated factory that creates message objects based on MessageType enum.\n";
    content << "class FactoryBuilder {\n";
    content << "public:\n";
    content << "  /// @brief Create a message instance by its type.\n";
    content << "  /// @param type The message type enum value.\n";
    content << "  /// @return A shared pointer to the created message.\n";
    content << "  /// @throws std::runtime_error if the message type is unknown.\n";
    content << "  static std::shared_ptr<NetworkMessage> createMessage(MessageType type) {\n";
    content << "    switch (type) {\n";

    // Generate switch cases
    for (const auto& name : message_names)
    {
        std::string enum_value = _to_enum_value_name(name);
        content << "      case MessageType::" << enum_value << ": return std::make_shared<" << name << ">();\n";
    }

    content << "      default:\n";
    content << "        throw std::runtime_error(\"Unknown message type: \" + std::to_string(static_cast<int64_t>(type)));\n";
    content << "    }\n";
    content << "  }\n";
    content << "};\n\n";

    // Close namespace
    content << "} // namespace " << ns << "\n\n";

    // Close header guard
    content << "#endif // FACTORY_BUILDER_H\n";

    return content.str();
}

} // namespace curious::dsl::capnpgen

#include "cpp_enum_generator.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "string_utils.hpp"

namespace curious::dsl::capnpgen
{

namespace
{

constexpr const char* USER_INCLUDES_START = "// USER_INCLUDES_START";
constexpr const char* USER_INCLUDES_END = "// USER_INCLUDES_END";
constexpr const char* USER_DEFINITIONS_START = "// USER_DEFINITIONS_START";
constexpr const char* USER_DEFINITIONS_END = "// USER_DEFINITIONS_END";

std::string extract_between_markers(const std::string& content, const char* start_marker, const char* end_marker)
{
    std::size_t start_pos = content.find(start_marker);
    if (start_pos == std::string::npos)
    {
        return "";
    }

    start_pos += std::strlen(start_marker);

    // Skip to next line
    std::size_t line_end = content.find('\n', start_pos);
    if (line_end != std::string::npos)
    {
        start_pos = line_end + 1;
    }

    std::size_t end_pos = content.find(end_marker, start_pos);
    if (end_pos == std::string::npos)
    {
        return "";
    }

    return content.substr(start_pos, end_pos - start_pos);
}

} // anonymous namespace

// ---- Constructor ----

CppEnumGenerator::CppEnumGenerator(const Schema& schema, const std::string& output_directory, const std::string& include_prefix)
    : _schema(schema)
    , _outputDirectory(_resolve_output_directory(output_directory))
    , _includePrefix(include_prefix)
{
    namespace fs = std::filesystem;

    // Construct output file path
    fs::path output_file_path = fs::path(_outputDirectory) / "enums.hpp";

    // Read user-defined sections if file exists
    std::string user_includes = _read_user_section(output_file_path.string(), USER_INCLUDES_START, USER_INCLUDES_END);
    std::string user_definitions = _read_user_section(output_file_path.string(), USER_DEFINITIONS_START, USER_DEFINITIONS_END);

    // Generate content
    std::string content = _generate_enums_header_content(user_includes, user_definitions);

    // Write to file
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to create enums header file: " + output_file_path.string());
    }

    output_file << content;
}

// ---- Private static methods ----

std::string CppEnumGenerator::_resolve_output_directory(const std::string& path)
{
    namespace fs = std::filesystem;
    fs::create_directories(path);
    return path;
}

std::string CppEnumGenerator::_read_user_section(const std::string& file_path, const char* start_marker, const char* end_marker)
{
    try
    {
        std::string content = string_utils::read_file(file_path);
        return extract_between_markers(content, start_marker, end_marker);
    }
    catch (...)
    {
        return ""; // File doesn't exist or can't be read
    }
}

// ---- Private instance methods ----

std::string CppEnumGenerator::_generate_enum_class(const EnumDecl& enum_decl)
{
    std::ostringstream code;

    code << "/// @brief Enum: " << enum_decl.name << "\n";
    if (enum_decl.capnp_id != 0)
    {
        code << "/// @details Cap'n Proto ID: 0x" << std::hex << enum_decl.capnp_id << std::dec << "\n";
    }

    code << "enum class " << enum_decl.name << " : std::int64_t\n";
    code << "{\n";

    for (const auto& value : enum_decl.values)
    {
        code << "    " << value.name << " = " << value.value << ",\n";
    }

    code << "};\n\n";

    // Add operator<< for convenient printing
    code << "/// @brief Stream output operator for " << enum_decl.name << ".\n";
    code << "inline std::ostream& operator<<(std::ostream& os, " << enum_decl.name << " value)\n";
    code << "{\n";
    code << "    switch (value)\n";
    code << "    {\n";

    for (const auto& enum_value : enum_decl.values)
    {
        code << "        case " << enum_decl.name << "::" << enum_value.name << ": return os << \""
             << enum_value.name << "\";\n";
    }

    code << "        default: return os << \"Unknown(\" << static_cast<std::int64_t>(value) << \")\";\n";
    code << "    }\n";
    code << "}\n\n";

    return code.str();
}

std::string CppEnumGenerator::_generate_enums_header_content(const std::string& user_includes, const std::string& user_definitions)
{
    std::ostringstream content;

    // Header guard
    content << "#pragma once\n\n";
    content << "#ifndef ENUMS_HPP\n";
    content << "#define ENUMS_HPP\n\n";

    // Includes
    content << "#include <cstdint>\n";
    content << "#include <ostream>\n\n";

    // User includes
    content << USER_INCLUDES_START << "\n";
    if (!user_includes.empty())
    {
        content << user_includes;
    }
    content << USER_INCLUDES_END << "\n\n";

    // Namespace
    const std::string& ns = _schema.namespace_name.empty() ?
                             "curious::message" : _schema.namespace_name;
    content << "namespace " << ns << "\n{\n\n";

    content << "// ---- Auto-Generated Enum Definitions ----\n\n";

    // Generate enums in alphabetical order (excluding MessageType)
    std::vector<std::string> enum_names;
    for (const auto& [name, enum_decl] : _schema.enums)
    {
        enum_names.push_back(name);
    }
    std::sort(enum_names.begin(), enum_names.end());

    for (const auto& name : enum_names)
    {
        content << _generate_enum_class(_schema.enums.at(name));
    }

    // User definitions section
    content << USER_DEFINITIONS_START << "\n";
    if (!user_definitions.empty())
    {
        content << user_definitions;
    }
    else
    {
        content << "// Add your custom enum definitions or helper functions here\n";
        content << "// Example:\n";
        content << "// inline bool is_success(Status status) { return status == Status::OK; }\n";
    }
    content << USER_DEFINITIONS_END << "\n\n";

    // Close namespace
    content << "} // namespace " << ns << "\n\n";

    // Close header guard
    content << "#endif // ENUMS_HPP\n";

    return content.str();
}

} // namespace curious::dsl::capnpgen

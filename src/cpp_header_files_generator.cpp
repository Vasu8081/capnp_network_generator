#include "cpp_header_files_generator.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "string_utils.hpp"

namespace curious::dsl::capnpgen
{

namespace
{

/// @brief Marker for the start of user-defined includes section.
constexpr const char* USER_INCLUDES_START_MARKER = "// USER_INCLUDES_START";

/// @brief Marker for the end of user-defined includes section.
constexpr const char* USER_INCLUDES_END_MARKER = "// USER_INCLUDES_END";

/// @brief Marker for the start of user-defined properties section.
constexpr const char* USER_PROPERTIES_START_MARKER = "// USER_PROPERTIES_START";

/// @brief Marker for the end of user-defined properties section.
constexpr const char* USER_PROPERTIES_END_MARKER = "// USER_PROPERTIES_END";

/// @brief Extract content between two markers in a file.
/// @param file_content The file content to search.
/// @param start_marker The start marker.
/// @param end_marker The end marker.
/// @return The content between markers, or empty string if not found.
std::string extract_content_between_markers(const std::string& file_content, const char* start_marker, const char* end_marker)
{
    std::size_t start_pos = file_content.find(start_marker);
    if (start_pos == std::string::npos)
    {
        return ""; // Marker not found
    }

    start_pos += std::strlen(start_marker);

    // Skip to next line
    std::size_t line_end = file_content.find('\n', start_pos);
    if (line_end != std::string::npos)
    {
        start_pos = line_end + 1;
    }

    std::size_t end_pos = file_content.find(end_marker, start_pos);
    if (end_pos == std::string::npos)
    {
        return ""; // End marker not found
    }

    return file_content.substr(start_pos, end_pos - start_pos);
}

} // anonymous namespace

// ---- Constructor ----

CppHeaderFileGenerator::CppHeaderFileGenerator(const Schema& schema, const std::string& output_path)
    : _schema(schema)
    , _outputPath(_resolve_output_directory(output_path))
{
    // Generate a header file for each message
    for (const auto& [message_name, message] : _schema.messages)
    {
        _generate_header_for_message(message);
    }
}

// ---- Private static methods ----

std::string CppHeaderFileGenerator::_resolve_output_directory(const std::string& path)
{
    namespace fs = std::filesystem;

    fs::create_directories(path);
    return path;
}

std::string CppHeaderFileGenerator::_read_user_defined_includes(const std::string& file_path)
{
    try
    {
        std::string file_content = string_utils::read_file(file_path);
        return extract_content_between_markers(file_content, USER_INCLUDES_START_MARKER, USER_INCLUDES_END_MARKER);
    }
    catch (...)
    {
        return ""; // File doesn't exist or can't be read
    }
}

std::string CppHeaderFileGenerator::_read_user_defined_properties(const std::string& file_path)
{
    try
    {
        std::string file_content = string_utils::read_file(file_path);
        return extract_content_between_markers(file_content, USER_PROPERTIES_START_MARKER, USER_PROPERTIES_END_MARKER);
    }
    catch (...)
    {
        return ""; // File doesn't exist or can't be read
    }
}

// ---- Private instance methods ----

void CppHeaderFileGenerator::_generate_header_for_message(const Message& message) const
{
    namespace fs = std::filesystem;

    // Construct output file path
    fs::path output_file_path = fs::path(_outputPath) / (message.name + ".hpp");

    // Read user-defined sections if file exists
    std::string user_includes = _read_user_defined_includes(output_file_path.string());
    std::string user_properties = _read_user_defined_properties(output_file_path.string());

    // Generate header content
    std::string header_content = _generate_header_content(message, user_includes, user_properties);

    // Write to file
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to create header file: " + output_file_path.string());
    }

    output_file << header_content;
}

std::string CppHeaderFileGenerator::_generate_header_content(const Message& message, const std::string& user_includes, const std::string& user_properties) const
{
    std::ostringstream content;

    // Header guard
    std::string guard_name = message.name;
    for (char& c : guard_name)
    {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    guard_name += "_HPP";

    content << "#pragma once\n\n";
    content << "#ifndef " << guard_name << "\n";
    content << "#define " << guard_name << "\n\n";

    // Standard includes
    content << "#include <cstdint>\n";
    content << "#include <string>\n";
    content << "#include <vector>\n";
    content << "#include <unordered_map>\n\n";

    // User-defined includes section
    content << USER_INCLUDES_START_MARKER << "\n";
    if (!user_includes.empty())
    {
        content << user_includes;
    }
    content << USER_INCLUDES_END_MARKER << "\n\n";

    // Namespace
    const std::string& ns = _schema.namespace_name.empty() ? "curious::message" : _schema.namespace_name;
    content << "namespace " << ns << "\n{\n\n";

    // Class definition
    content << "/// @brief Auto-generated message class for " << message.name << ".\n";
    content << "class " << message.name << "\n";
    content << "{\n";
    content << "public:\n";

    // Default constructor
    content << "    /// @brief Default constructor.\n";
    content << "    " << message.name << "() = default;\n\n";

    // Destructor
    content << "    /// @brief Destructor.\n";
    content << "    ~" << message.name << "() = default;\n\n";

    // Public fields (generated from DSL)
    content << "    // ---- Generated fields ----\n\n";
    for (const auto& field : message.fields)
    {
        content << "    /// @brief Field: " << field.get_field_name() << "\n";
        content << "    " << field.get_cpp_type() << " " << field.get_field_name() << ";\n\n";
    }

    // User-defined properties section
    content << "    " << USER_PROPERTIES_START_MARKER << "\n";
    if (!user_properties.empty())
    {
        content << user_properties;
    }
    content << "    " << USER_PROPERTIES_END_MARKER << "\n";

    content << "};\n\n";

    // Close namespace
    content << "} // namespace " << ns << "\n\n";

    // Close header guard
    content << "#endif // " << guard_name << "\n";

    return content.str();
}

} // namespace curious::dsl::capnpgen

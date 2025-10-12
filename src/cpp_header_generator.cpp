#include "cpp_header_generator.hpp"

#include <algorithm>
#include <cctype>
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
constexpr const char* USER_METHODS_START = "// USER_METHODS_START";
constexpr const char* USER_METHODS_END = "// USER_METHODS_END";
constexpr const char* USER_PROTECTED_START = "// USER_PROTECTED_START";
constexpr const char* USER_PROTECTED_END = "// USER_PROTECTED_END";
constexpr const char* USER_PRIVATE_START = "// USER_PRIVATE_START";
constexpr const char* USER_PRIVATE_END = "// USER_PRIVATE_END";

std::string extract_between_markers(const std::string& content,
                                      const char* start_marker,
                                      const char* end_marker)
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

CppHeaderGenerator::CppHeaderGenerator(const Schema& schema, const std::string& output_directory)
    : _schema(schema)
    , _outputDirectory(_resolve_output_directory(output_directory))
{
    // Generate header for each message
    for (const auto& [message_name, message] : _schema.messages)
    {
        _generate_header_for_message(message);
    }
}

// ---- Private static methods ----

std::string CppHeaderGenerator::_resolve_output_directory(const std::string& path)
{
    namespace fs = std::filesystem;
    fs::create_directories(path);
    return path;
}

std::string CppHeaderGenerator::_read_user_section(const std::string& file_path,
                                                     const char* start_marker,
                                                     const char* end_marker)
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

std::string CppHeaderGenerator::_to_capnp_method_name(const std::string& field_name)
{
    if (field_name.empty())
    {
        return field_name;
    }

    std::string result = field_name;
    result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

// ---- Private instance methods ----

std::string CppHeaderGenerator::_get_parent_class_name(const Message& message)
{
    if (message.parent_name.empty())
    {
        return "curious::dsl::capnpgen::MessageBase";
    }

    return message.parent_name;
}

void CppHeaderGenerator::_generate_header_for_message(const Message& message)
{
    namespace fs = std::filesystem;

    fs::path output_file_path = fs::path(_outputDirectory) / (message.name + ".hpp");

    // Read user-defined sections
    std::string user_includes = _read_user_section(output_file_path.string(),
                                                     USER_INCLUDES_START,
                                                     USER_INCLUDES_END);
    std::string user_methods = _read_user_section(output_file_path.string(),
                                                    USER_METHODS_START,
                                                    USER_METHODS_END);
    std::string user_protected = _read_user_section(output_file_path.string(),
                                                      USER_PROTECTED_START,
                                                      USER_PROTECTED_END);
    std::string user_private = _read_user_section(output_file_path.string(),
                                                    USER_PRIVATE_START,
                                                    USER_PRIVATE_END);

    // Generate content
    std::string content = _generate_header_content(message,
                                                     user_includes,
                                                     user_methods,
                                                     user_protected,
                                                     user_private);

    // Write to file
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to create header file: " + output_file_path.string());
    }

    output_file << content;
}

std::string CppHeaderGenerator::_generate_field_declarations(const Message& message)
{
    std::ostringstream fields;

    for (const auto& field : message.fields)
    {
        fields << "    /// @brief Field: " << field.get_field_name() << "\n";
        fields << "    /// @details Type: " << field.get_cpp_type() << "\n";
        fields << "    " << field.get_cpp_type() << " " << field.get_field_name() << ";\n\n";
    }

    return fields.str();
}

std::string CppHeaderGenerator::_generate_header_content(const Message& message,
                                                           const std::string& user_includes,
                                                           const std::string& user_methods,
                                                           const std::string& user_protected,
                                                           const std::string& user_private)
{
    std::ostringstream content;

    // Header guard
    std::string guard_name = message.name;
    for (char& c : guard_name)
    {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    guard_name += "_HPP";

    content << "#pragma once\n\n";
    content << "#ifndef " << guard_name << "\n";
    content << "#define " << guard_name << "\n\n";

    // Includes
    content << "#include <cstdint>\n";
    content << "#include <string>\n";
    content << "#include <vector>\n";
    content << "#include <unordered_map>\n";
    content << "#include <memory>\n\n";
    content << "#include \"message_base.hpp\"\n\n";

    // Forward declare Cap'n Proto
    content << "// Forward declarations for Cap'n Proto\n";
    content << "namespace capnp {\n";
    content << "    class MessageBuilder;\n";
    content << "    class MessageReader;\n";
    content << "}\n\n";

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

    // Forward declarations for parent and related types
    if (!message.parent_name.empty())
    {
        content << "// Forward declaration\n";
        content << "class " << message.parent_name << ";\n\n";
    }

    // Class declaration
    content << "/// @brief Auto-generated message class for " << message.name << ".\n";
    content << "/// @details Message ID: " << message.id << "\n";
    if (!message.parent_name.empty())
    {
        content << "///          Inherits from: " << message.parent_name << "\n";
    }
    content << "class " << message.name << " : public " << _get_parent_class_name(message) << "\n";
    content << "{\n";
    content << "public:\n";

    // Constructors
    content << "    // ---- Constructors and Destructor ----\n\n";
    content << "    /// @brief Default constructor.\n";
    content << "    " << message.name << "();\n\n";

    content << "    /// @brief Copy constructor.\n";
    content << "    " << message.name << "(const " << message.name << "& other);\n\n";

    content << "    /// @brief Move constructor.\n";
    content << "    " << message.name << "(" << message.name << "&& other) noexcept;\n\n";

    content << "    /// @brief Copy assignment operator.\n";
    content << "    " << message.name << "& operator=(const " << message.name << "& other);\n\n";

    content << "    /// @brief Move assignment operator.\n";
    content << "    " << message.name << "& operator=(" << message.name << "&& other) noexcept;\n\n";

    content << "    /// @brief Destructor.\n";
    content << "    virtual ~" << message.name << "();\n\n";

    // MessageBase interface implementation
    content << "    // ---- MessageBase Interface ----\n\n";
    content << "    /// @brief Get the message type identifier.\n";
    content << "    /// @return The message type ID.\n";
    content << "    std::uint64_t get_message_id() const override;\n\n";

    content << "    /// @brief Get the message type name.\n";
    content << "    /// @return The message type name as a string.\n";
    content << "    std::string get_message_name() const override;\n\n";

    content << "    /// @brief Serialize this message to a byte vector.\n";
    content << "    /// @return A vector containing the serialized message.\n";
    content << "    std::vector<std::uint8_t> serialize() const override;\n\n";

    content << "    /// @brief Deserialize from a byte vector.\n";
    content << "    /// @param data The serialized data.\n";
    content << "    /// @return True if deserialization succeeded, false otherwise.\n";
    content << "    bool deserialize(const std::vector<std::uint8_t>& data) override;\n\n";

    content << "    /// @brief Deserialize from a byte buffer.\n";
    content << "    /// @param data Pointer to the data buffer.\n";
    content << "    /// @param size Size of the data buffer in bytes.\n";
    content << "    /// @return True if deserialization succeeded, false otherwise.\n";
    content << "    bool deserialize(const std::uint8_t* data, std::size_t size) override;\n\n";

    // Cap'n Proto conversion methods
    content << "    // ---- Cap'n Proto Conversion Methods ----\n\n";
    content << "    /// @brief Convert this object to a Cap'n Proto message builder.\n";
    content << "    /// @param message_builder The Cap'n Proto message builder to populate.\n";
    content << "    void to_capnp(::capnp::MessageBuilder& message_builder) const;\n\n";

    content << "    /// @brief Populate this object from a Cap'n Proto message reader.\n";
    content << "    /// @param message_reader The Cap'n Proto message reader to read from.\n";
    content << "    void from_capnp(::capnp::MessageReader& message_reader);\n\n";

    // Fields
    content << "    // ---- Generated Fields ----\n\n";
    content << _generate_field_declarations(message);

    // User methods
    content << "    " << USER_METHODS_START << "\n";
    if (!user_methods.empty())
    {
        content << user_methods;
    }
    content << "    " << USER_METHODS_END << "\n\n";

    // Protected section
    content << "protected:\n";
    content << "    " << USER_PROTECTED_START << "\n";
    if (!user_protected.empty())
    {
        content << user_protected;
    }
    content << "    " << USER_PROTECTED_END << "\n\n";

    // Private section
    content << "private:\n";
    content << "    /// @brief Copy fields from another instance.\n";
    content << "    /// @param other The instance to copy from.\n";
    content << "    void _copy_from(const " << message.name << "& other);\n\n";

    content << "    " << USER_PRIVATE_START << "\n";
    if (!user_private.empty())
    {
        content << user_private;
    }
    content << "    " << USER_PRIVATE_END << "\n";

    content << "};\n\n";

    // Close namespace
    content << "} // namespace " << ns << "\n\n";

    // Close header guard
    content << "#endif // " << guard_name << "\n";

    return content.str();
}

} // namespace curious::dsl::capnpgen

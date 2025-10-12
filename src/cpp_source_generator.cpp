#include "cpp_source_generator.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "string_utils.hpp"
#include "type_converter.hpp"

namespace curious::dsl::capnpgen
{

namespace
{

constexpr const char* USER_IMPL_INCLUDES_START = "// USER_IMPL_INCLUDES_START";
constexpr const char* USER_IMPL_INCLUDES_END = "// USER_IMPL_INCLUDES_END";
constexpr const char* USER_CONSTRUCTOR_START = "// USER_CONSTRUCTOR_START";
constexpr const char* USER_CONSTRUCTOR_END = "// USER_CONSTRUCTOR_END";
constexpr const char* USER_TO_CAPNP_START = "// USER_TO_CAPNP_START";
constexpr const char* USER_TO_CAPNP_END = "// USER_TO_CAPNP_END";
constexpr const char* USER_FROM_CAPNP_START = "// USER_FROM_CAPNP_START";
constexpr const char* USER_FROM_CAPNP_END = "// USER_FROM_CAPNP_END";
constexpr const char* USER_COPY_FROM_START = "// USER_COPY_FROM_START";
constexpr const char* USER_COPY_FROM_END = "// USER_COPY_FROM_END";
constexpr const char* USER_IMPL_START = "// USER_IMPL_START";
constexpr const char* USER_IMPL_END = "// USER_IMPL_END";

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

CppSourceGenerator::CppSourceGenerator(const Schema& schema,
                                       const std::string& output_directory,
                                       const std::string& capnp_header_name,
                                       const std::string& include_prefix)
    : _schema(schema)
    , _outputDirectory(_resolve_output_directory(output_directory))
    , _capnpHeaderName(capnp_header_name)
    , _includePrefix(include_prefix)
{
    // Generate source for each message
    for (const auto& [message_name, message] : _schema.messages)
    {
        _generate_source_for_message(message);
    }
}

// ---- Private static methods ----

std::string CppSourceGenerator::_resolve_output_directory(const std::string& path)
{
    namespace fs = std::filesystem;
    fs::create_directories(path);
    return path;
}

std::string CppSourceGenerator::_read_user_section(const std::string& file_path,
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

std::string CppSourceGenerator::_to_capnp_method_name(const std::string& field_name)
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

std::string CppSourceGenerator::_get_parent_class_name(const Message& message)
{
    if (message.parent_name.empty())
    {
        return "";
    }

    return message.parent_name;
}

std::string CppSourceGenerator::_get_capnp_struct_name(const std::string& message_name)
{
    return "NetworkMsg::" + message_name;
}

void CppSourceGenerator::_generate_source_for_message(const Message& message)
{
    namespace fs = std::filesystem;

    fs::path output_file_path = fs::path(_outputDirectory) / (message.name + ".cpp");

    // Read user-defined sections
    std::string user_impl_includes = _read_user_section(output_file_path.string(),
                                                         USER_IMPL_INCLUDES_START,
                                                         USER_IMPL_INCLUDES_END);
    std::string user_constructor = _read_user_section(output_file_path.string(),
                                                       USER_CONSTRUCTOR_START,
                                                       USER_CONSTRUCTOR_END);
    std::string user_to_capnp = _read_user_section(output_file_path.string(),
                                                     USER_TO_CAPNP_START,
                                                     USER_TO_CAPNP_END);
    std::string user_from_capnp = _read_user_section(output_file_path.string(),
                                                       USER_FROM_CAPNP_START,
                                                       USER_FROM_CAPNP_END);
    std::string user_copy_from = _read_user_section(output_file_path.string(),
                                                      USER_COPY_FROM_START,
                                                      USER_COPY_FROM_END);
    std::string user_impl = _read_user_section(output_file_path.string(),
                                                 USER_IMPL_START,
                                                 USER_IMPL_END);

    // Generate content
    std::string content = _generate_source_content(message,
                                                     user_impl_includes,
                                                     user_constructor,
                                                     user_to_capnp,
                                                     user_from_capnp,
                                                     user_copy_from,
                                                     user_impl);

    // Write to file
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to create source file: " + output_file_path.string());
    }

    output_file << content;
}

std::string CppSourceGenerator::_generate_constructor(const Message& message,
                                                        const std::string& user_constructor)
{
    std::ostringstream code;

    code << message.name << "::" << message.name << "()\n";

    // Initialize parent if exists
    if (!message.parent_name.empty())
    {
        code << "    : " << message.parent_name << "()\n";
    }

    code << "{\n";
    code << "    " << USER_CONSTRUCTOR_START << "\n";
    if (!user_constructor.empty())
    {
        code << user_constructor;
    }
    code << "    " << USER_CONSTRUCTOR_END << "\n";
    code << "}\n\n";

    return code.str();
}

std::string CppSourceGenerator::_generate_to_capnp(const Message& message,
                                                     const std::string& user_to_capnp)
{
    std::ostringstream code;

    std::string capnp_struct = _get_capnp_struct_name(message.name);

    code << "void " << message.name << "::to_capnp(::capnp::MessageBuilder& message_builder) const\n";
    code << "{\n";
    code << "    auto root = message_builder.initRoot<" << capnp_struct << ">();\n\n";

    // Call parent's method if exists
    if (!message.parent_name.empty())
    {
        code << "    // Populate parent fields\n";
        code << "    " << message.parent_name << "::to_capnp(message_builder);\n\n";
    }

    // Generate field conversions
    for (const auto& field : message.fields)
    {
        std::string method_name = _to_capnp_method_name(field.get_field_name());
        code << "    // Field: " << field.get_field_name() << "\n";
        code << TypeConverter::generate_to_capnp_code(field, "root", field.get_field_name(), field.get_field_name(), 1);
        code << "\n";
    }

    code << "    " << USER_TO_CAPNP_START << "\n";
    if (!user_to_capnp.empty())
    {
        code << user_to_capnp;
    }
    code << "    " << USER_TO_CAPNP_END << "\n";
    code << "}\n\n";

    return code.str();
}

std::string CppSourceGenerator::_generate_from_capnp(const Message& message,
                                                       const std::string& user_from_capnp)
{
    std::ostringstream code;

    std::string capnp_struct = _get_capnp_struct_name(message.name);

    code << "void " << message.name << "::from_capnp(::capnp::MessageReader& message_reader)\n";
    code << "{\n";
    code << "    auto root = message_reader.getRoot<" << capnp_struct << ">();\n\n";

    // Call parent's method if exists
    if (!message.parent_name.empty())
    {
        code << "    // Read parent fields\n";
        code << "    " << message.parent_name << "::from_capnp(message_reader);\n\n";
    }

    // Generate field conversions
    for (const auto& field : message.fields)
    {
        code << "    // Field: " << field.get_field_name() << "\n";
        code << TypeConverter::generate_from_capnp_code(field, "root", field.get_field_name(), 1);
        code << "\n";
    }

    code << "    " << USER_FROM_CAPNP_START << "\n";
    if (!user_from_capnp.empty())
    {
        code << user_from_capnp;
    }
    code << "    " << USER_FROM_CAPNP_END << "\n";
    code << "}\n\n";

    return code.str();
}

std::string CppSourceGenerator::_generate_copy_from(const Message& message,
                                                      const std::string& user_copy_from)
{
    std::ostringstream code;

    code << "void " << message.name << "::_copy_from(const " << message.name << "& other)\n";
    code << "{\n";

    // Copy each field
    for (const auto& field : message.fields)
    {
        code << "    " << field.get_field_name() << " = other." << field.get_field_name() << ";\n";
    }

    if (!message.fields.empty())
    {
        code << "\n";
    }

    code << "    " << USER_COPY_FROM_START << "\n";
    if (!user_copy_from.empty())
    {
        code << user_copy_from;
    }
    code << "    " << USER_COPY_FROM_END << "\n";
    code << "}\n\n";

    return code.str();
}

std::string CppSourceGenerator::_generate_source_content(const Message& message,
                                                           const std::string& user_impl_includes,
                                                           const std::string& user_constructor,
                                                           const std::string& user_to_capnp,
                                                           const std::string& user_from_capnp,
                                                           const std::string& user_copy_from,
                                                           const std::string& user_impl)
{
    std::ostringstream content;

    // Include header with proper prefix
    content << "#include \"" << _includePrefix << message.name << ".hpp\"\n\n";

    // Include Cap'n Proto headers
    content << "#include <capnp/message.h>\n";
    content << "#include <capnp/serialize.h>\n";
    content << "#include <kj/array.h>\n\n";
    content << "#include \"" << _includePrefix << "enums.hpp\"\n";
    content << "#include \"" << _capnpHeaderName << "\"\n\n";

    // User implementation includes
    content << USER_IMPL_INCLUDES_START << "\n";
    if (!user_impl_includes.empty())
    {
        content << user_impl_includes;
    }
    content << USER_IMPL_INCLUDES_END << "\n\n";

    // Namespace
    const std::string& ns = _schema.namespace_name.empty() ?
                             "curious::message" : _schema.namespace_name;
    content << "namespace " << ns << "\n{\n\n";

    // Constructors and destructor
    content << "// ---- Constructors and Destructor ----\n\n";
    content << _generate_constructor(message, user_constructor);

    content << message.name << "::" << message.name << "(const " << message.name << "& other)\n";
    if (!message.parent_name.empty())
    {
        content << "    : " << message.parent_name << "(other)\n";
    }
    content << "{\n";
    content << "    _copy_from(other);\n";
    content << "}\n\n";

    content << message.name << "::" << message.name << "(" << message.name << "&& other) noexcept\n";
    if (!message.parent_name.empty())
    {
        content << "    : " << message.parent_name << "(std::move(other))\n";
    }
    else
    {
        content << "    : MessageBase(std::move(other))\n";
    }

    // Move each field
    if (!message.fields.empty())
    {
        for (const auto& field : message.fields)
        {
            content << "    , " << field.get_field_name() << "(std::move(other."
                    << field.get_field_name() << "))\n";
        }
    }
    content << "{\n";
    content << "}\n\n";

    content << message.name << "& " << message.name << "::operator=(const "
            << message.name << "& other)\n";
    content << "{\n";
    content << "    if (this != &other)\n";
    content << "    {\n";
    if (!message.parent_name.empty())
    {
        content << "        " << message.parent_name << "::operator=(other);\n";
    }
    content << "        _copy_from(other);\n";
    content << "    }\n";
    content << "    return *this;\n";
    content << "}\n\n";

    content << message.name << "& " << message.name << "::operator=("
            << message.name << "&& other) noexcept\n";
    content << "{\n";
    content << "    if (this != &other)\n";
    content << "    {\n";
    if (!message.parent_name.empty())
    {
        content << "        " << message.parent_name << "::operator=(std::move(other));\n";
    }
    else
    {
        content << "        MessageBase::operator=(std::move(other));\n";
    }

    for (const auto& field : message.fields)
    {
        content << "        " << field.get_field_name() << " = std::move(other."
                << field.get_field_name() << ");\n";
    }

    content << "    }\n";
    content << "    return *this;\n";
    content << "}\n\n";

    content << message.name << "::~" << message.name << "() = default;\n\n";

    // MessageBase interface implementation
    content << "// ---- MessageBase Interface ----\n\n";
    content << "std::uint64_t " << message.name << "::get_message_id() const\n";
    content << "{\n";
    content << "    return " << message.id << ";\n";
    content << "}\n\n";

    content << "std::string " << message.name << "::get_message_name() const\n";
    content << "{\n";
    content << "    return \"" << message.name << "\";\n";
    content << "}\n\n";

    content << "std::vector<std::uint8_t> " << message.name << "::serialize() const\n";
    content << "{\n";
    content << "    ::capnp::MallocMessageBuilder msg_builder;\n";
    content << "    to_capnp(msg_builder);\n\n";
    content << "    kj::Array<capnp::word> words = capnp::messageToFlatArray(msg_builder);\n";
    content << "    auto bytes = words.asBytes();\n\n";
    content << "    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());\n";
    content << "}\n\n";

    content << "bool " << message.name << "::deserialize(const std::vector<std::uint8_t>& data)\n";
    content << "{\n";
    content << "    return deserialize(data.data(), data.size());\n";
    content << "}\n\n";

    content << "bool " << message.name << "::deserialize(const std::uint8_t* data, std::size_t size)\n";
    content << "{\n";
    content << "    try\n";
    content << "    {\n";
    content << "        kj::ArrayPtr<const capnp::word> words(\n";
    content << "            reinterpret_cast<const capnp::word*>(data),\n";
    content << "            size / sizeof(capnp::word)\n";
    content << "        );\n\n";
    content << "        ::capnp::FlatArrayMessageReader reader(words);\n";
    content << "        from_capnp(reader);\n\n";
    content << "        return true;\n";
    content << "    }\n";
    content << "    catch (...)\n";
    content << "    {\n";
    content << "        return false;\n";
    content << "    }\n";
    content << "}\n\n";

    // Cap'n Proto conversion methods
    content << "// ---- Cap'n Proto Conversion Methods ----\n\n";
    content << _generate_to_capnp(message, user_to_capnp);
    content << _generate_from_capnp(message, user_from_capnp);

    // Private helpers
    content << "// ---- Private Helpers ----\n\n";
    content << _generate_copy_from(message, user_copy_from);

    // User implementations
    content << USER_IMPL_START << "\n";
    if (!user_impl.empty())
    {
        content << user_impl;
    }
    content << USER_IMPL_END << "\n\n";

    // Close namespace
    content << "} // namespace " << ns << "\n";

    return content.str();
}

} // namespace curious::dsl::capnpgen

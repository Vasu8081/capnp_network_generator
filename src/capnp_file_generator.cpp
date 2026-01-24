#include "capnp_file_generator.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "id_generator.hpp"

namespace curious::dsl::capnpgen
{

// ---- Constructor ----

CapnpFileGenerator::CapnpFileGenerator(const Schema& schema, const std::string& output_path)
    : _schema(schema)
    , _outputPath(_resolve_output_path(output_path))
    , _fileId(_initialize_file_id(_outputPath))
{
    // Open output file
    std::ofstream output_file(_outputPath, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to open output file: " + _outputPath);
    }

    // Generate content
    std::ostringstream content;
    _write_header(content);
    _write_all_enums(content);
    _write_map_template(content);
    _write_all_structs(content);

    // Write to file
    output_file << content.str();
}

// ---- Private static methods ----

std::string CapnpFileGenerator::_resolve_output_path(const std::string& path)
{
    namespace fs = std::filesystem;

    fs::path file_path(path);

    // If path ends with .capnp, use it directly
    if (file_path.extension() == ".capnp")
    {
        // Create parent directories if needed
        if (file_path.has_parent_path())
        {
            fs::create_directories(file_path.parent_path());
        }
        return file_path.string();
    }

    // Otherwise, treat as directory and create network_msg.capnp inside
    fs::create_directories(file_path);
    return (file_path / "network_msg.capnp").string();
}

std::uint64_t CapnpFileGenerator::_initialize_file_id(const std::string& resolved_path)
{
    // Try to extract existing file ID from the output file
    std::uint64_t existing_id = IdGenerator::extract_file_id_from_capnp(resolved_path);

    if (existing_id != 0)
    {
        // Reuse existing file ID to preserve derived IDs
        return existing_id;
    }

    // No existing file or couldn't extract ID, generate new random ID
    return IdGenerator::generate_random_id();
}

std::string CapnpFileGenerator::_to_capnp_identifier(const std::string& identifier)
{
    std::string result;
    result.reserve(identifier.size());

    for (char c : identifier)
    {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            result.push_back('_');
        }
        else
        {
            result.push_back(c);
        }
    }

    return result;
}

void CapnpFileGenerator::_write_map_template(std::ostringstream& output)
{
    output << "struct Map(Key, Value) {\n"
           << "  entries @0 :List(Entry);\n"
           << "  struct Entry {\n"
           << "    key @0 :Key;\n"
           << "    value @1 :Value;\n"
           << "  }\n"
           << "}\n\n";
}

void CapnpFileGenerator::_flatten_message_fields(const Schema& schema,
                                                  const Message& message,
                                                  std::vector<const Type*>& out_fields)
{
    // Recursively add parent fields first
    if (!message.parent_name.empty())
    {
        auto parent_it = schema.messages.find(message.parent_name);
        if (parent_it != schema.messages.end())
        {
            _flatten_message_fields(schema, parent_it->second, out_fields);
        }
    }

    // Add this message's fields
    for (const auto& field : message.fields)
    {
        out_fields.push_back(&field);
    }
}

bool CapnpFileGenerator::_is_message_type_field(const Type& field)
{
    return (field.get_kind() == Type::Kind::Custom || field.get_kind() == Type::Kind::Enum) &&
           field.get_custom_name() == "MessageType" &&
           field.get_field_name() == "msgType";
}

std::string CapnpFileGenerator::_get_capnp_type_for_field(const Type& field)
{
    return field.get_capnp_type();
}

// ---- Private instance methods ----

void CapnpFileGenerator::_write_header(std::ostringstream& output) const
{
    // Write file ID
    output << IdGenerator::format_id_as_hex(_fileId) << ";\n";

    // Import C++ namespace annotation
    output << "using Cxx = import \"/capnp/c++.capnp\";\n";

    // Capnp-generated types use namespace_name (e.g., curious::message).
    // Wrapper classes use wrapper_namespace_name (e.g., curious::net).
    std::string ns = _schema.namespace_name.empty() ? "curious::message" : _schema.namespace_name;

    // Replace dots with :: for C++ namespace syntax
    std::string::size_type pos = 0;
    while ((pos = ns.find('.', pos)) != std::string::npos)
    {
        ns.replace(pos, 1, "::");
        pos += 2;
    }

    output << "$Cxx.namespace(\"" << ns << "\");\n\n";
}

void CapnpFileGenerator::_write_enum(std::ostringstream& output, const EnumDecl& enum_decl) const
{
    // Use explicit ID if provided, otherwise derive from file ID
    std::uint64_t enum_id = (enum_decl.capnp_id != 0)
                                ? (enum_decl.capnp_id | (1ULL << 63))  // Ensure MSB set
                                : IdGenerator::derive_id(_fileId, enum_decl.name);

    output << "enum " << _to_capnp_identifier(enum_decl.name) << " "
           << IdGenerator::format_id_as_hex(enum_id) << " {\n";

    for (const auto& value : enum_decl.values)
    {
        output << "  " << _to_capnp_identifier(value.name) << " @" << value.value << ";\n";
    }

    output << "}\n\n";
}

void CapnpFileGenerator::_write_all_enums(std::ostringstream& output) const
{
    // Collect and sort enum names for deterministic output
    std::vector<std::string> enum_names;
    enum_names.reserve(_schema.enums.size());

    for (const auto& [name, _] : _schema.enums)
    {
        enum_names.push_back(name);
    }

    std::sort(enum_names.begin(), enum_names.end());

    // Write each enum
    for (const auto& name : enum_names)
    {
        _write_enum(output, _schema.enums.at(name));
    }
}

void CapnpFileGenerator::_write_struct(std::ostringstream& output, const Message& message) const
{
    // Derive struct ID from file ID and message name
    std::uint64_t struct_id = IdGenerator::derive_id(_fileId, message.name);

    output << "struct " << _to_capnp_identifier(message.name) << " "
           << IdGenerator::format_id_as_hex(struct_id) << " {\n";

    // Flatten fields (including inherited ones)
    std::vector<const Type*> all_fields;
    all_fields.reserve(message.fields.size());
    _flatten_message_fields(_schema, message, all_fields);

    // Check if first field is already msgType
    bool has_message_type_first = (!all_fields.empty() && _is_message_type_field(*all_fields.front()));

    std::size_t field_ordinal = 0;

    // Ensure msgType is first field
    if (!has_message_type_first)
    {
        output << "  msgType @" << field_ordinal++ << " : MessageType;\n";
    }

    // Write all fields
    for (const Type* field : all_fields)
    {
        if (field_ordinal == 0 && _is_message_type_field(*field))
        {
            // This is the msgType field at position 0
            output << "  " << _to_capnp_identifier(field->get_field_name())
                   << " @" << field_ordinal++
                   << " : " << _get_capnp_type_for_field(*field) << ";\n";
            continue;
        }

        output << "  " << _to_capnp_identifier(field->get_field_name())
               << " @" << field_ordinal++
               << " : " << _get_capnp_type_for_field(*field) << ";\n";
    }

    output << "}\n\n";
}

void CapnpFileGenerator::_write_all_structs(std::ostringstream& output) const
{
    // Collect and sort message names for deterministic output
    std::vector<std::string> message_names;
    message_names.reserve(_schema.messages.size());

    for (const auto& [name, _] : _schema.messages)
    {
        message_names.push_back(name);
    }

    std::sort(message_names.begin(), message_names.end());

    // Write each struct
    for (const auto& name : message_names)
    {
        _write_struct(output, _schema.messages.at(name));
    }
}

} // namespace curious::dsl::capnpgen

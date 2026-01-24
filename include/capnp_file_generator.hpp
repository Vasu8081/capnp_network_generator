#pragma once

#include <cstdint>
#include <string>

#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates a Cap'n Proto schema file from a parsed DSL Schema.
/// @details Construction performs the generation and writes to disk.
/// If output_path ends with ".capnp", it is used directly; otherwise a file
/// named "network_msg.capnp" is created inside output_path.
class CapnpFileGenerator
{
public:
    /// @brief Create a generator and immediately write the file to disk.
    /// @param schema Parsed DSL schema.
    /// @param output_path Destination path or directory.
    CapnpFileGenerator(const Schema& schema, const std::string& output_path);

private:
    /// @brief Reference to the schema being generated.
    const Schema& _schema;

    /// @brief Resolved output file path.
    std::string _outputPath;

    /// @brief Unique file ID for this Cap'n Proto schema.
    std::uint64_t _fileId;

    /// @brief Resolve the output path (handle directory vs file).
    /// @param path The user-provided path.
    /// @return The resolved output file path.
    static std::string _resolve_output_path(const std::string& path);

    /// @brief Initialize file ID by extracting from existing file or generating new.
    /// @param resolved_path The resolved output file path.
    /// @return Existing file ID if available, otherwise a new random ID.
    static std::uint64_t _initialize_file_id(const std::string& resolved_path);

    /// @brief Write the file header (ID and namespace declaration).
    /// @param output The output stream to write to.
    void _write_header(std::ostringstream& output) const;

    /// @brief Convert an identifier to Cap'n Proto format (replace spaces with underscores).
    /// @param identifier The identifier to convert.
    /// @return The Cap'n Proto-safe identifier.
    static std::string _to_capnp_identifier(const std::string& identifier);

    /// @brief Write a single enum declaration.
    /// @param output The output stream to write to.
    /// @param enum_decl The enum to write.
    void _write_enum(std::ostringstream& output, const EnumDecl& enum_decl) const;

    /// @brief Write all enum declarations in deterministic order.
    /// @param output The output stream to write to.
    void _write_all_enums(std::ostringstream& output) const;

    /// @brief Write the generic Map template definition.
    /// @param output The output stream to write to.
    static void _write_map_template(std::ostringstream& output);

    /// @brief Flatten message fields including inherited fields from parent.
    /// @param schema The schema containing all messages.
    /// @param message The message to flatten.
    /// @param out_fields Output vector to append fields to.
    static void _flatten_message_fields(const Schema& schema, const Message& message, std::vector<const Type*>& out_fields);

    /// @brief Check if a field is the special "msgType" field.
    /// @param field The field to check.
    /// @return True if this is a MessageType msgType field.
    static bool _is_message_type_field(const Type& field);

    /// @brief Get the Cap'n Proto type string for a field.
    /// @param field The field type.
    /// @return The Cap'n Proto type string.
    static std::string _get_capnp_type_for_field(const Type& field);

    /// @brief Write a single struct (message) declaration.
    /// @param output The output stream to write to.
    /// @param message The message to write.
    void _write_struct(std::ostringstream& output, const Message& message) const;

    /// @brief Write all struct declarations in deterministic order.
    /// @param output The output stream to write to.
    void _write_all_structs(std::ostringstream& output) const;
};

} // namespace curious::dsl::capnpgen

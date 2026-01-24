#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "type.hpp"

namespace curious::dsl::capnpgen
{

// Forward declaration
class Lexer;

/// @brief Simple message container parsed from the DSL.
struct Message
{
    /// @brief Message numeric id (used for Cap'n Proto @id).
    std::uint64_t id{0};

    /// @brief Message name.
    std::string name;

    /// @brief Optional base/parent message name (empty if none).
    std::string parent_name;

    /// @brief Parsed field types (in declaration order).
    std::vector<Type> fields;

    /// @brief Return the Cap'n Proto-style hex id string (e.g., "@0x0000000000000001").
    /// @return Formatted ID string.
    std::string get_capnp_id_string() const;

    /// @brief Add a field from a single line (e.g., "vector<int> nums;").
    /// @param line The field declaration line.
    void add_field_from_line(std::string_view line);
};

/// @brief Enum value with a resolved integral value.
struct EnumValue
{
    /// @brief Enum symbolic name.
    std::string name;

    /// @brief Resolved integer value.
    std::int64_t value{0};
};

/// @brief Enum declaration with all values and optional Cap'n Proto id.
struct EnumDecl
{
    /// @brief Enum name.
    std::string name;

    /// @brief Values in declaration order.
    std::vector<EnumValue> values;

    /// @brief Optional Cap'n Proto id (0 if not provided).
    std::uint64_t capnp_id{0};
};

/// @brief Full schema: namespace, messages, and enums; supports parsing from a file.
class Schema
{
public:
    /// @brief Default constructor.
    Schema();

    /// @brief Destructor.
    ~Schema();

    /// @brief Parsed namespace for capnp types (e.g., "curious.message").
    std::string namespace_name;

    /// @brief Parsed namespace for wrapper classes (e.g., "curious.net").
    /// If empty, defaults to namespace_name.
    std::string wrapper_namespace_name;

    /// @brief Messages by name.
    std::unordered_map<std::string, Message> messages;

    /// @brief Enums by name.
    std::unordered_map<std::string, EnumDecl> enums;

    /// @brief Parse and populate this schema from a DSL file path.
    /// @param file_path The path to the DSL file.
    /// @throws std::runtime_error on errors.
    void parse_from_file(const std::string& file_path);

private:
    /// @brief Internal lexer for tokenizing input.
    std::unique_ptr<Lexer> _lexer;

    /// @brief Order in which messages were parsed (for deterministic output).
    std::vector<std::string> _messageOrder;

    /// @brief Throw a parsing error with the given message.
    /// @param message The error message.
    [[noreturn]] void _throw_parse_error(const std::string& message) const;

    /// @brief Read a braced block {...} and return its contents.
    /// @return The content inside the braces.
    std::string _read_braced_block();

    /// @brief Parse a namespace declaration.
    void _parse_namespace();

    /// @brief Parse a wrapper_namespace declaration.
    void _parse_wrapper_namespace();

    /// @brief Parse an enum declaration.
    void _parse_enum();

    /// @brief Parse a message declaration.
    void _parse_message();

    /// @brief Ensure the MessageType enum exists and is properly populated.
    void _ensure_message_type_enum();

    /// @brief Normalize a field line by removing unnecessary keywords.
    /// @param line The raw field line.
    /// @return The normalized field line.
    static std::string _normalize_field_line(std::string line);

    /// @brief Add a field line to a message, skipping invalid lines.
    /// @param message The message to add the field to.
    /// @param raw_line The raw field line.
    static void _add_field_line_to_message(Message& message, std::string raw_line);
};

} // namespace curious::dsl::capnpgen

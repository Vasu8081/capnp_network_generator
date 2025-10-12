#include "schema.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "lexer.hpp"
#include "string_utils.hpp"

namespace curious::dsl::capnpgen
{

// ---- Message methods ----

std::string Message::get_capnp_id_string() const
{
    std::ostringstream oss;
    oss << "@0x"
        << std::hex << std::nouppercase
        << std::setfill('0') << std::setw(16)
        << id;
    return oss.str();
}

void Message::add_field_from_line(std::string_view line)
{
    fields.emplace_back(Type::parse_from_line(line));
}

// ---- Schema public methods ----

Schema::Schema() = default;

Schema::~Schema() = default;

void Schema::parse_from_file(const std::string& file_path)
{
    // Read and preprocess file
    std::string file_content = string_utils::read_file(file_path);
    std::string content_without_comments = string_utils::strip_comments(file_content);

    // Initialize lexer
    _lexer = std::make_unique<Lexer>(std::move(content_without_comments));

    // Clear previous state
    namespace_name.clear();
    messages.clear();
    enums.clear();
    _messageOrder.clear();

    // Parse top-level declarations
    while (true)
    {
        auto token = _lexer->peek_token();
        if (!token)
        {
            break; // End of file
        }

        if (token->is_keyword("namespace"))
        {
            _parse_namespace();
        }
        else if (token->is_keyword("enum"))
        {
            _parse_enum();
        }
        else if (token->is_keyword("message"))
        {
            _parse_message();
        }
        else
        {
            _throw_parse_error("Expected 'namespace', 'enum', or 'message'");
        }
    }

    // Ensure MessageType enum is properly populated
    _ensure_message_type_enum();
}

// ---- Schema private methods ----

[[noreturn]] void Schema::_throw_parse_error(const std::string& message) const
{
    throw std::runtime_error("Schema parse error: " + message);
}

std::string Schema::_read_braced_block()
{
    auto opening_brace = _lexer->next_token();
    if (!opening_brace.is_keyword("{"))
    {
        _throw_parse_error("Expected '{'");
    }

    std::string content;
    int brace_depth = 1;

    while (true)
    {
        auto token = _lexer->next_token();
        if (token.is_eof)
        {
            _throw_parse_error("Unexpected EOF inside '{...}'");
        }

        if (token.is_keyword("{"))
        {
            ++brace_depth;
            content += "{ ";
        }
        else if (token.is_keyword("}"))
        {
            --brace_depth;
            if (brace_depth == 0)
            {
                break; // End of block
            }
            content += "} ";
        }
        else
        {
            content += token.text;
            content += " ";
        }
    }

    return content;
}

void Schema::_parse_namespace()
{
    _lexer->next_token(); // Consume 'namespace'

    auto name_token = _lexer->next_token();
    if (!name_token.is_identifier())
    {
        _throw_parse_error("Expected identifier after 'namespace'");
    }

    std::string ns = name_token.text;

    // Handle dotted namespace (e.g., my.company.product)
    while (true)
    {
        auto dot_token = _lexer->peek_token();
        if (!dot_token || !dot_token->is_keyword("."))
        {
            break;
        }

        _lexer->next_token(); // Consume '.'

        auto part_token = _lexer->next_token();
        if (!part_token.is_identifier())
        {
            _throw_parse_error("Expected identifier after '.'");
        }

        ns += ".";
        ns += part_token.text;
    }

    auto semicolon = _lexer->next_token();
    if (!semicolon.is_keyword(";"))
    {
        _throw_parse_error("Expected ';' after namespace");
    }

    namespace_name = std::move(ns);
}

void Schema::_parse_enum()
{
    _lexer->next_token(); // Consume 'enum'

    auto name_token = _lexer->next_token();
    if (!name_token.is_identifier())
    {
        _throw_parse_error("Expected enum name");
    }

    EnumDecl enum_decl;
    enum_decl.name = name_token.text;

    // Check for optional @id
    auto next_token = _lexer->peek_token();
    if (next_token && next_token->is_keyword("@"))
    {
        _lexer->next_token(); // Consume '@'

        auto id_token = _lexer->next_token();
        if (!id_token.is_number())
        {
            _throw_parse_error("Expected numeric enum id after '@' (e.g., 0x1234)");
        }

        // Parse the number (hex or decimal)
        const std::string& id_str = id_token.text;
        std::uint64_t id_value = 0;

        if (id_str.size() > 2 && id_str[0] == '0' && (id_str[1] == 'x' || id_str[1] == 'X'))
        {
            id_value = std::stoull(id_str.substr(2), nullptr, 16);
        }
        else
        {
            id_value = std::stoull(id_str, nullptr, 10);
        }

        enum_decl.capnp_id = id_value;
    }

    // Parse enum body
    std::string body = _read_braced_block();
    auto items = string_utils::split_respecting_nesting(body, ',');

    std::int64_t next_value = 0;

    for (const auto& item : items)
    {
        if (item.empty())
        {
            continue; // Skip empty items (trailing commas)
        }

        // Check if item has explicit value (NAME | VALUE)
        std::size_t bar_pos = item.find('|');

        if (bar_pos == std::string::npos)
        {
            // No explicit value
            EnumValue enum_value;
            enum_value.name = string_utils::trim(item);
            enum_value.value = next_value++;
            enum_decl.values.push_back(std::move(enum_value));
        }
        else
        {
            // Has explicit value
            std::string name_part = string_utils::trim(item.substr(0, bar_pos));
            std::string value_part = string_utils::trim(item.substr(bar_pos + 1));

            if (name_part.empty() || value_part.empty())
            {
                _throw_parse_error("Malformed enum item near '|'");
            }

            EnumValue enum_value;
            enum_value.name = std::move(name_part);

            try
            {
                long long parsed = std::stoll(value_part);
                enum_value.value = static_cast<std::int64_t>(parsed);
                next_value = enum_value.value + 1; // Continue from explicit value
            }
            catch (...)
            {
                _throw_parse_error("Enum value must be an integer: '" + value_part + "'");
            }

            enum_decl.values.push_back(std::move(enum_value));
        }
    }

    // Optional trailing semicolon
    auto trailing_semicolon = _lexer->peek_token();
    if (trailing_semicolon && trailing_semicolon->is_keyword(";"))
    {
        _lexer->next_token();
    }

    enums[enum_decl.name] = std::move(enum_decl);
}

void Schema::_parse_message()
{
    _lexer->next_token(); // Consume 'message'

    auto name_token = _lexer->next_token();
    if (!name_token.is_identifier())
    {
        _throw_parse_error("Expected message name");
    }

    Message message;
    message.name = name_token.text;

    // Parse message ID: (id)
    auto open_paren = _lexer->next_token();
    if (!open_paren.is_keyword("("))
    {
        _throw_parse_error("Expected '(' after message name");
    }

    auto id_token = _lexer->next_token();
    if (!id_token.is_number())
    {
        _throw_parse_error("Expected numeric message id");
    }

    message.id = std::stoull(id_token.text);

    auto close_paren = _lexer->next_token();
    if (!close_paren.is_keyword(")"))
    {
        _throw_parse_error("Expected ')'");
    }

    // Check for optional 'extends'
    auto extends_token = _lexer->peek_token();
    if (extends_token && extends_token->is_keyword("extends"))
    {
        _lexer->next_token(); // Consume 'extends'

        auto base_token = _lexer->next_token();
        if (!base_token.is_identifier())
        {
            _throw_parse_error("Expected base message name after 'extends'");
        }

        message.parent_name = base_token.text;
    }

    // Parse message body
    std::string body = _read_braced_block();
    auto field_lines = string_utils::split_respecting_nesting(body, ';');

    for (auto& field_line : field_lines)
    {
        if (!field_line.empty())
        {
            _add_field_line_to_message(message, field_line + ";");
        }
    }

    _messageOrder.push_back(message.name);
    messages[message.name] = std::move(message);
}

void Schema::_ensure_message_type_enum()
{
    EnumDecl& message_type_enum = enums["MessageType"];
    message_type_enum.name = "MessageType";
    message_type_enum.capnp_id = 0x0;

    // Add 'undefined' if enum is empty
    if (message_type_enum.values.empty())
    {
        message_type_enum.values.push_back({"undefined", 0});
    }

    // Track existing names
    std::unordered_set<std::string> existing_names;
    existing_names.reserve(message_type_enum.values.size() * 2 + _messageOrder.size());

    for (const auto& value : message_type_enum.values)
    {
        existing_names.insert(value.name);
    }

    // Append missing message entries in deterministic order
    for (const auto& message_name : _messageOrder)
    {
        if (existing_names.find(message_name) != existing_names.end())
        {
            continue; // Already exists
        }

        const auto& msg = messages.at(message_name);

        // Convert first character to lowercase (enum value naming convention)
        std::string enum_value_name = msg.name;
        if (!enum_value_name.empty() && std::isupper(static_cast<unsigned char>(enum_value_name[0])))
        {
            enum_value_name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(enum_value_name[0])));
        }

        if (existing_names.find(enum_value_name) != existing_names.end())
        {
            continue; // Avoid duplicates
        }

        EnumValue enum_value;
        enum_value.name = enum_value_name;
        enum_value.value = static_cast<std::int64_t>(msg.id);

        message_type_enum.values.push_back(std::move(enum_value));
        existing_names.insert(enum_value_name);
    }
}

std::string Schema::_normalize_field_line(std::string line)
{
    line = string_utils::trim(line);

    if (line.empty())
    {
        return line;
    }

    // Remove "enum " prefix if present (allows "enum Status statusCode;" inside messages)
    if (string_utils::starts_with_keyword(line, "enum"))
    {
        std::size_t pos = 0;

        // Skip "enum"
        while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos])))
        {
            ++pos;
        }

        // Skip whitespace after "enum"
        while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
        {
            ++pos;
        }

        line = line.substr(pos);
    }

    return line;
}

void Schema::_add_field_line_to_message(Message& message, std::string raw_line)
{
    std::string normalized_line = _normalize_field_line(std::move(raw_line));

    if (normalized_line.empty())
    {
        return; // Skip empty lines
    }

    // Skip lines that start with keywords we don't want as fields
    if (string_utils::starts_with_keyword(normalized_line, "message") ||
        string_utils::starts_with_keyword(normalized_line, "enum") ||
        string_utils::starts_with_keyword(normalized_line, "extends"))
    {
        return;
    }

    message.add_field_from_line(normalized_line);
}

} // namespace curious::dsl::capnpgen

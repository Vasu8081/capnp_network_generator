#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "type.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Simple message container parsed from the DSL.
struct Message
{
    /// @brief Message numeric id (used for Cap’n Proto @id).
    std::uint64_t _id { 0 };

    /// @brief Message name.
    std::string _name;

    /// @brief Optional base/parent message name (empty if none).
    std::string _parent;

    /// @brief Parsed field types (in declaration order).
    std::vector<Type> _fields;

    /// @brief Return the Cap’n Proto-style hex id string (e.g., "@0x0000000000000001").
    std::string get_capnp_id() const;

    /// @brief Add a field from a single line (e.g., "vector<int> nums;").
    void add(std::string_view line);
};

/// @brief Enum value with a resolved integral value.
struct EnumValue
{
    /// @brief Enum symbolic name.
    std::string _name;

    /// @brief Resolved integer value.
    std::int64_t _value { 0 };
};

/// @brief Enum declaration with all values and optional Cap’n Proto id.
struct EnumDecl
{
    /// @brief Enum name.
    std::string _name;

    /// @brief Values in declaration order.
    std::vector<EnumValue> _values;

    /// @brief Optional Cap’n Proto id (0 if not provided).
    std::uint64_t _capnpId { 0 };
};

/// @brief Full schema: namespace, messages, and enums; supports parsing from a file.
class Schema
{
public:
    Schema();

    ~Schema();

    /// @brief Parsed namespace (e.g., "my.company.product").
    std::string _namespace;

    /// @brief Messages by name.
    std::unordered_map<std::string, Message> _messages;

    /// @brief Enums by name.
    std::unordered_map<std::string, EnumDecl> _enums;

    /// @brief Parse and populate this schema from a DSL file path.
    /// Throws std::runtime_error on errors.
    void parse(const std::string& file_path);

private:
    // ---- Internal state (private members must be camelBack + '_' prefix) ----
    struct Lexer; // forward-declared, defined in .cpp
    std::unique_ptr<Lexer> _lex;
    std::vector<std::string> _messageOrder;

    // ---- Helpers (private methods use CamelCase) ----
    [[noreturn]] void Fail(const std::string& msg) const;

    std::string ReadBracedBlock();
    void ParseNamespace();
    void ParseEnum();
    void ParseMessage();

    void EnsureMessageTypeEnum();

    // Normalization helpers
    static bool StartsWithKw(const std::string& s, const char* kw);
    static std::string NormalizeFieldLine(std::string line);
    static void AddFieldLine(Message& m, std::string raw);
};

} // namespace curious::dsl::capnpgen

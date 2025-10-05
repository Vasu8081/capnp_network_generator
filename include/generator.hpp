#pragma once

#include <cstdint>
#include <string>

#include "schema.hpp"

namespace curious::dsl::capnpgen
{

/// @brief Generates a Cap’n Proto schema file from a parsed DSL Schema.
/// @details Construction performs the generation and writes to disk.
/// If @p output_path ends with ".capnp", it is used directly; otherwise a file
/// named "network_msg.capnp" is created inside @p output_path.
class CapnpFileGenerator
{
public:
    /// @brief Create a generator and immediately write the file to disk.
    /// @param schema Parsed DSL schema.
    /// @param output_path Destination path or directory.
    CapnpFileGenerator(const Schema& schema, const std::string& output_path);

private:
    // ---- Data (private members: camelBack + '_' prefix) ----
    const Schema& _schema;
    std::string _outPath;
    std::uint64_t _fileId;

    // ---- Helpers (private methods: CamelCase) ----

    // Path handling
    static std::string ResolveOutPath(const std::string& p);

    // Header & IDs
    void WriteHeader(std::ostringstream& o) const;
    static std::uint64_t RandomCapnpId();
    static std::uint64_t Fnv1a64(std::string_view sv);
    static std::uint64_t DeriveId(std::uint64_t parent_id, std::string_view name);
    static std::string FmtId(std::uint64_t x);

    // Identifiers
    static std::string ToCapnpIdent(const std::string& s);

    // Enums
    void WriteOneEnum(std::ostringstream& o, const EnumDecl& e) const;
    void WriteEnums(std::ostringstream& o) const;

    // Map template
    void WriteMapTemplate(std::ostringstream& o) const;

    // Structs
    static void FlattenMessageFields(const Schema& s, const Message& m, std::vector<const Type*>& out);
    static bool IsMsgTypeField(const Type& t);
    static std::string CapnpTypeOf(const Type& t);
    void WriteOneStruct(std::ostringstream& o, const Message& m) const;
    void WriteStructs(std::ostringstream& o) const;
};

} // namespace curious::dsl::capnpgen

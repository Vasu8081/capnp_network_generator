#include "generator.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <unordered_set>

namespace curious::dsl::capnpgen
{

// ---- Construction (public) ----

CapnpFileGenerator::CapnpFileGenerator(const Schema& schema, const std::string& output_path)
    : _schema(schema),
      _outPath(ResolveOutPath(output_path)),
      _fileId(RandomCapnpId())
{
    std::ofstream ofs(_outPath, std::ios::binary);
    if (!ofs)
    {
        throw std::runtime_error("Failed to open output: " + _outPath);
    }

    std::ostringstream body;
    WriteHeader(body);
    WriteEnums(body);
    WriteMapTemplate(body);
    WriteStructs(body);

    ofs << body.str();
}

// ---- Path handling ----

std::string CapnpFileGenerator::ResolveOutPath(const std::string& p)
{
    namespace fs = std::filesystem;
    fs::path path(p);
    if (path.extension() == ".capnp")
    {
        if (path.has_parent_path())
        {
            fs::create_directories(path.parent_path());
        }
        return path.string();
    }
    fs::create_directories(path);
    return (path / "network_msg.capnp").string();
}

// ---- Header & IDs ----

void CapnpFileGenerator::WriteHeader(std::ostringstream& o) const
{
    o << FmtId(_fileId) << ";\n";
    o << "using Cxx = import \"/capnp/c++.capnp\";\n";

    const std::string ns =
        _schema._namespace.empty() ? "curious::message" : _schema._namespace;

    o << "$Cxx.namespace(\"" << ns << "\");\n\n";
}

std::uint64_t CapnpFileGenerator::RandomCapnpId()
{
    std::random_device rd;
    std::uint64_t hi = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
    std::uint64_t lo = (static_cast<std::uint64_t>(rd()) << 32) ^ rd();
    std::uint64_t x = (hi << 32) ^ lo;
    return x | (1ULL << 63); // ensure MSB=1
}

std::uint64_t CapnpFileGenerator::Fnv1a64(std::string_view sv)
{
    const std::uint64_t OFF = 0xcbf29ce484222325ULL;
    const std::uint64_t PRM = 0x100000001b3ULL;
    std::uint64_t h = OFF;
    for (unsigned char c : sv)
    {
        h ^= c;
        h *= PRM;
    }
    return h;
}

std::uint64_t CapnpFileGenerator::DeriveId(std::uint64_t parent_id, std::string_view name)
{
    char buf[8];
    for (int i = 0; i < 8; ++i)
    {
        buf[i] = static_cast<char>((parent_id >> (8 * (7 - i))) & 0xFF);
    }
    std::string seed(buf, buf + 8);
    seed.append(name);
    return Fnv1a64(seed) | (1ULL << 63);
}

std::string CapnpFileGenerator::FmtId(std::uint64_t x)
{
    std::ostringstream oss;
    oss << "@0x" << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << x;
    return oss.str();
}

// ---- Identifiers ----

std::string CapnpFileGenerator::ToCapnpIdent(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        out.push_back(std::isspace(static_cast<unsigned char>(c)) != 0 ? '_' : c);
    }
    return out;
}

// ---- Enums ----

void CapnpFileGenerator::WriteOneEnum(std::ostringstream& o, const EnumDecl& e) const
{
    // Use explicit id if provided; otherwise derive from file id + name. Force MSB=1.
    std::uint64_t enum_id =
        (e._capnpId != 0) ? (e._capnpId | (1ULL << 63)) : DeriveId(_fileId, e._name);

    o << "enum " << ToCapnpIdent(e._name) << " " << FmtId(enum_id) << " {\n";
    for (const auto& v : e._values)
    {
        o << "  " << ToCapnpIdent(v._name) << " @" << v._value << ";\n";
    }
    o << "}\n\n";
}

void CapnpFileGenerator::WriteEnums(std::ostringstream& o) const
{
    // Deterministic order
    std::vector<std::string> names;
    names.reserve(_schema._enums.size());
    for (const auto& kv : _schema._enums) names.push_back(kv.first);
    std::sort(names.begin(), names.end());

    for (const auto& name : names)
    {
        WriteOneEnum(o, _schema._enums.at(name));
    }
}

// ---- Map template ----

void CapnpFileGenerator::WriteMapTemplate(std::ostringstream& o) const
{
    o << "struct Map(Key, Value) {\n"
      << "  entries @0 :List(Entry);\n"
      << "  struct Entry {\n"
      << "    key @0 :Key;\n"
      << "    value @1 :Value;\n"
      << "  }\n"
      << "}\n\n";
}

// ---- Struct helpers ----

void CapnpFileGenerator::FlattenMessageFields(const Schema& s, const Message& m, std::vector<const Type*>& out)
{
    if (!m._parent.empty())
    {
        auto it = s._messages.find(m._parent);
        if (it != s._messages.end())
        {
            FlattenMessageFields(s, it->second, out);
        }
    }
    for (const auto& f : m._fields) out.push_back(&f);
}

bool CapnpFileGenerator::IsMsgTypeField(const Type& t)
{
    return (t.kind() == Type::Kind::Custom || t.kind() == Type::Kind::Enum) &&
           t.custom_name() == "MessageType" &&
           t.field_name() == "msgType";
}

std::string CapnpFileGenerator::CapnpTypeOf(const Type& t)
{
    return t.capnp_type();
}

// ---- Structs ----

void CapnpFileGenerator::WriteOneStruct(std::ostringstream& o, const Message& m) const
{
    // Derive a valid Cap’n Proto ID from the file ID + message name
    const std::uint64_t struct_id = DeriveId(_fileId, m._name);
    o << "struct " << ToCapnpIdent(m._name) << " " << FmtId(struct_id) << " {\n";

    std::vector<const Type*> flat;
    flat.reserve(m._fields.size());
    FlattenMessageFields(_schema, m, flat);

    // Ensure msgType first
    bool has_msg_type_first = (!flat.empty() && IsMsgTypeField(*flat.front()));
    std::size_t ord = 0;
    if (!has_msg_type_first)
    {
        o << "  msgType @" << ord++ << " : MessageType;\n";
    }
    for (const Type* f : flat)
    {
        if (ord == 0 && IsMsgTypeField(*f))
        {
            o << "  " << ToCapnpIdent(f->field_name()) << " @" << ord++
              << " : " << CapnpTypeOf(*f) << ";\n";
            continue;
        }
        o << "  " << ToCapnpIdent(f->field_name()) << " @" << ord++
          << " : " << CapnpTypeOf(*f) << ";\n";
    }
    o << "}\n\n";
}

void CapnpFileGenerator::WriteStructs(std::ostringstream& o) const
{
    // Deterministic order
    std::vector<std::string> names;
    names.reserve(_schema._messages.size());
    for (const auto& kv : _schema._messages) names.push_back(kv.first);
    std::sort(names.begin(), names.end());
    for (const auto& n : names) WriteOneStruct(o, _schema._messages.at(n));
}

} // namespace curious::dsl::capnpgen

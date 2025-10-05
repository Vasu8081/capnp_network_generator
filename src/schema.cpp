#include "schema.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace curious::dsl::capnpgen
{
namespace
{
// ---- Local helpers (internal-only; not in public API) ----

std::string Trim(std::string s)
{
    std::size_t a = 0;
    std::size_t b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a])) != 0) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1])) != 0) --b;
    return s.substr(a, b - a);
}

std::string ReadFile(const std::string& path)
{
    std::ifstream ifs(path);
    if (!ifs)
    {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// Remove // and /*...*/ comments
std::string StripComments(const std::string& in)
{
    std::string out;
    out.reserve(in.size());
    enum State { N, SLASH, LINE, BLOCK, STAR } st = N;
    for (char c : in)
    {
        switch (st)
        {
            case N:
                if (c == '/')
                {
                    st = SLASH;
                    out.push_back(' ');
                }
                else
                {
                    out.push_back(c);
                }
                break;
            case SLASH:
                if (c == '/')
                {
                    st = LINE;
                    out.back() = ' ';
                }
                else if (c == '*')
                {
                    st = BLOCK;
                    out.back() = ' ';
                }
                else
                {
                    out.back() = '/';
                    out.push_back(c);
                    st = N;
                }
                break;
            case LINE:
                if (c == '\n')
                {
                    out.push_back('\n');
                    st = N;
                }
                break;
            case BLOCK:
                if (c == '*')
                {
                    st = STAR;
                }
                break;
            case STAR:
                if (c == '/')
                {
                    st = N;
                }
                else if (c != '*')
                {
                    st = BLOCK;
                }
                break;
        }
    }
    return out;
}

// Split top-level by a delimiter (ignores braces/brackets/angles nesting)
std::vector<std::string> SplitTopLevel(const std::string& s, char delim)
{
    std::vector<std::string> out;
    std::string cur;
    int a = 0, b = 0, c = 0; // <>, (), {}
    for (char ch : s)
    {
        if (ch == '<') ++a;
        else if (ch == '>') --a;
        else if (ch == '(') ++b;
        else if (ch == ')') --b;
        else if (ch == '{') ++c;
        else if (ch == '}') --c;

        if (ch == delim && a == 0 && b == 0 && c == 0)
        {
            auto piece = Trim(cur);
            out.push_back(piece);
            cur.clear();
        }
        else
        {
            cur.push_back(ch);
        }
    }
    if (!Trim(cur).empty())
    {
        out.push_back(Trim(cur));
    }
    return out;
}

} // namespace

// ---- Message ----

std::string Message::get_capnp_id() const
{
    std::ostringstream oss;
    oss << "@0x"
        << std::hex << std::nouppercase
        << std::setfill('0') << std::setw(16)
        << static_cast<std::uint64_t>(_id);
    return oss.str();
}

void Message::add(std::string_view line)
{
    _fields.emplace_back(line);
}

// ---- Schema internals ----
struct Schema::Lexer
{
    struct Token
    {
        std::string text;
        bool eof { false };

        bool is(const char* s) const { return !eof && text == s; }

        bool is_ident() const
        {
            if (eof || text.empty()) return false;
            if (!(std::isalpha(static_cast<unsigned char>(text[0])) != 0 || text[0] == '_')) return false;
            for (std::size_t i = 1; i < text.size(); ++i)
            {
                char c = text[i];
                if (!(std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == ':')) return false;
            }
            return true;
        }

        bool is_number() const
        {
            if (eof || text.empty()) return false;
            std::size_t k = 0;
            if (text[k] == '+' || text[k] == '-') ++k;
            if (k >= text.size()) return false;

            // hex: 0x....
            if (k + 2 <= text.size() && text[k] == '0' && (text[k + 1] == 'x' || text[k + 1] == 'X'))
            {
                if (k + 2 == text.size()) return false; // "0x" only
                for (std::size_t i = k + 2; i < text.size(); ++i)
                {
                    if (!std::isxdigit(static_cast<unsigned char>(text[i]))) return false;
                }
                return true;
            }

            // decimal
            for (; k < text.size(); ++k)
            {
                if (!std::isdigit(static_cast<unsigned char>(text[k]))) return false;
            }
            return true;
        }
    };

    std::string s;
    std::size_t i { 0 };

    explicit Lexer(std::string src) : s(std::move(src)), i(0) {}

    static bool is_sym(char c)
    {
        return c == '{' || c == '}' || c == '(' || c == ')' || c == '/' || c == '*' || c == ';' ||
               c == ',' || c == '<' || c == '>' || c == '.' || c == '|' || c == '@';
    }

    void ws()
    {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) ++i;
    }

    std::optional<Token> peek()
    {
        std::size_t sv = i;
        auto t = next();
        i = sv;
        if (t.eof) return std::nullopt;
        return t;
    }

    Token next()
    {
        ws();
        if (i >= s.size()) return Token{{}, true};
        char c = s[i];
        if (is_sym(c))
        {
            ++i;
            return Token{std::string(1, c), false};
        }
        if (std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_')
        {
            std::size_t j = i + 1;
            while (j < s.size())
            {
                char d = s[j];
                if (std::isalnum(static_cast<unsigned char>(d)) != 0 || d == '_' || d == ':') ++j;
                else break;
            }
            std::string t = s.substr(i, j - i);
            i = j;
            return Token{t, false};
        }
        if (std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '+' || c == '-')
        {
            std::size_t j = i + 1;

            // HEX: 0x...
            if (c == '0' && j < s.size() && (s[j] == 'x' || s[j] == 'X'))
            {
                j += 1; // past 'x'
                ++j;    // move from '0' to after 'x'
                while (j < s.size() && std::isxdigit(static_cast<unsigned char>(s[j])) != 0) ++j;
                std::string t = s.substr(i, j - i);
                i = j;
                return Token{t, false};
            }

            // DECIMAL
            while (j < s.size() && std::isdigit(static_cast<unsigned char>(s[j])) != 0) ++j;
            std::string t = s.substr(i, j - i);
            i = j;
            return Token{t, false};
        }
        // default single char
        ++i;
        return Token{std::string(1, c), false};
    }
};

Schema::Schema() = default;
Schema::~Schema() = default;

// ---- Schema: public API ----

void Schema::parse(const std::string& file_path)
{
    auto text = StripComments(ReadFile(file_path));
    _lex = std::make_unique<Lexer>(std::move(text));
    _namespace.clear();
    _messages.clear();
    _enums.clear();
    _messageOrder.clear();

    while (true)
    {
        auto tk = _lex->peek();
        if (!tk) break;

        if (tk->is("namespace"))      ParseNamespace();
        else if (tk->is("enum"))      ParseEnum();
        else if (tk->is("message"))   ParseMessage();
        else                          Fail("expected 'namespace', 'enum', or 'message'");
    }

    // synthesize/augment MessageType enum using parsed messages
    EnsureMessageTypeEnum();
}

// ---- Schema: private helpers ----

[[noreturn]] void Schema::Fail(const std::string& msg) const
{
    throw std::runtime_error("Schema parse error: " + msg);
}

std::string Schema::ReadBracedBlock()
{
    auto t = _lex->next();
    if (!t.is("{")) Fail("expected '{'");
    int depth = 1;
    std::string buf;
    while (true)
    {
        auto tk = _lex->next();
        if (tk.eof) Fail("unexpected EOF inside '{...}'");
        if (tk.is("{"))
        {
            ++depth;
            buf.push_back('{');
        }
        else if (tk.is("}"))
        {
            --depth;
            if (depth == 0) break;
            buf.push_back('}');
        }
        else
        {
            buf += tk.text;
            buf.push_back(' ');
        }
    }
    return buf;
}

void Schema::ParseNamespace()
{
    _lex->next(); // 'namespace'
    auto t = _lex->next();
    if (!t.is_ident()) Fail("expected identifier after 'namespace'");
    std::string ns = t.text;
    while (true)
    {
        auto dot = _lex->peek();
        if (!dot || !dot->is(".")) break;
        _lex->next();
        auto part = _lex->next();
        if (!part.is_ident()) Fail("expected identifier after '.'");
        ns += "." + part.text;
    }
    auto semi = _lex->next();
    if (!semi.is(";")) Fail("expected ';' after namespace");
    _namespace = std::move(ns);
}

void Schema::ParseEnum()
{
    // enum Name @0x1234 { A | 10, B | 20, C, }
    _lex->next(); // 'enum'
    auto nm = _lex->next();
    if (!nm.is_ident()) Fail("expected enum name");
    EnumDecl e;
    e._name = nm.text;

    if (auto pk = _lex->peek(); pk && pk->is("@"))
    {
        _lex->next(); // consume '@'
        auto idTok = _lex->next();
        if (!idTok.is_number()) Fail("expected numeric enum id after '@' (e.g., 0x1234)");
        std::uint64_t val = 0;
        const std::string& s = idTok.text;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
            val = std::stoull(s.substr(2), nullptr, 16);
        }
        else
        {
            val = std::stoull(s, nullptr, 10);
        }
        e._capnpId = val;
    }

    auto body = ReadBracedBlock();
    auto parts = SplitTopLevel(body, ','); // items may be "IDENT | NUMBER" or just "IDENT"
    std::int64_t next_val = 0;

    for (auto item : parts)
    {
        item = Trim(item);
        if (item.empty()) continue; // tolerate trailing comma

        // split by '|' at top-level
        std::size_t bar = item.find('|');
        if (bar == std::string::npos)
        {
            EnumValue v;
            v._name = Trim(item);
            v._value = next_val++;
            e._values.push_back(std::move(v));
        }
        else
        {
            std::string left = Trim(item.substr(0, bar));
            std::string right = Trim(item.substr(bar + 1));
            if (left.empty() || right.empty()) Fail("malformed enum item near '|'");
            EnumValue v;
            v._name = std::move(left);
            try
            {
                long long parsed = std::stoll(right);
                v._value = static_cast<std::int64_t>(parsed);
                next_val = v._value + 1; // continue counting from explicit value
            }
            catch (...)
            {
                Fail("enum value must be an integer: '" + right + "'");
            }
            e._values.push_back(std::move(v));
        }
    }

    // optional trailing ';'
    if (auto tk = _lex->peek(); tk && tk->is(";")) _lex->next();

    _enums[e._name] = std::move(e);
}

void Schema::ParseMessage()
{
    // message Name (id) [extends Base] { fields... }
    _lex->next(); // 'message'
    auto nameTok = _lex->next();
    if (!nameTok.is_ident()) Fail("expected message name");

    Message m;
    m._name = nameTok.text;

    // read (id)  ------ inlined logic ------
    {
        auto t = _lex->next();
        if (!t.is("(")) Fail("expected '(' after message name");
        auto num = _lex->next();
        if (!num.is_number()) Fail("expected numeric message id");
        std::uint64_t id = std::stoull(num.text);
        auto close = _lex->next();
        if (!close.is(")")) Fail("expected ')'");
        m._id = id;
    }
    // --------------------------------------

    if (auto pk = _lex->peek(); pk && pk->is("extends"))
    {
        _lex->next();
        auto baseTok = _lex->next();
        if (!baseTok.is_ident()) Fail("expected base after 'extends'");
        m._parent = baseTok.text;
    }

    auto body = ReadBracedBlock();
    for (auto& piece : SplitTopLevel(body, ';'))
    {
        if (!piece.empty()) AddFieldLine(m, piece + ';');
    }

    _messageOrder.push_back(m._name);
    _messages[m._name] = std::move(m);
}


bool Schema::StartsWithKw(const std::string& s, const char* kw)
{
    std::size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])) != 0) ++i;
    std::size_t j = 0;
    while (i < s.size() && kw[j] && s[i] == kw[j]) { ++i; ++j; }
    return kw[j] == 0 && (i == s.size() || std::isspace(static_cast<unsigned char>(s[i])) != 0 || s[i] == '<');
}

std::string Schema::NormalizeFieldLine(std::string line)
{
    line = Trim(line);
    if (line.empty()) return line;
    // allow "enum Status statusCode;" inside messages by stripping "enum "
    if (StartsWithKw(line, "enum"))
    {
        std::size_t i = 0;
        while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i])) != 0) ++i; // skip 'enum'
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])) != 0) ++i;  // spaces
        line = line.substr(i);
    }
    return line;
}

void Schema::AddFieldLine(Message& m, std::string raw)
{
    auto line = NormalizeFieldLine(std::move(raw));
    if (line.empty()) return;
    if (StartsWithKw(line, "message") || StartsWithKw(line, "enum") || StartsWithKw(line, "extends"))
        return;
    m._fields.emplace_back(Type::from_line(line));
}

void Schema::EnsureMessageTypeEnum()
{
    EnumDecl& mt = _enums["MessageType"];
    mt._name = "MessageType";
    mt._capnpId = 0x0;
    if (mt._values.empty())
    {
        mt._values.push_back({"undefined", 0});
    }

    // track existing names if user defined some entries already
    std::unordered_set<std::string> existing;
    existing.reserve(mt._values.size() * 2 + _messageOrder.size());
    for (const auto& v : mt._values) existing.insert(v._name);

    // append missing entries in deterministic order
    for (const auto& mname : _messageOrder)
    {
        if (existing.find(mname) != existing.end()) continue;
        const auto& msg = _messages.at(mname);

        // Make first character lowercase to follow enum value naming convention
        auto name = msg._name;
        if (!name.empty() && std::isupper(static_cast<unsigned char>(name[0])) != 0)
        {
            name[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(name[0])));
        }
        if (existing.find(name) != existing.end()) continue; // avoid duplicates

        EnumValue ev { name, static_cast<std::int64_t>(msg._id) };
        mt._values.push_back(std::move(ev));
    }
}

} // namespace curious::dsl::capnpgen

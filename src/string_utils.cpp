#include "string_utils.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace curious::dsl::capnpgen
{
namespace string_utils
{

std::string trim(const std::string& str)
{
    std::size_t start = 0;
    std::size_t end = str.size();

    // Find first non-whitespace
    while (start < end && std::isspace(static_cast<unsigned char>(str[start])))
    {
        ++start;
    }

    // Find last non-whitespace
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
    {
        --end;
    }

    return str.substr(start, end - start);
}

std::string read_file(const std::string& file_path)
{
    std::ifstream file(file_path);
    if (!file)
    {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string strip_comments(const std::string& input)
{
    enum class State
    {
        Normal,
        AfterSlash,
        InLineComment,
        InBlockComment,
        AfterStar
    };

    std::string output;
    output.reserve(input.size());

    State state = State::Normal;

    for (char c : input)
    {
        switch (state)
        {
            case State::Normal:
                if (c == '/')
                {
                    state = State::AfterSlash;
                    output.push_back(' '); // Placeholder
                }
                else if (c == '#')
                {
                    // Hash-style line comment (e.g., # comment)
                    state = State::InLineComment;
                }
                else
                {
                    output.push_back(c);
                }
                break;

            case State::AfterSlash:
                if (c == '/')
                {
                    state = State::InLineComment;
                    output.back() = ' '; // Replace placeholder
                }
                else if (c == '*')
                {
                    state = State::InBlockComment;
                    output.back() = ' '; // Replace placeholder
                }
                else
                {
                    output.back() = '/'; // Restore the slash
                    output.push_back(c);
                    state = State::Normal;
                }
                break;

            case State::InLineComment:
                if (c == '\n')
                {
                    output.push_back('\n');
                    state = State::Normal;
                }
                // Ignore all other characters in line comment
                break;

            case State::InBlockComment:
                if (c == '*')
                {
                    state = State::AfterStar;
                }
                // Ignore all other characters in block comment
                break;

            case State::AfterStar:
                if (c == '/')
                {
                    state = State::Normal;
                }
                else if (c != '*')
                {
                    state = State::InBlockComment;
                }
                // Stay in AfterStar if we see another '*'
                break;
        }
    }

    return output;
}

std::vector<std::string> split_respecting_nesting(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    std::string current;

    int angle_bracket_depth = 0;
    int paren_depth = 0;
    int brace_depth = 0;

    for (char c : str)
    {
        // Track nesting levels
        if (c == '<') ++angle_bracket_depth;
        else if (c == '>') --angle_bracket_depth;
        else if (c == '(') ++paren_depth;
        else if (c == ')') --paren_depth;
        else if (c == '{') ++brace_depth;
        else if (c == '}') --brace_depth;

        // Split only at top level
        if (c == delimiter &&
            angle_bracket_depth == 0 &&
            paren_depth == 0 &&
            brace_depth == 0)
        {
            std::string trimmed = trim(current);
            if (!trimmed.empty())
            {
                result.push_back(trimmed);
            }
            current.clear();
        }
        else
        {
            current.push_back(c);
        }
    }

    // Add final piece
    std::string trimmed = trim(current);
    if (!trimmed.empty())
    {
        result.push_back(trimmed);
    }

    return result;
}

bool starts_with_keyword(const std::string& str, const char* keyword)
{
    std::size_t pos = 0;

    // Skip leading whitespace
    while (pos < str.size() && std::isspace(static_cast<unsigned char>(str[pos])))
    {
        ++pos;
    }

    // Match keyword
    std::size_t keyword_index = 0;
    while (pos < str.size() && keyword[keyword_index] != '\0' && str[pos] == keyword[keyword_index])
    {
        ++pos;
        ++keyword_index;
    }

    // Check that keyword matched completely
    if (keyword[keyword_index] != '\0')
    {
        return false;
    }

    // Check that keyword is followed by whitespace or '<'
    return pos == str.size() ||
           std::isspace(static_cast<unsigned char>(str[pos])) ||
           str[pos] == '<';
}

std::string to_lower(const std::string& str)
{
    std::string result;
    result.reserve(str.size());

    for (char c : str)
    {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    return result;
}

std::string to_cpp_namespace(const std::string& ns)
{
    std::string result = ns;
    std::string::size_type pos = 0;

    // Replace all dots with ::
    while ((pos = result.find('.', pos)) != std::string::npos)
    {
        result.replace(pos, 1, "::");
        pos += 2;
    }

    return result;
}

} // namespace string_utils
} // namespace curious::dsl::capnpgen

#include "lexer.hpp"

#include <cctype>

namespace curious::dsl::capnpgen
{

// ---- Token methods ----

bool Lexer::Token::is_keyword(const char* keyword) const
{
    return !is_eof && text == keyword;
}

bool Lexer::Token::is_identifier() const
{
    if (is_eof || text.empty())
    {
        return false;
    }

    // Must start with letter or underscore
    if (!std::isalpha(static_cast<unsigned char>(text[0])) && text[0] != '_')
    {
        return false;
    }

    // Remaining characters must be alphanumeric, underscore, or colon
    for (std::size_t i = 1; i < text.size(); ++i)
    {
        char c = text[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != ':')
        {
            return false;
        }
    }

    return true;
}

bool Lexer::Token::is_number() const
{
    if (is_eof || text.empty())
    {
        return false;
    }

    std::size_t index = 0;

    // Handle optional sign
    if (text[index] == '+' || text[index] == '-')
    {
        ++index;
    }

    if (index >= text.size())
    {
        return false;
    }

    // Check for hexadecimal (0x or 0X)
    if (index + 2 <= text.size() && text[index] == '0' &&
        (text[index + 1] == 'x' || text[index + 1] == 'X'))
    {
        if (index + 2 == text.size())
        {
            return false; // "0x" alone is not valid
        }

        for (std::size_t i = index + 2; i < text.size(); ++i)
        {
            if (!std::isxdigit(static_cast<unsigned char>(text[i])))
            {
                return false;
            }
        }
        return true;
    }

    // Check for decimal
    for (; index < text.size(); ++index)
    {
        if (!std::isdigit(static_cast<unsigned char>(text[index])))
        {
            return false;
        }
    }

    return true;
}

// ---- Lexer methods ----

Lexer::Lexer(std::string source)
    : _source(std::move(source))
{
}

Lexer::Token Lexer::next_token()
{
    _skip_whitespace();

    if (_position >= _source.size())
    {
        return Token{{}, true};
    }

    char current_char = _source[_position];

    // Handle single-character symbols
    if (_is_symbol(current_char))
    {
        ++_position;
        return Token{std::string(1, current_char), false};
    }

    // Handle identifiers (start with letter or underscore)
    if (std::isalpha(static_cast<unsigned char>(current_char)) || current_char == '_')
    {
        return _read_identifier();
    }

    // Handle numbers (start with digit, +, or -)
    if (std::isdigit(static_cast<unsigned char>(current_char)) ||
        current_char == '+' || current_char == '-')
    {
        return _read_number();
    }

    // Default: treat as single character token
    ++_position;
    return Token{std::string(1, current_char), false};
}

std::optional<Lexer::Token> Lexer::peek_token()
{
    std::size_t saved_position = _position;
    Token token = next_token();
    _position = saved_position;

    if (token.is_eof)
    {
        return std::nullopt;
    }

    return token;
}

// ---- Private helper methods ----

void Lexer::_skip_whitespace()
{
    while (_position < _source.size() &&
           std::isspace(static_cast<unsigned char>(_source[_position])))
    {
        ++_position;
    }
}

bool Lexer::_is_symbol(char c) const
{
    return c == '{' || c == '}' || c == '(' || c == ')' ||
           c == '/' || c == '*' || c == ';' || c == ',' ||
           c == '<' || c == '>' || c == '.' || c == '|' || c == '@';
}

Lexer::Token Lexer::_read_identifier()
{
    std::size_t start = _position;

    // Skip first character (already validated as letter or underscore)
    ++_position;

    // Continue while we see alphanumeric, underscore, or colon
    while (_position < _source.size())
    {
        char c = _source[_position];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == ':')
        {
            ++_position;
        }
        else
        {
            break;
        }
    }

    std::string token_text = _source.substr(start, _position - start);
    return Token{token_text, false};
}

Lexer::Token Lexer::_read_number()
{
    std::size_t start = _position;
    char first_char = _source[_position];

    // Handle sign if present
    if (first_char == '+' || first_char == '-')
    {
        ++_position;
    }

    // Check for hexadecimal (0x or 0X)
    if (_position < _source.size() && _source[_position] == '0')
    {
        if (_position + 1 < _source.size() &&
            (_source[_position + 1] == 'x' || _source[_position + 1] == 'X'))
        {
            _position += 2; // Skip '0x'

            // Read hexadecimal digits
            while (_position < _source.size() &&
                   std::isxdigit(static_cast<unsigned char>(_source[_position])))
            {
                ++_position;
            }

            std::string token_text = _source.substr(start, _position - start);
            return Token{token_text, false};
        }
    }

    // Read decimal digits
    while (_position < _source.size() &&
           std::isdigit(static_cast<unsigned char>(_source[_position])))
    {
        ++_position;
    }

    std::string token_text = _source.substr(start, _position - start);
    return Token{token_text, false};
}

} // namespace curious::dsl::capnpgen

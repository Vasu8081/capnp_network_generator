#pragma once

#include <optional>
#include <string>

namespace curious::dsl::capnpgen
{

/// @brief A simple lexical analyzer for tokenizing DSL input.
/// @details Breaks source text into tokens, handling identifiers, numbers, symbols, and whitespace.
class Lexer
{
public:
    /// @brief Represents a single token from the source.
    struct Token
    {
        /// @brief The text content of this token.
        std::string text;

        /// @brief True if this represents end-of-file.
        bool is_eof{false};

        /// @brief Check if this token matches a specific keyword.
        /// @param keyword The keyword to compare against.
        /// @return True if the token text equals the keyword.
        bool is_keyword(const char* keyword) const;

        /// @brief Check if this token is a valid identifier.
        /// @return True if the token is an identifier (starts with letter/underscore).
        bool is_identifier() const;

        /// @brief Check if this token is a numeric literal.
        /// @return True if the token represents a decimal or hexadecimal number.
        bool is_number() const;
    };

    /// @brief Construct a lexer with the given source text.
    /// @param source The complete source text to tokenize.
    explicit Lexer(std::string source);

    /// @brief Consume and return the next token.
    /// @return The next token, or a token with is_eof=true if at end.
    Token next_token();

    /// @brief Look at the next token without consuming it.
    /// @return An optional containing the next token, or nullopt if at end.
    std::optional<Token> peek_token();

private:
    /// @brief The source text being tokenized.
    std::string _source;

    /// @brief Current position in the source text.
    std::size_t _position{0};

    /// @brief Skip whitespace characters at the current position.
    void _skip_whitespace();

    /// @brief Check if a character is a single-character symbol token.
    /// @param c The character to check.
    /// @return True if the character is a recognized symbol.
    bool _is_symbol(char c) const;

    /// @brief Read an identifier token starting at the current position.
    /// @return The identifier token.
    Token _read_identifier();

    /// @brief Read a number token starting at the current position.
    /// @return The number token.
    Token _read_number();
};

} // namespace curious::dsl::capnpgen

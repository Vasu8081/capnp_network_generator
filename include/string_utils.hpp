#pragma once

#include <string>
#include <vector>

namespace curious::dsl::capnpgen
{

/// @brief Utility functions for string manipulation.
namespace string_utils
{

/// @brief Remove leading and trailing whitespace from a string.
/// @param str The string to trim.
/// @return A new string with whitespace removed.
std::string trim(const std::string& str);

/// @brief Read the entire contents of a file.
/// @param file_path Path to the file to read.
/// @return The file contents as a string.
/// @throws std::runtime_error if the file cannot be opened.
std::string read_file(const std::string& file_path);

/// @brief Remove C++ style comments (// and /* */) from source text.
/// @param input The source text potentially containing comments.
/// @return A new string with comments replaced by spaces.
std::string strip_comments(const std::string& input);

/// @brief Split a string by a delimiter, respecting nested brackets.
/// @details Ignores delimiters inside <>, (), or {} pairs.
/// @param str The string to split.
/// @param delimiter The character to split on.
/// @return A vector of trimmed substrings.
std::vector<std::string> split_respecting_nesting(const std::string& str, char delimiter);

/// @brief Check if a string starts with a specific keyword.
/// @param str The string to check.
/// @param keyword The keyword to look for.
/// @return True if the string starts with the keyword followed by whitespace or '<'.
bool starts_with_keyword(const std::string& str, const char* keyword);

/// @brief Convert a string to lowercase.
/// @param str The string to convert.
/// @return A new lowercase string.
std::string to_lower(const std::string& str);

/// @brief Convert dot-separated namespace to C++ format (::).
/// @param ns The namespace string (e.g., "curious.message").
/// @return The namespace in C++ format (e.g., "curious::message").
std::string to_cpp_namespace(const std::string& ns);

/// @brief Convert PascalCase to lowerCamelCase (first letter lowercase).
/// @param str The string in PascalCase (e.g., "YoutubeVideo").
/// @return The string in lowerCamelCase (e.g., "youtubeVideo").
std::string to_lower_camel_case(const std::string& str);

} // namespace string_utils

} // namespace curious::dsl::capnpgen

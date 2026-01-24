#include "cpp_message_base_generator.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "string_utils.hpp"

namespace curious::dsl::capnpgen
{

// ---- Constructor ----

CppMessageBaseGenerator::CppMessageBaseGenerator(const Schema& schema, const std::string& output_directory, const std::string& include_prefix)
    : _schema(schema)
    , _outputDirectory(_resolve_output_directory(output_directory))
    , _includePrefix(include_prefix)
{
    namespace fs = std::filesystem;

    // Construct output file path
    fs::path output_file_path = fs::path(_outputDirectory) / "MessageBase.hpp";

    // Generate content
    std::string content = _generate_message_base_content();

    // Write to file
    std::ofstream output_file(output_file_path, std::ios::binary);
    if (!output_file)
    {
        throw std::runtime_error("Failed to create MessageBase header file: " + output_file_path.string());
    }

    output_file << content;
}

// ---- Private static methods ----

std::string CppMessageBaseGenerator::_resolve_output_directory(const std::string& path)
{
    namespace fs = std::filesystem;
    fs::create_directories(path);
    return path;
}

// ---- Private instance methods ----

std::string CppMessageBaseGenerator::_generate_message_base_content()
{
    std::ostringstream content;

    // Use wrapper_namespace_name if specified, otherwise fall back to namespace_name
    const std::string& raw_ns = _schema.wrapper_namespace_name.empty() ?
                                  _schema.namespace_name : _schema.wrapper_namespace_name;
    const std::string ns = raw_ns.empty() ?
                             "curious::net" :
                             string_utils::to_cpp_namespace(raw_ns);

    // Header guard
    content << "#pragma once\n\n";
    content << "#ifndef MESSAGEBASE_HPP\n";
    content << "#define MESSAGEBASE_HPP\n\n";

    // Includes
    content << "#include <cstdint>\n";
    content << "#include <cstdlib>\n";
    content << "#include <vector>\n";
    content << "#include <memory>\n";
    content << "#include <string>\n";
    content << "#include <utility>\n\n";
    content << "#include <kj/array.h>\n";
    content << "#include <capnp/common.h>\n\n";

    // Open namespace
    content << "namespace " << ns << "\n";
    content << "{\n\n";

    // SerializedData struct - zero-copy wrapper for Cap'n Proto data
    content << "/// @brief Zero-copy wrapper for serialized Cap'n Proto message data.\n";
    content << "/// @details Holds a kj::Array<capnp::word> directly from messageToFlatArray().\n";
    content << "///          This avoids unnecessary copies - the data is allocated once by Cap'n Proto\n";
    content << "///          and can be passed directly to the transport layer.\n";
    content << "struct SerializedData\n";
    content << "{\n";
    content << "    kj::Array<capnp::word> words;\n\n";

    content << "    SerializedData() = default;\n";
    content << "    explicit SerializedData(kj::Array<capnp::word>&& w) : words(kj::mv(w)) {}\n\n";

    content << "    // Move-only semantics (kj::Array is already move-only)\n";
    content << "    SerializedData(const SerializedData&) = delete;\n";
    content << "    SerializedData& operator=(const SerializedData&) = delete;\n\n";

    content << "    SerializedData(SerializedData&& other) noexcept = default;\n";
    content << "    SerializedData& operator=(SerializedData&& other) noexcept = default;\n\n";

    content << "    ~SerializedData() = default;\n\n";

    content << "    /// @brief Check if data is valid.\n";
    content << "    explicit operator bool() const { return words.size() > 0; }\n\n";

    content << "    /// @brief Get raw data pointer.\n";
    content << "    const void* data() const { return words.begin(); }\n\n";

    content << "    /// @brief Get size in bytes.\n";
    content << "    std::size_t size() const { return words.size() * sizeof(capnp::word); }\n\n";

    content << "    /// @brief Get size in words (8-byte units).\n";
    content << "    std::size_t word_count() const { return words.size(); }\n\n";

    content << "    /// @brief Get data as byte pointer.\n";
    content << "    const std::uint8_t* bytes() const { return reinterpret_cast<const std::uint8_t*>(words.begin()); }\n\n";

    content << "    /// @brief Release ownership of the underlying array.\n";
    content << "    /// @details Caller becomes responsible for the array's lifetime.\n";
    content << "    ///          After calling this, the SerializedData is empty.\n";
    content << "    kj::Array<capnp::word> release() { return kj::mv(words); }\n";
    content << "};\n\n";

    // MessageBase class
    content << "/// @brief Base class for all generated message classes.\n";
    content << "/// @details Provides common serialization/deserialization interface.\n";
    content << "///          All generated message classes inherit from this base.\n";
    content << "class MessageBase\n";
    content << "{\n";
    content << "public:\n";
    content << "    /// @brief Default constructor.\n";
    content << "    MessageBase() = default;\n\n";

    content << "    /// @brief Virtual destructor for proper cleanup.\n";
    content << "    virtual ~MessageBase() = default;\n\n";

    content << "    /// @brief Copy constructor.\n";
    content << "    MessageBase(const MessageBase& other) = default;\n\n";

    content << "    /// @brief Move constructor.\n";
    content << "    MessageBase(MessageBase&& other) noexcept = default;\n\n";

    content << "    /// @brief Copy assignment operator.\n";
    content << "    MessageBase& operator=(const MessageBase& other) = default;\n\n";

    content << "    /// @brief Move assignment operator.\n";
    content << "    MessageBase& operator=(MessageBase&& other) noexcept = default;\n\n";

    content << "    /// @brief Get the message type identifier.\n";
    content << "    /// @return The message type ID.\n";
    content << "    virtual std::uint64_t get_message_id() const = 0;\n\n";

    content << "    /// @brief Get the message type name.\n";
    content << "    /// @return The message type name as a string.\n";
    content << "    virtual std::string get_message_name() const = 0;\n\n";

    content << "    /// @brief Serialize this message to a byte vector.\n";
    content << "    /// @return A vector containing the serialized message.\n";
    content << "    /// @note For better performance, use serialize_fast() instead.\n";
    content << "    virtual std::vector<std::uint8_t> serialize() const = 0;\n\n";

    content << "    /// @brief Serialize this message with minimal copies.\n";
    content << "    /// @return SerializedData containing word-aligned serialized data.\n";
    content << "    /// @note This avoids the vector copy overhead of serialize().\n";
    content << "    virtual SerializedData serialize_fast() const = 0;\n\n";

    content << "    /// @brief Deserialize from a byte vector.\n";
    content << "    /// @param data The serialized data.\n";
    content << "    /// @return True if deserialization succeeded, false otherwise.\n";
    content << "    virtual bool deserialize(const std::vector<std::uint8_t>& data) = 0;\n\n";

    content << "    /// @brief Deserialize from a byte buffer.\n";
    content << "    /// @param data Pointer to the data buffer.\n";
    content << "    /// @param size Size of the data buffer in bytes.\n";
    content << "    /// @return True if deserialization succeeded, false otherwise.\n";
    content << "    virtual bool deserialize(const std::uint8_t* data, std::size_t size) = 0;\n";
    content << "};\n\n";

    // Close namespace
    content << "} // namespace " << ns << "\n\n";

    // Close header guard
    content << "#endif // MESSAGEBASE_HPP\n";

    return content.str();
}

} // namespace curious::dsl::capnpgen

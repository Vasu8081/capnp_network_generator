#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <utility>

namespace curious::dsl::capnpgen
{

/// @brief Lightweight wrapper for serialized message data.
/// @details Holds a pointer to word-aligned data and its size.
///          The data is owned by this struct and freed on destruction.
struct SerializedData
{
    void* data = nullptr;
    std::size_t size = 0;          // Size in bytes
    std::size_t word_count = 0;    // Size in words (8-byte units)

    SerializedData() = default;
    SerializedData(void* ptr, std::size_t byte_size, std::size_t words)
        : data(ptr), size(byte_size), word_count(words) {}

    // Move-only semantics
    SerializedData(const SerializedData&) = delete;
    SerializedData& operator=(const SerializedData&) = delete;

    SerializedData(SerializedData&& other) noexcept
        : data(other.data), size(other.size), word_count(other.word_count)
    {
        other.data = nullptr;
        other.size = 0;
        other.word_count = 0;
    }

    SerializedData& operator=(SerializedData&& other) noexcept
    {
        if (this != &other)
        {
            free(data);
            data = other.data;
            size = other.size;
            word_count = other.word_count;
            other.data = nullptr;
            other.size = 0;
            other.word_count = 0;
        }
        return *this;
    }

    ~SerializedData() { free(data); }

    /// @brief Check if data is valid.
    explicit operator bool() const { return data != nullptr && size > 0; }

    /// @brief Get data as byte pointer.
    const std::uint8_t* bytes() const { return static_cast<const std::uint8_t*>(data); }
};

/// @brief Base class for all generated message classes.
/// @details Provides common serialization/deserialization interface.
///          All generated message classes inherit from this base.
class MessageBase
{
public:
    /// @brief Default constructor.
    MessageBase() = default;

    /// @brief Virtual destructor for proper cleanup.
    virtual ~MessageBase() = default;

    /// @brief Copy constructor.
    MessageBase(const MessageBase& other) = default;

    /// @brief Move constructor.
    MessageBase(MessageBase&& other) noexcept = default;

    /// @brief Copy assignment operator.
    MessageBase& operator=(const MessageBase& other) = default;

    /// @brief Move assignment operator.
    MessageBase& operator=(MessageBase&& other) noexcept = default;

    /// @brief Get the message type identifier.
    /// @return The message type ID.
    virtual std::uint64_t get_message_id() const = 0;

    /// @brief Get the message type name.
    /// @return The message type name as a string.
    virtual std::string get_message_name() const = 0;

    /// @brief Serialize this message to a byte vector.
    /// @return A vector containing the serialized message.
    /// @note For better performance, use serialize_fast() instead.
    virtual std::vector<std::uint8_t> serialize() const = 0;

    /// @brief Serialize this message with minimal copies.
    /// @return SerializedData containing word-aligned serialized data.
    /// @note This avoids the vector copy overhead of serialize().
    virtual SerializedData serialize_fast() const = 0;

    /// @brief Deserialize from a byte vector.
    /// @param data The serialized data.
    /// @return True if deserialization succeeded, false otherwise.
    virtual bool deserialize(const std::vector<std::uint8_t>& data) = 0;

    /// @brief Deserialize from a byte buffer.
    /// @param data Pointer to the data buffer.
    /// @param size Size of the data buffer in bytes.
    /// @return True if deserialization succeeded, false otherwise.
    virtual bool deserialize(const std::uint8_t* data, std::size_t size) = 0;
};

} // namespace curious::dsl::capnpgen

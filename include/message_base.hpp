#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace curious::dsl::capnpgen
{

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
    virtual std::vector<std::uint8_t> serialize() const = 0;

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

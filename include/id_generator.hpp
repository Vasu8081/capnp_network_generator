#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace curious::dsl::capnpgen
{

/// @brief Utility class for generating and formatting Cap'n Proto IDs.
class IdGenerator
{
public:
    /// @brief Generate a random Cap'n Proto ID with MSB set.
    /// @return A random 64-bit ID suitable for Cap'n Proto.
    static std::uint64_t generate_random_id();

    /// @brief Derive a deterministic ID from a parent ID and a name.
    /// @param parent_id The parent ID to derive from.
    /// @param name The name to incorporate into the derived ID.
    /// @return A derived ID with MSB set.
    static std::uint64_t derive_id(std::uint64_t parent_id, std::string_view name);

    /// @brief Format an ID as a Cap'n Proto hex string.
    /// @param id The ID to format.
    /// @return A string like "@0x0123456789abcdef".
    static std::string format_id_as_hex(std::uint64_t id);

    /// @brief Extract the file ID from an existing Cap'n Proto schema file.
    /// @param file_path Path to the .capnp file.
    /// @return The file ID if found, or 0 if not found or file doesn't exist.
    static std::uint64_t extract_file_id_from_capnp(const std::string& file_path);

    /// @brief Compute FNV-1a 64-bit hash of a string.
    /// @param data The data to hash.
    /// @return The 64-bit hash value.
    static std::uint64_t compute_fnv1a_hash(std::string_view data);

private:
    /// @brief FNV-1a offset basis constant.
    static constexpr std::uint64_t FNV1A_OFFSET_BASIS = 0xcbf29ce484222325ULL;

    /// @brief FNV-1a prime constant.
    static constexpr std::uint64_t FNV1A_PRIME = 0x100000001b3ULL;

    /// @brief Cap'n Proto ID MSB flag (must be set for valid IDs).
    static constexpr std::uint64_t CAPNP_ID_MSB_FLAG = 1ULL << 63;
};

} // namespace curious::dsl::capnpgen

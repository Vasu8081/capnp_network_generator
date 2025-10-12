#include "id_generator.hpp"

#include <iomanip>
#include <random>
#include <sstream>

namespace curious::dsl::capnpgen
{

std::uint64_t IdGenerator::generate_random_id()
{
    std::random_device random_device;

    // Generate random 64-bit value
    std::uint64_t high = (static_cast<std::uint64_t>(random_device()) << 32) ^ random_device();
    std::uint64_t low = (static_cast<std::uint64_t>(random_device()) << 32) ^ random_device();
    std::uint64_t random_value = (high << 32) ^ low;

    // Ensure MSB is set (Cap'n Proto requirement)
    return random_value | CAPNP_ID_MSB_FLAG;
}

std::uint64_t IdGenerator::derive_id(std::uint64_t parent_id, std::string_view name)
{
    // Convert parent ID to bytes (big-endian)
    char parent_bytes[8];
    for (int i = 0; i < 8; ++i)
    {
        parent_bytes[i] = static_cast<char>((parent_id >> (8 * (7 - i))) & 0xFF);
    }

    // Concatenate parent ID bytes with name
    std::string seed(parent_bytes, parent_bytes + 8);
    seed.append(name);

    // Compute hash and ensure MSB is set
    return compute_fnv1a_hash(seed) | CAPNP_ID_MSB_FLAG;
}

std::string IdGenerator::format_id_as_hex(std::uint64_t id)
{
    std::ostringstream oss;
    oss << "@0x"
        << std::hex << std::nouppercase
        << std::setfill('0') << std::setw(16)
        << id;
    return oss.str();
}

std::uint64_t IdGenerator::compute_fnv1a_hash(std::string_view data)
{
    std::uint64_t hash = FNV1A_OFFSET_BASIS;

    for (unsigned char byte : data)
    {
        hash ^= byte;
        hash *= FNV1A_PRIME;
    }

    return hash;
}

} // namespace curious::dsl::capnpgen

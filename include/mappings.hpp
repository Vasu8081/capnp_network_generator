#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept> 

namespace curious::dsl::capnpgen
{

/// @brief Enumerates all supported DSL types.
enum class DslType
{
    Int8,        ///< 8-bit signed integer
    Int16,       ///< 16-bit signed integer
    Int32,       ///< 32-bit signed integer
    Int64,       ///< 64-bit signed integer
    Uint8,       ///< 8-bit unsigned integer
    Uint16,      ///< 16-bit unsigned integer
    Uint32,      ///< 32-bit unsigned integer
    Uint64,      ///< 64-bit unsigned integer
    Float32,     ///< 32-bit floating-point number
    Float64,     ///< 64-bit floating-point number
    Bool,        ///< Boolean value
    String,      ///< UTF-8 text string
    Bytes,       ///< Raw byte sequence
    List,        ///< List type
    Map,         ///< Map type
    Enum,        ///< Enumeration type
    AnyPointer,  ///< Cap’n Proto AnyPointer
    Void,        ///< Void type
    Custom       ///< User-defined/custom type
};

/// @brief Map from DSL string keywords → DslType enum.
inline const std::unordered_map<std::string, DslType> string_to_dsl_map =
{
    {"int",      DslType::Int32},
    {"int8",     DslType::Int8},
    {"int16",    DslType::Int16},
    {"int32",    DslType::Int32},
    {"int64",    DslType::Int64},
    {"uint8",    DslType::Uint8},
    {"uint16",   DslType::Uint16},
    {"uint32",   DslType::Uint32},
    {"uint64",   DslType::Uint64},
    {"float32",  DslType::Float32},
    {"float64",  DslType::Float64},
    {"bool",     DslType::Bool},
    {"string",   DslType::String},
    {"bytes",    DslType::Bytes},
    {"list",     DslType::List},
    {"map",      DslType::Map},
    {"enum",     DslType::Enum},
    {"anypointer", DslType::AnyPointer},
    {"void",     DslType::Void}
};

/// @brief Map from DslType → Cap’n Proto type string.
inline const std::unordered_map<DslType, std::string> dsl_to_capnp_map =
{
    {DslType::Int8,       "Int8"},
    {DslType::Int16,      "Int16"},
    {DslType::Int32,      "Int32"},
    {DslType::Int64,      "Int64"},
    {DslType::Uint8,      "UInt8"},
    {DslType::Uint16,     "UInt16"},
    {DslType::Uint32,     "UInt32"},
    {DslType::Uint64,     "UInt64"},
    {DslType::Float32,    "Float32"},
    {DslType::Float64,    "Float64"},
    {DslType::Bool,       "Bool"},
    {DslType::String,     "Text"},
    {DslType::Bytes,      "Data"},
    {DslType::List,       "List"},
    {DslType::Map,        "Map"},
    {DslType::Enum,       "Enum"},
    {DslType::AnyPointer, "AnyPointer"},
    {DslType::Void,       "Void"}
};

/// @brief Map from DslType → C++ type string.
inline const std::unordered_map<DslType, std::string> dsl_to_cpp_map =
{
    {DslType::Int8,       "int8_t"},
    {DslType::Int16,      "int16_t"},
    {DslType::Int32,      "int32_t"},
    {DslType::Int64,      "int64_t"},
    {DslType::Uint8,      "uint8_t"},
    {DslType::Uint16,     "uint16_t"},
    {DslType::Uint32,     "uint32_t"},
    {DslType::Uint64,     "uint64_t"},
    {DslType::Float32,    "float"},
    {DslType::Float64,    "double"},
    {DslType::Bool,       "bool"},
    {DslType::String,     "std::string"},
    {DslType::Bytes,      "std::vector<uint8_t>"},
    {DslType::List,       "std::vector"},
    {DslType::Map,        "std::unordered_map"},
    {DslType::Enum,       "enum"},
    {DslType::AnyPointer, "void*"},
    {DslType::Void,       "void"}
};

} // namespace curious::dsl::capnpgen

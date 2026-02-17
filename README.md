# Cap'n Proto Network Generator

Generates a Cap'n Proto schema and C++ wrapper classes from a DSL file. Write your messages once, get serialization for free.

```
schema.dsl  ──>  capnp_generator  ──>  .capnp schema
                                  ──>  .hpp/.cpp per message
                                  ──>  enums.hpp
                                  ──>  MessageBase.hpp
                                  ──>  factory_builder.h
```

## CLI

```bash
capnp_generator -i <dsl> -ocapnp <dir> [-ohpp <dir> -ocpp <dir>]
```

| Flag | Short | Required | Description |
|------|-------|----------|-------------|
| `--input` | `-i` | Yes | Input DSL file |
| `--out-capnp` | `-ocapnp` | Yes | Output directory for `network_msg.capnp` |
| `--out-hpp` | `-ohpp` | No | Output directory for `.hpp` headers |
| `--out-cpp` | `-ocpp` | No | Output directory for `.cpp` sources |

If either `-ohpp` or `-ocpp` is given, both are required. The last folder name from `-ohpp` becomes the include prefix (e.g. `-ohpp include/network` produces `#include <network/MyMessage.hpp>`).

```bash
# Schema only
capnp_generator -i schemas/network.dsl -ocapnp schemas/

# Schema + C++ wrappers
capnp_generator -i schemas/network.dsl -ocapnp schemas/ \
    -ohpp include/network -ocpp src/network/src
```

## DSL Syntax

```dsl
namespace curious.message;
wrapper_namespace curious.net;    // C++ namespace (optional, defaults to namespace)

enum Status {
    OK | 0,
    ERROR | 1,
    PENDING | 2
}

message Request(50) {
    string requestId;
    int32 timeout;
}

message DataRequest(51) extends Request {
    vector<string> tags;
    map<string, int32> filters;
    bool verbose;
}
```

### Keywords

| Keyword | Usage |
|---------|-------|
| `namespace` | Cap'n Proto package: `namespace curious.message;` |
| `wrapper_namespace` | C++ namespace for wrappers: `wrapper_namespace curious.net;` |
| `enum` | `enum Name { A \| 0, B \| 1 }` — values after `\|` are optional |
| `message(id)` | `message Name(42) { ... }` — id is a unique numeric identifier |
| `extends` | `message Child(43) extends Parent { ... }` — inherits all parent fields |

### Types

| DSL | C++ | Cap'n Proto |
|-----|-----|-------------|
| `int8`, `int16`, `int32`, `int64` | `int8_t` ... `int64_t` | `Int8` ... `Int64` |
| `uint8`, `uint16`, `uint32`, `uint64` | `uint8_t` ... `uint64_t` | `UInt8` ... `UInt64` |
| `int` | `int32_t` | `Int32` |
| `float32`, `float64` | `float`, `double` | `Float32`, `Float64` |
| `bool` | `bool` | `Bool` |
| `string` | `std::string` | `Text` |
| `bytes` / `data` | `std::vector<uint8_t>` | `Data` |
| `vector<T>` / `list<T>` | `std::vector<T>` | `List(T)` |
| `map<K, V>` | `std::unordered_map<K, V>` | `Map(K, V)` struct |
| `EnumName` | `EnumName` (cast) | `EnumName` |
| `MessageName` | `MessageName` | Nested struct |

## Generated Output

### Per Message: `Message.hpp` + `Message.cpp`

Each message becomes a class inheriting from its parent (or `MessageBase`):

```cpp
class DataRequest : public Request {
public:
    DataRequest();
    DataRequest(const DataRequest& other);
    DataRequest(DataRequest&& other) noexcept;
    DataRequest& operator=(const DataRequest& other);
    DataRequest& operator=(DataRequest&& other) noexcept;
    virtual ~DataRequest();

    // MessageBase interface
    std::uint64_t get_message_id() const override;    // returns 51
    std::string get_message_name() const override;     // returns "DataRequest"
    std::vector<std::uint8_t> serialize() const override;
    SerializedData serialize_fast() const override;    // zero-copy
    bool deserialize(const std::vector<std::uint8_t>& data) override;
    bool deserialize(const std::uint8_t* data, std::size_t size) override;

    // Cap'n Proto conversion
    void to_capnp(::capnp::MessageBuilder& builder) const;
    void from_capnp(::capnp::MessageReader& reader);
    template<typename B> void to_capnp_struct(B&& builder) const;
    template<typename R> void from_capnp_struct(const R& reader);

    // Fields (public, inherited + own)
    std::vector<std::string> tags;
    std::unordered_map<std::string, int32_t> filters;
    bool verbose;
};
```

The `.cpp` handles all Cap'n Proto conversion automatically — primitives, strings, lists, maps, nested messages, and enums. `serialize_fast()` returns a `SerializedData` wrapper around `kj::Array<capnp::word>` for zero-copy use.

### `enums.hpp`

All DSL enums plus an auto-generated `MessageType` enum with an entry per message. Each enum gets:
- `enum class Name : std::int64_t { ... }`
- `operator<<` for stream output
- `NameFromString()` for string-to-enum conversion

### `MessageBase.hpp`

Abstract base class with the virtual interface (`get_message_id`, `serialize`, `deserialize`, etc.) and the `SerializedData` zero-copy wrapper struct.

### `factory_builder.h`

`FactoryBuilder::createMessage(MessageType type)` — returns a `shared_ptr<MessageBase>` for any message type via a switch on the `MessageType` enum.

### `network_msg.capnp`

The Cap'n Proto schema with all enums and structs. Inherited fields are flattened into child structs. A `Map(Key, Value)` helper struct is always included. Struct IDs are derived from the file ID and preserved across regeneration.

## User Code Preservation

Generated files have marker comments. Code between them survives regeneration:

**`.hpp`:** `USER_INCLUDES`, `USER_METHODS`, `USER_PROTECTED`, `USER_PRIVATE`
**`.cpp`:** `USER_IMPL_INCLUDES`, `USER_CONSTRUCTOR`, `USER_TO_CAPNP`, `USER_FROM_CAPNP`, `USER_COPY_FROM`, `USER_IMPL`
**`enums.hpp`:** `USER_INCLUDES`, `USER_DEFINITIONS`

```cpp
// In MyMessage.hpp:
// USER_METHODS_START
bool validate() const;
// USER_METHODS_END

// In MyMessage.cpp:
// USER_IMPL_START
bool MyMessage::validate() const { return !tags.empty(); }
// USER_IMPL_END
```

## Build

```bash
cmake -S . -B build && cmake --build build
```

Requires C++20, CMake 3.16+.

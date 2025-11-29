# C++ Wrapper for Capnp

## Overview

The Cap'n Proto Network Generator now supports automatic generation of C++ wrapper classes that provide seamless conversion between C++ objects and Cap'n Proto messages. This eliminates the need to manually write serialization/deserialization code.

## Features

### ✅ Complete Type Support

- **Primitives**: int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64, bool
- **Strings**: string type maps to `std::string`
- **Bytes**: bytes type maps to `std::vector<uint8_t>`
- **Collections**: vector/list maps to `std::vector<T>`
- **Maps**: map types map to `std::unordered_map<K, V>`
- **Enums**: Custom enums with proper casting
- **Custom Types**: User-defined message types with recursive conversion
- **Inheritance**: Full support for message inheritance chains

### ✅ User Code Preservation

Generated files include marked sections for user code that are preserved across regeneration:

- `USER_INCLUDES_START/END` - Custom #include directives
- `USER_METHODS_START/END` - Custom public methods
- `USER_PROTECTED_START/END` - Custom protected members
- `USER_PRIVATE_START/END` - Custom private members
- `USER_IMPL_INCLUDES_START/END` - Implementation includes
- `USER_CONSTRUCTOR_START/END` - Custom constructor logic
- `USER_TO_CAPNP_START/END` - Custom serialization code
- `USER_FROM_CAPNP_START/END` - Custom deserialization code
- `USER_COPY_FROM_START/END` - Custom copy logic
- `USER_IMPL_START/END` - Custom method implementations

### ✅ Base Class with Common Interface

All generated classes inherit from `MessageBase` which provides:

```cpp
class MessageBase
{
public:
    virtual std::uint64_t get_message_id() const = 0;
    virtual std::string get_message_name() const = 0;
    virtual std::vector<std::uint8_t> serialize() const = 0;
    virtual bool deserialize(const std::vector<std::uint8_t>& data) = 0;
    virtual bool deserialize(const std::uint8_t* data, std::size_t size) = 0;
};
```

## Usage

### Basic Generation

```bash
# Generate only Cap'n Proto schema
./capnp_generator schema.dsl output/

# Generate Cap'n Proto schema AND C++ wrapper classes
./capnp_generator schema.dsl output/ --cpp-dir src/generated/
```

### Example DSL Schema

```dsl
namespace my::app::messages;

enum Status {
    OK | 0,
    ERROR | 1,
    PENDING | 2
}

message BaseMessage (100) {
    int64 timestamp;
    string sender;
}

message DataMessage (101) extends BaseMessage {
    vector<int32> values;
    Status status;
    string payload;
}
```

### Generated C++ Usage

```cpp
#include "DataMessage.hpp"

// Create a message
test::messages::DataMessage msg;
msg.timestamp = 1234567890;
msg.sender = "user@example.com";
msg.values = {1, 2, 3, 4, 5};
msg.status = test::messages::Status::OK;
msg.payload = "Hello, World!";

// Serialize to bytes
std::vector<std::uint8_t> data = msg.serialize();

// Send data over network...

// Deserialize from bytes
test::messages::DataMessage received;
if (received.deserialize(data))
{
    std::cout << "Received from: " << received.sender << "\n";
    std::cout << "Payload: " << received.payload << "\n";
    std::cout << "Values count: " << received.values.size() << "\n";
}
```

## Type Conversion Reference

### Primitive Types

| DSL Type | C++ Type | Cap'n Proto Type |
|----------|----------|------------------|
| int8 | int8_t | Int8 |
| int16 | int16_t | Int16 |
| int32 | int32_t | Int32 |
| int64 | int64_t | Int64 |
| uint8 | uint8_t | UInt8 |
| uint16 | uint16_t | UInt16 |
| uint32 | uint32_t | UInt32 |
| uint64 | uint64_t | UInt64 |
| float32 | float | Float32 |
| float64 | double | Float64 |
| bool | bool | Bool |
| string | std::string | Text |
| bytes | std::vector<uint8_t> | Data |

### Collection Types

```dsl
// DSL
vector<int32> numbers;
map<string, int32> scores;

// Generated C++
std::vector<int32_t> numbers;
std::unordered_map<std::string, int32_t> scores;
```

### Nested Types

```dsl
message Inner (1) {
    string value;
}

message Outer (2) {
    Inner nested;
    vector<Inner> list_of_nested;
}
```

Generated code automatically handles recursive conversion:

```cpp
Outer outer;
outer.nested.value = "Hello";
outer.list_of_nested.push_back(Inner{});

// Serialization handles all nesting automatically
auto data = outer.serialize();
```

## Inheritance Support

```dsl
message Base (100) {
    int64 id;
}

message Derived (101) extends Base {
    string name;
}
```

Generated C++ properly inherits and calls parent methods:

```cpp
class Derived : public Base
{
    // Inherits id field from Base
    std::string name;

    // to_capnp() calls Base::to_capnp() first
    // from_capnp() calls Base::from_capnp() first
};
```

## Adding Custom Code

### Example: Adding Validation

```cpp
// In DataMessage.hpp, in USER_METHODS_START section:
// USER_METHODS_START
bool validate() const;
std::string to_json() const;
// USER_METHODS_END

// In DataMessage.cpp, in USER_IMPL_START section:
// USER_IMPL_START
bool DataMessage::validate() const
{
    return !payload.empty() && !values.empty();
}

std::string DataMessage::to_json() const
{
    // Custom JSON conversion logic
    return "{}";
}
// USER_IMPL_END
```

When you regenerate, your custom code is preserved!

## Generated File Structure

For each message `MessageName`, the following files are generated:

### MessageName.hpp
```
├── Include guards
├── Standard includes
├── message_base.hpp include
├── USER_INCLUDES section (preserved)
├── Namespace declaration
├── Class declaration
│   ├── Constructors/Destructor
│   ├── MessageBase interface implementation
│   ├── Cap'n Proto conversion methods
│   ├── Generated field declarations
│   ├── USER_METHODS section (preserved)
│   ├── USER_PROTECTED section (preserved)
│   └── USER_PRIVATE section (preserved)
└── Namespace close
```

### MessageName.cpp
```
├── Header include
├── Cap'n Proto includes
├── USER_IMPL_INCLUDES section (preserved)
├── Namespace declaration
├── Constructor implementations
│   └── USER_CONSTRUCTOR section (preserved)
├── Copy/Move constructors
├── Assignment operators
├── MessageBase interface implementation
│   ├── get_message_id()
│   ├── get_message_name()
│   ├── serialize()
│   └── deserialize()
├── Cap'n Proto conversion implementation
│   ├── to_capnp() with USER_TO_CAPNP section (preserved)
│   └── from_capnp() with USER_FROM_CAPNP section (preserved)
├── _copy_from() helper with USER_COPY_FROM section (preserved)
├── USER_IMPL section (preserved)
└── Namespace close
```

## Conversion Logic Details

### Primitive Fields
```cpp
// DSL: int32 count;
// to_capnp:
root.setCount(count);

// from_capnp:
count = root.getCount();
```

### String Fields
```cpp
// DSL: string text;
// to_capnp:
root.setText(text);

// from_capnp:
if (root.hasText())
{
    text = root.getText().cStr();
}
```

### Vector Fields
```cpp
// DSL: vector<int32> values;
// to_capnp:
if (!values.empty())
{
    auto list_builder = root.initValues(values.size());
    for (size_t i = 0; i < values.size(); ++i)
    {
        list_builder.set(i, values[i]);
    }
}

// from_capnp:
if (root.hasValues())
{
    auto list_reader = root.getValues();
    values.clear();
    values.reserve(list_reader.size());
    for (const auto& item : list_reader)
    {
        values.push_back(item);
    }
}
```

### Map Fields
```cpp
// DSL: map<string, int32> scores;
// to_capnp:
if (!scores.empty())
{
    auto map_builder = root.initScores();
    auto entries_builder = map_builder.initEntries(scores.size());
    size_t idx = 0;
    for (const auto& [key, value] : scores)
    {
        auto entry_builder = entries_builder[idx++];
        entry_builder.setKey(key);
        entry_builder.setValue(value);
    }
}

// from_capnp:
if (root.hasScores())
{
    auto map_reader = root.getScores();
    scores.clear();
    if (map_reader.hasEntries())
    {
        auto entries = map_reader.getEntries();
        for (const auto& entry : entries)
        {
            scores[entry.getKey()] = entry.getValue();
        }
    }
}
```

### Nested Message Fields
```cpp
// DSL: DataMessage data;
// to_capnp:
{
    auto nested_builder = root.initData();
    data.to_capnp_struct(nested_builder);
}

// from_capnp:
if (root.hasData())
{
    data.from_capnp_struct(root.getData());
}
```

### Enum Fields
```cpp
// DSL: Status status;
// to_capnp:
root.setStatus(static_cast<NetworkMsg::Status>(status));

// from_capnp:
status = static_cast<Status>(root.getStatus());
```

## Best Practices

1. **Always use the base class pointer for polymorphism**:
   ```cpp
   std::unique_ptr<MessageBase> msg = std::make_unique<DataMessage>();
   auto data = msg->serialize();
   ```

2. **Check deserialization return value**:
   ```cpp
   if (!msg.deserialize(data))
   {
       // Handle error
   }
   ```

3. **Use move semantics for large messages**:
   ```cpp
   DataMessage create_message()
   {
       DataMessage msg;
       msg.values = {1, 2, 3, 4, 5};
       return msg; // RVO/move
   }

   auto msg = create_message(); // No copy
   ```

4. **Add validation in USER sections**:
   ```cpp
   // Always validate before serializing
   if (msg.validate())
   {
       auto data = msg.serialize();
   }
   ```

5. **Keep user code in marked sections**:
   - Never modify generated code outside USER sections
   - Generator preserves all USER sections during regeneration

## Troubleshooting

### Issue: Compilation errors about missing Cap'n Proto headers

**Solution**: Ensure Cap'n Proto is installed and the generated `.capnp.h` files are in your include path.

```bash
# Compile the Cap'n Proto schema first
capnp compile -oc++ network_msg.capnp
```

### Issue: Linker errors about undefined references

**Solution**: Link against Cap'n Proto libraries:

```cmake
target_link_libraries(your_target PRIVATE capnp kj)
```

### Issue: User code disappeared after regeneration

**Solution**: Ensure your code is between the correct markers:

```cpp
// ✅ Correct - Will be preserved
// USER_METHODS_START
bool validate() const;
// USER_METHODS_END

// ❌ Wrong - Will be overwritten
bool validate() const; // Outside markers
```

## Performance Considerations

- **Serialization**: O(n) where n is the total size of data
- **Deserialization**: O(n) with zero-copy for Cap'n Proto read
- **Memory**: Generated classes use standard C++ containers
- **Move operations**: Fully optimized with noexcept move constructors

## Future Enhancements

Potential improvements for future versions:

- [ ] JSON serialization support
- [ ] Validation annotations in DSL
- [ ] Custom allocators support
- [ ] Async serialization APIs
- [ ] Schema evolution helpers
- [ ] Automatic test generation

## Support

For issues or questions:
1. Check the generated code for USER sections
2. Verify Cap'n Proto schema compiles correctly
3. Review type mappings in this guide
4. Ensure all dependencies are installed

## License

Same as the main project.

# Command-Line Interface Usage Guide

## Overview

The Cap'n Proto Network Generator provides a flexible command-line interface for generating Cap'n Proto schemas and C++ wrapper classes with proper include path handling.

## Command-Line Arguments

### Required Arguments

| Argument | Short | Description |
|----------|-------|-------------|
| `--input <file>` | `-i` | Input DSL schema file |
| `--out-capnp <path>` | `-ocapnp` | Output path for Cap'n Proto schema (directory or .capnp file) |

### Optional Arguments

| Argument | Short | Description |
|----------|-------|-------------|
| `--out-hpp <dir>` | `-ohpp` | Output directory for C++ header files (.hpp) |
| `--out-cpp <dir>` | `-ocpp` | Output directory for C++ source files (.cpp) |
| `--help` | `-h` | Show help message |

**Note**: Both `-ohpp` and `-ocpp` must be specified together if you want to generate C++ wrapper classes.

## Basic Usage Examples

### 1. Generate Only Cap'n Proto Schema

```bash
capnp_generator -i schema.dsl -ocapnp output/
```

This generates only the `.capnp` file without C++ wrappers.

### 2. Generate Cap'n Proto Schema + C++ Wrappers in Same Directory

```bash
capnp_generator -i schema.dsl -ocapnp output/ -ohpp src/gen/ -ocpp src/gen/
```

Both `.hpp` and `.cpp` files will be generated in `src/gen/`.

**Generated includes:**
```cpp
#include "gen/MessageName.hpp"
```

### 3. Generate with Separate Header/Source Directories

```bash
capnp_generator -i schema.dsl -ocapnp output/ -ohpp include/network/ -ocpp src/network/
```

Headers go to `include/network/`, sources go to `src/network/`.

**Generated includes:**
```cpp
#include "network/MessageName.hpp"
```

## Include Path Handling

The generator automatically extracts the **last folder name** from the `-ohpp` path and uses it as the include prefix in generated `.cpp` files.

### Examples

| `-ohpp` Path | Generated Include Statement |
|--------------|---------------------------|
| `include/network/` | `#include "network/MessageName.hpp"` |
| `include/company/project/messages/` | `#include "messages/MessageName.hpp"` |
| `src/gen/` | `#include "gen/MessageName.hpp"` |
| `output/` | `#include "output/MessageName.hpp"` |

This allows you to organize your project structure flexibly while maintaining correct include paths.

## Recommended Project Structures

### Structure 1: Separate Include and Source

```
my_project/
├── include/
│   └── messages/          ← Headers go here (-ohpp)
│       ├── BaseMessage.hpp
│       ├── DataMessage.hpp
│       └── ...
├── src/
│   └── messages/          ← Sources go here (-ocpp)
│       ├── BaseMessage.cpp
│       ├── DataMessage.cpp
│       └── ...
└── build/
    └── output/            ← Cap'n Proto schemas (-ocapnp)
        └── network_msg.capnp
```

**Command:**
```bash
capnp_generator -i schema.dsl -ocapnp build/output/ -ohpp include/messages/ -ocpp src/messages/
```

**Result:**
- `.cpp` files use: `#include "messages/MessageName.hpp"`
- Your code uses: `#include <messages/MessageName.hpp>`

### Structure 2: Unified Generated Directory

```
my_project/
├── src/
│   ├── main.cpp
│   └── generated/         ← Both headers and sources (-ohpp and -ocpp)
│       ├── BaseMessage.hpp
│       ├── BaseMessage.cpp
│       ├── DataMessage.hpp
│       ├── DataMessage.cpp
│       └── ...
└── schemas/
    └── output/            ← Cap'n Proto schemas (-ocapnp)
        └── network_msg.capnp
```

**Command:**
```bash
capnp_generator -i schema.dsl -ocapnp schemas/output/ -ohpp src/generated/ -ocpp src/generated/
```

**Result:**
- `.cpp` files use: `#include "generated/MessageName.hpp"`
- Your code uses: `#include <generated/MessageName.hpp>`

### Structure 3: Deep Nesting

```
my_project/
├── include/
│   └── company/
│       └── project/
│           └── network/   ← Headers go here (-ohpp)
│               └── messages/
│                   ├── BaseMessage.hpp
│                   └── ...
└── src/
    └── company/
        └── project/
            └── network/   ← Sources go here (-ocpp)
                └── messages/
                    ├── BaseMessage.cpp
                    └── ...
```

**Command:**
```bash
capnp_generator -i schema.dsl -ocapnp build/ \
    -ohpp include/company/project/network/messages/ \
    -ocpp src/company/project/network/messages/
```

**Result:**
- `.cpp` files use: `#include "messages/MessageName.hpp"`
- Your code uses: `#include <company/project/network/messages/MessageName.hpp>`

**Note**: Only the **last folder name** (`messages/`) is used in the generated includes!

## Integration with Build Systems

### CMake Example

```cmake
# Add include directory
include_directories(include)

# Generate files
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated.stamp
    COMMAND capnp_generator
        -i ${CMAKE_SOURCE_DIR}/schema.dsl
        -ocapnp ${CMAKE_BINARY_DIR}/capnp
        -ohpp ${CMAKE_SOURCE_DIR}/include/messages
        -ocpp ${CMAKE_SOURCE_DIR}/src/messages
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/generated.stamp
    DEPENDS ${CMAKE_SOURCE_DIR}/schema.dsl
    COMMENT "Generating Cap'n Proto and C++ wrapper classes"
)

add_custom_target(generate_messages DEPENDS ${CMAKE_BINARY_DIR}/generated.stamp)

# Compile Cap'n Proto schemas
execute_process(
    COMMAND capnp compile -oc++ ${CMAKE_BINARY_DIR}/capnp/network_msg.capnp
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Add generated sources
file(GLOB MESSAGE_SOURCES "src/messages/*.cpp")
add_library(messages ${MESSAGE_SOURCES})
target_link_libraries(messages PRIVATE capnp kj)

add_dependencies(messages generate_messages)
```

### Makefile Example

```makefile
GENERATOR = ./capnp_generator
SCHEMA = schema.dsl
CAPNP_OUT = build/capnp
HPP_OUT = include/messages
CPP_OUT = src/messages

.PHONY: all generate clean

all: generate

generate:
	$(GENERATOR) -i $(SCHEMA) -ocapnp $(CAPNP_OUT) -ohpp $(HPP_OUT) -ocpp $(CPP_OUT)
	capnp compile -oc++ $(CAPNP_OUT)/network_msg.capnp

clean:
	rm -rf $(CAPNP_OUT) $(HPP_OUT) $(CPP_OUT)
```

## Complete Workflow Example

```bash
# 1. Create your DSL schema
cat > messages.dsl << EOF
namespace my::app::messages;

enum Status {
    OK | 0,
    ERROR | 1
}

message BaseMessage (100) {
    int64 timestamp;
    string sender;
}

message DataMessage (101) extends BaseMessage {
    vector<int32> values;
    Status status;
}
EOF

# 2. Generate all files
capnp_generator -i messages.dsl \
    -ocapnp build/capnp \
    -ohpp include/messages \
    -ocpp src/messages

# 3. Compile Cap'n Proto schema
cd build/capnp
capnp compile -oc++ network_msg.capnp
cd ../..

# 4. Use in your code
cat > main.cpp << EOF
#include <iostream>
#include <messages/DataMessage.hpp>

int main() {
    // Create message
    my::app::messages::DataMessage msg;
    msg.timestamp = 123456;
    msg.sender = "server1";
    msg.values = {1, 2, 3, 4, 5};
    msg.status = my::app::messages::Status::OK;

    // Serialize
    auto data = msg.serialize();
    std::cout << "Serialized " << data.size() << " bytes\n";

    // Deserialize
    my::app::messages::DataMessage received;
    if (received.deserialize(data)) {
        std::cout << "Received from: " << received.sender << "\n";
        std::cout << "Values: " << received.values.size() << " items\n";
    }

    return 0;
}
EOF

# 5. Compile
g++ -std=c++20 main.cpp src/messages/*.cpp \
    -I include -I build/capnp \
    -lcapnp -lkj -o app

# 6. Run
./app
```

## Error Messages

### Missing Required Argument

```
Error: Missing required argument --input/-i
```

**Solution**: Provide the input DSL file with `-i` or `--input`.

### Missing Cap'n Proto Output

```
Error: Missing required argument --out-capnp/-ocapnp
```

**Solution**: Specify where to generate the Cap'n Proto schema with `-ocapnp`.

### Incomplete C++ Generation Arguments

```
Error: --out-hpp/-ohpp is required when generating C++ files
```

**Solution**: If you specify `-ocpp`, you must also specify `-ohpp` (and vice versa).

## Tips and Best Practices

1. **Use meaningful directory names**: The last folder name becomes part of your include path, so choose descriptive names like `messages`, `network`, `protocol`, etc.

2. **Keep headers and sources separate**: This follows C++ best practices and allows better organization.

3. **Use relative paths**: Makes your build system more portable.

4. **Add to version control**: The generator preserves USER sections, so generated files can be safely committed.

5. **Regenerate after schema changes**: Always regenerate when you modify the DSL schema.

6. **Check include paths**: After generation, verify the include statements in `.cpp` files match your project structure.

## Troubleshooting

### Issue: Wrong include path in generated files

**Symptom**: Generated `.cpp` files have incorrect include paths.

**Solution**: The generator uses the **last folder** from `-ohpp`. If you have:
```
-ohpp include/company/project/network/
```

It will generate:
```cpp
#include "network/MessageName.hpp"
```

Adjust your `-ohpp` path or project structure accordingly.

### Issue: Compiler can't find headers

**Symptom**: `fatal error: messages/MessageName.hpp: No such file or directory`

**Solution**: Add the parent directory to your include path:
```bash
g++ ... -I include  # If headers are in include/messages/
```

### Issue: Files regenerated but user code is lost

**Symptom**: Custom code disappeared after regeneration.

**Solution**: Ensure your code is between USER markers:
```cpp
// USER_METHODS_START
// Your code here
// USER_METHODS_END
```

## See Also

- `CPP_GENERATION_GUIDE.md` - Detailed C++ wrapper class documentation
- `README.md` - Project overview
- Cap'n Proto documentation: https://capnproto.org/

## Support

For issues:
1. Run with `--help` to see all options
2. Check that both `-ohpp` and `-ocpp` are specified for C++ generation
3. Verify the last folder name in `-ohpp` matches your desired include path
4. Ensure all paths are valid and writable

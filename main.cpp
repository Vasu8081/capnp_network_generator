#include "capnp_file_generator.hpp"
#include "cpp_header_generator.hpp"
#include "cpp_source_generator.hpp"
#include "schema.hpp"

#include <iostream>
#include <string>
#include <map>

namespace
{

/// @brief Parse command-line arguments into a map.
/// @param argc Argument count.
/// @param argv Argument vector.
/// @return Map of argument name to value.
std::map<std::string, std::string> parse_arguments(int argc, char** argv)
{
    std::map<std::string, std::string> args;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--input" || arg == "-i")
        {
            if (i + 1 < argc)
            {
                args["input"] = argv[++i];
            }
        }
        else if (arg == "--out-capnp" || arg == "-ocapnp")
        {
            if (i + 1 < argc)
            {
                args["out-capnp"] = argv[++i];
            }
        }
        else if (arg == "--out-hpp" || arg == "-ohpp")
        {
            if (i + 1 < argc)
            {
                args["out-hpp"] = argv[++i];
            }
        }
        else if (arg == "--out-cpp" || arg == "-ocpp")
        {
            if (i + 1 < argc)
            {
                args["out-cpp"] = argv[++i];
            }
        }
        else if (arg == "--help" || arg == "-h")
        {
            args["help"] = "true";
        }
    }

    return args;
}

/// @brief Print usage information.
/// @param program_name The name of the executable.
void print_usage(const char* program_name)
{
    std::cout << "Cap'n Proto Network Message Generator\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Required Options:\n";
    std::cout << "  -i, --input <file>       Input DSL schema file\n";
    std::cout << "  -ocapnp, --out-capnp <path>\n";
    std::cout << "                           Output path for Cap'n Proto schema\n";
    std::cout << "                           (directory or .capnp file)\n\n";
    std::cout << "Optional Options:\n";
    std::cout << "  -ohpp, --out-hpp <dir>   Output directory for C++ header files (.hpp)\n";
    std::cout << "  -ocpp, --out-cpp <dir>   Output directory for C++ source files (.cpp)\n";
    std::cout << "  -h, --help               Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  # Generate only Cap'n Proto schema:\n";
    std::cout << "  " << program_name << " -i schema.dsl -ocapnp output/\n\n";
    std::cout << "  # Generate Cap'n Proto schema and C++ wrappers:\n";
    std::cout << "  " << program_name << " -i schema.dsl -ocapnp output/ -ohpp include/messages/ -ocpp src/messages/\n\n";
    std::cout << "  # Headers and sources in same directory:\n";
    std::cout << "  " << program_name << " -i schema.dsl -ocapnp output/ -ohpp src/gen/ -ocpp src/gen/\n\n";
    std::cout << "Include Path Handling:\n";
    std::cout << "  If -ohpp is 'include/network/', generated .cpp files will use:\n";
    std::cout << "    #include \"network/MessageName.hpp\"\n";
    std::cout << "  This allows proper include paths in your build system.\n";
}

/// @brief Extract the last folder name from a path for include statements.
/// @param path The full path (e.g., "include/network/messages/").
/// @return The last folder name (e.g., "messages") or empty if path is invalid.
std::string extract_include_folder(const std::string& path)
{
    if (path.empty())
    {
        return "";
    }

    // Remove trailing slashes
    std::string normalized = path;
    while (!normalized.empty() && (normalized.back() == '/' || normalized.back() == '\\'))
    {
        normalized.pop_back();
    }

    if (normalized.empty())
    {
        return "";
    }

    // Find the last separator
    std::size_t last_sep = normalized.find_last_of("/\\");
    if (last_sep == std::string::npos)
    {
        // No separator, return the whole string
        return normalized;
    }

    // Return everything after the last separator
    return normalized.substr(last_sep + 1);
}

} // anonymous namespace

/// @brief Entry point for the Cap'n Proto network message generator.
/// @param argc Argument count.
/// @param argv Argument vector.
/// @return 0 on success, non-zero on error.
int main(int argc, char** argv)
{
    using namespace curious::dsl::capnpgen;

    // Parse arguments
    auto args = parse_arguments(argc, argv);

    // Check for help
    if (args.count("help") || argc == 1)
    {
        print_usage(argv[0]);
        return 0;
    }

    // Validate required arguments
    if (!args.count("input"))
    {
        std::cerr << "Error: Missing required argument --input/-i\n\n";
        print_usage(argv[0]);
        return 1;
    }

    if (!args.count("out-capnp"))
    {
        std::cerr << "Error: Missing required argument --out-capnp/-ocapnp\n\n";
        print_usage(argv[0]);
        return 1;
    }

    const std::string input_file = args["input"];
    const std::string capnp_output = args["out-capnp"];
    const std::string hpp_output = args.count("out-hpp") ? args["out-hpp"] : "";
    const std::string cpp_output = args.count("out-cpp") ? args["out-cpp"] : "";

    // Validate C++ generation arguments
    bool generate_cpp = !hpp_output.empty() || !cpp_output.empty();
    if (generate_cpp)
    {
        if (hpp_output.empty())
        {
            std::cerr << "Error: --out-hpp/-ohpp is required when generating C++ files\n";
            return 1;
        }
        if (cpp_output.empty())
        {
            std::cerr << "Error: --out-cpp/-ocpp is required when generating C++ files\n";
            return 1;
        }
    }

    try
    {
        // Parse the DSL schema
        std::cout << "Parsing DSL file: " << input_file << "\n";
        Schema schema;
        schema.parse_from_file(input_file);

        std::cout << "✓ Parsed " << schema.messages.size() << " message(s) and "
                  << schema.enums.size() << " enum(s)\n\n";

        // Generate Cap'n Proto schema file
        std::cout << "Generating Cap'n Proto schema...\n";
        CapnpFileGenerator capnp_generator(schema, capnp_output);
        std::cout << "✓ Generated Cap'n Proto schema: " << capnp_output << "\n\n";

        // Generate C++ wrapper classes if requested
        if (generate_cpp)
        {
            // Extract include folder name for proper #include directives
            std::string include_folder = extract_include_folder(hpp_output);
            std::string include_prefix = include_folder.empty() ? "" : (include_folder + "/");

            std::cout << "Generating C++ wrapper classes...\n";
            std::cout << "  Header output: " << hpp_output << "\n";
            std::cout << "  Source output: " << cpp_output << "\n";

            if (!include_folder.empty())
            {
                std::cout << "  Include prefix: " << include_prefix << "\n";
            }

            // Generate headers
            CppHeaderGenerator header_generator(schema, hpp_output);
            std::cout << "✓ Generated " << schema.messages.size() << " header file(s)\n";

            // Generate sources with include prefix
            CppSourceGenerator source_generator(schema, cpp_output, "network_msg.capnp.h", include_prefix);
            std::cout << "✓ Generated " << schema.messages.size() << " source file(s)\n\n";

            std::cout << "C++ Usage Example:\n";
            std::cout << "  #include <" << include_prefix << "MessageName.hpp>\n";
            std::cout << "  \n";
            std::cout << "  // Create message\n";
            std::cout << "  MessageName msg;\n";
            std::cout << "  msg.field = value;\n";
            std::cout << "  \n";
            std::cout << "  // Serialize\n";
            std::cout << "  auto data = msg.serialize();\n";
            std::cout << "  \n";
            std::cout << "  // Deserialize\n";
            std::cout << "  MessageName received;\n";
            std::cout << "  received.deserialize(data);\n\n";
        }

        std::cout << "✓ Generation complete!\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "\nError: " << ex.what() << "\n";
        return 2;
    }
}

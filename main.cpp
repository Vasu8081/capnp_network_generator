#include "capnp_file_generator.hpp"
#include "schema.hpp"

#include <iostream>

/// @brief Entry point for the Cap'n Proto network message generator.
/// @param argc Argument count.
/// @param argv Argument vector.
/// @return 0 on success, non-zero on error.
int main(int argc, char** argv)
{
    using namespace curious::dsl::capnpgen;

    // Check command line arguments
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input.dsl> <output-dir-or-file.capnp>\n";
        std::cerr << "\n";
        std::cerr << "Arguments:\n";
        std::cerr << "  <input.dsl>              Path to the input DSL file\n";
        std::cerr << "  <output-dir-or-file>     Output path (directory or .capnp file)\n";
        std::cerr << "\n";
        std::cerr << "Example:\n";
        std::cerr << "  " << argv[0] << " schema.dsl output/\n";
        std::cerr << "  " << argv[0] << " schema.dsl output/network.capnp\n";
        return 1;
    }

    const std::string input_file_path = argv[1];
    const std::string output_path = argv[2];

    try
    {
        // Parse the DSL schema
        std::cout << "Parsing DSL file: " << input_file_path << "\n";
        Schema schema;
        schema.parse_from_file(input_file_path);

        std::cout << "Parsed " << schema.messages.size() << " message(s) and "
                  << schema.enums.size() << " enum(s)\n";

        // Generate Cap'n Proto schema file
        std::cout << "Generating Cap'n Proto schema...\n";
        CapnpFileGenerator capnp_generator(schema, output_path);

        std::cout << "Successfully generated Cap'n Proto schema!\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
}

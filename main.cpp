#include "schema.hpp"
#include "generator.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    using namespace curious::dsl::capnpgen;

    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input.dsl> <output-dir-or-file.capnp>\n";
        return 1;
    }

    const std::string input_path  = argv[1];
    const std::string output_path = argv[2];

    try
    {
        Schema sch;
        sch.parse(input_path);

        CapnpFileGenerator gen(sch, output_path);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }

    return 0;
}

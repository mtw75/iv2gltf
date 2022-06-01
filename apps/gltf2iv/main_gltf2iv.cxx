#include "GltfIvWriter.h"

#include <cxxopts.hpp>
#include <tiny_gltf.h>
#include <Inventor/SoDB.h>

#include <iostream>
#include <optional>

int main(int argc, char * argv[])
{
    cxxopts::Options options("gltf2iv", "a converter for gltf to open inventor");
    options.add_options()
        ("i,gltf", "input gltf file", cxxopts::value<std::string>())
        ("o,iv", "output open inventor file", cxxopts::value<std::string>())
        ("h,help", "print usage")
        ;

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    }
    catch (const cxxopts::OptionParseException & x) {
        std::cerr << "error: " << x.what() << '\n';
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return EXIT_SUCCESS;
    }

    if (result.count("i") && result.count("o")) {

        const std::string inputFilename{ result["i"].as<std::string>() };

        std::optional<tinygltf::Model> maybeGltfModel{ GltfIv::read(inputFilename) };

        if (maybeGltfModel.has_value()) {

            SoDB::init();

            GltfIvWriter writer{std::move(maybeGltfModel.value())};

            const std::string outputFilename{ result["o"].as<std::string>() };

            if (!writer.write(outputFilename)) {
                return EXIT_FAILURE;
            }
        }
        else {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
#include "GltfIvWriter.h"

#include <spdlog/spdlog.h>
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
        ("b,binary", "write binary", cxxopts::value<bool>()->default_value("false"))
        ("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "print usage")
        ;

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    }
    catch (const cxxopts::OptionParseException & x) {
        spdlog::error("error parsing command line options: {}", x.what());
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        return EXIT_SUCCESS;
    }

    if (!result.count("i")) {
        spdlog::error("missing command line option 'i'");
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    if (!result.count("o")) {
        spdlog::error("missing command line option 'o'");
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    const bool verbose{ result["v"].as<bool>() };

    if (verbose) {
        spdlog::set_level(spdlog::level::trace);
    }
    else {
        spdlog::set_level(spdlog::level::warn);
    }

    SoDB::init();
    
    const std::string inputFilename{ result["i"].as<std::string>() };
    const std::string outputFilename{ result["o"].as<std::string>() };
    const bool writeBinary{ result["b"].as<bool>() };

    spdlog::info("converting {} to {} (binary: {}) ", inputFilename, outputFilename, writeBinary);

    std::optional<tinygltf::Model> maybeGltfModel{ GltfIv::read(inputFilename) };
    
    if (maybeGltfModel.has_value()) {
        
        GltfIvWriter writer{std::move(maybeGltfModel.value())};
        
        if (writer.write(outputFilename, writeBinary)) {
            spdlog::info("successfully converted {} to {}", inputFilename, outputFilename);
            return EXIT_SUCCESS;
        }
        else {
            spdlog::warn("failed to convert {} to {}", inputFilename, outputFilename);
            return EXIT_FAILURE;
        }
    } 
    else {
        spdlog::warn("failed to read {}", inputFilename);
        return EXIT_FAILURE;
    }
}

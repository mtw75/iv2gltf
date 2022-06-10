#include "GltfIv.h"

#include <spdlog/stopwatch.h>

#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>

#include <iostream>

std::optional<tinygltf::Model> GltfIv::read(std::string filename) 
{
    spdlog::stopwatch stopwatch;
    spdlog::trace("reading gltf model from file {}", filename);
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string error_message;
    std::string warning_message;
    bool success{ false };

    if (filename.ends_with(".gltf")) {
        spdlog::debug("reading gltf file {} as ascii", filename);
        success = loader.LoadASCIIFromFile(&model, &error_message, &warning_message, filename);
    }
    else if (filename.ends_with(".glb")) {
        spdlog::debug("reading gltf file {} as binary", filename);
        success = loader.LoadBinaryFromFile(&model, &error_message, &warning_message, filename);
    }
    else {
        spdlog::error("unknown gltf file type. supported types are .gltf and .glb");
        return std::nullopt;
    }

    if (!error_message.empty()) {
        spdlog::error("reading gltf file: {}", error_message);
    }

    if (!warning_message.empty()) {
        spdlog::warn("reading gltf file: {}", warning_message);
    }

    if (!success) {
        spdlog::error("failed to read gltf file");
        return std::nullopt;
    }
    spdlog::debug("successfully read gltf model from file {} ({} seconds)", filename, stopwatch);
    return model;
}

bool GltfIv::write(std::string filename, SoSeparator * root, bool isBinary)
{
    spdlog::stopwatch stopwatch;
    spdlog::trace("writing open inventor model to file {} as {})", filename, (isBinary ? "binary" : "ascii"));

    SoOutput out;
    if (!out.openFile(filename.c_str())) {
        spdlog::error("failed to open file {}", filename);
        return false;
    }
    SoWriteAction wa(&out);
    out.setBinary(isBinary);
    wa.apply(root);
    out.closeFile();

    spdlog::debug("successfully wrote open inventor model to file {} as {} ({} seconds)", filename, (isBinary ? "binary" : "ascii"), stopwatch);
    return true;
}

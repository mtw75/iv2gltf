#include "GltfIv.h"

#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/SoOutput.h>

#include <iostream>

std::optional<tinygltf::Model> GltfIv::read(std::string filename) 
{
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
    spdlog::debug("successfully read gltf file {}", filename);
    return model;
}

bool GltfIv::write(std::string filename, SoSeparator * root, bool isBinary)
{
    spdlog::debug("writing open inventor model to file {} (binary: {})", filename, isBinary);

    SoOutput out;
    if (!out.openFile(filename.c_str())) {
        spdlog::error("failed to open file {}", filename);
        return false;
    }
    SoWriteAction wa(&out);
    out.setBinary(isBinary);
    wa.apply(root);
    out.closeFile();

    spdlog::debug("successfully wrote open inventor model to file {} (binary: {})", filename, isBinary);
    return true;
}

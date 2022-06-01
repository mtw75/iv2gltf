#include "GltfIv.h"

#include <iostream>

std::optional<tinygltf::Model> GltfIv::read(std::string filename) 
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string error_message;
    std::string warning_message;
    bool success{ false };

    if (filename.ends_with(".gltf")) {
        success = loader.LoadASCIIFromFile(&model, &error_message, &warning_message, filename);
    }
    else if (filename.ends_with(".glb")) {
        success = loader.LoadBinaryFromFile(&model, &error_message, &warning_message, filename);
    }
    else {
        std::cerr << "error: unknown gltf file type";
        return std::nullopt;
    }

    if (!error_message.empty()) {
        std::cerr << "error: " << error_message << '\n';
    }

    if (!warning_message.empty()) {
        std::clog << "warning: " << warning_message << '\n';
    }

    if (!success) {
        std::cerr << "failed to parse gltf file";
        return std::nullopt;
    }

    return model;
}

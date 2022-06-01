#pragma once 

#include <tiny_gltf.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <string>
#include <optional>

class GltfIv
{
public:
    static std::optional<tinygltf::Model> read(std::string filename);
};
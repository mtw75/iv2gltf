#pragma once 

#include <tiny_gltf.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <Inventor/nodes/SoSeparator.h>

#include <string>
#include <optional>

class GltfIv
{
public:
    static std::optional<tinygltf::Model> read(std::string filename);
    static bool write(std::string filename, SoSeparator * root, bool isBinary);
};
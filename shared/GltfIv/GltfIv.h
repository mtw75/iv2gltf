#pragma once 

#include <tiny_gltf.h>

#include <string>
#include <optional>

class GltfIv
{
public:
    static std::optional<tinygltf::Model> read(std::string filename);
};
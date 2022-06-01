#pragma once

#include "GltfIv.h"

class GLTFIV_EXPORT GltfIvWriter {
public:
    GltfIvWriter(tinygltf::Model && gltf_model);
    ~GltfIvWriter();
    bool write(std::string filename);
};
#pragma once

#include "GltfIv.h"

class GltfIvWriter {
public:
    GltfIvWriter(tinygltf::Model && gltfModel);
    ~GltfIvWriter();
    bool write(std::string filename, bool writeBinary);

private:
    const tinygltf::Model m_gltfModel;
    SoSeparator * m_ivModel{ nullptr };
};
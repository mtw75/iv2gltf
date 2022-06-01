#include "GltfIvWriter.h"

GltfIvWriter::GltfIvWriter(tinygltf::Model && gltfModel)
    : m_gltfModel{ std::move(gltfModel) }
{

}

GltfIvWriter::~GltfIvWriter()
{

}

bool GltfIvWriter::write(std::string filename)
{
    return false;
}
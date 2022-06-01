#include "GltfIvWriter.h"

GltfIvWriter::GltfIvWriter(tinygltf::Model && gltfModel)
    : m_gltfModel{ std::move(gltfModel) }
    , m_ivModel{ new SoSeparator() }
{
    m_ivModel->ref();
}

GltfIvWriter::~GltfIvWriter()
{
    if (m_ivModel) {
        m_ivModel->unref();
    }
}

bool GltfIvWriter::write(std::string filename)
{
    return false;
}
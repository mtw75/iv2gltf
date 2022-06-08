#include "GltfIvWriter.h"

#include <set>
#include <stdexcept>

GltfIvWriter::GltfIvWriter(tinygltf::Model && gltfModel)
    : m_gltfModel{ std::move(gltfModel) }
    , m_ivModel{ new SoSeparator() }
    , m_positionIndexMap{ }
{
    m_ivModel->ref();
}

GltfIvWriter::~GltfIvWriter()
{
    if (m_ivModel) {
        m_ivModel->unref();
    }
}

bool GltfIvWriter::write(std::string filename, bool writeBinary)
{
    if (!m_ivModel) {
        spdlog::error("open inventor model uninitilized");
        return false;
    }

    try {
        return convertModel() && GltfIv::write(filename, m_ivModel, writeBinary);
    }
    catch (std::exception exception) {
        spdlog::error(std::format("failed to convert model: {}", exception.what()));
        return false;
    }
}

bool GltfIvWriter::convertModel()
{
    spdlog::trace("converting gltf model to open inventor model");
    return std::all_of(
        m_gltfModel.scenes.cbegin(), 
        m_gltfModel.scenes.cend(), 
        [this](const tinygltf::Scene & scene) { return this->convertScene(scene); }
        );
}

bool GltfIvWriter::convertScene(const tinygltf::Scene & scene)
{
    spdlog::trace("converting scene with name '{}'", scene.name);

    return convertNodes(scene.nodes);
}

bool GltfIvWriter::convertNodes(const std::vector<int> nodeIndices)
{
    return std::all_of(
        nodeIndices.cbegin(),
        nodeIndices.cend(),
        [this] (int nodeIndex)
        {
            return this->convertNode(nodeIndex);
        }
    );
}

bool GltfIvWriter::convertNode(int nodeIndex)
{
    spdlog::trace("converting node with index {}", nodeIndex);

    if (nodeIndex < 0 || nodeIndex >= m_gltfModel.nodes.size()) {
        spdlog::error("node index {} out of bounds [0, {})", nodeIndex, m_gltfModel.nodes.size());
        return false;
    }

    return convertNode(m_gltfModel.nodes[nodeIndex]);
}

bool GltfIvWriter::convertNode(const tinygltf::Node & node)
{
    spdlog::trace("converting node with name '{}'", node.name);

    return convertMesh(node.mesh) && convertNodes(node.children);
}

bool GltfIvWriter::convertMesh(int meshIndex)
{
    spdlog::trace("converting mesh with index {}", meshIndex);

    if (meshIndex < 0 || meshIndex >= m_gltfModel.meshes.size()) {
        spdlog::error("mesh index {} out of bounds [0, {})", meshIndex, m_gltfModel.meshes.size());
        return false;
    }

    return convertMesh(m_gltfModel.meshes[meshIndex]);
}

bool GltfIvWriter::convertMesh(const tinygltf::Mesh & mesh)
{
    spdlog::trace("converting mesh with name '{}'", mesh.name);

    return convertPrimitives(mesh.primitives);
}

bool GltfIvWriter::convertPrimitives(const std::vector<tinygltf::Primitive> & primitives)
{
    return std::all_of(
        primitives.cbegin(),
        primitives.cend(),
        [this] (const tinygltf::Primitive & primitive) { return this->convertPrimitive(primitive); }
    );
}

bool GltfIvWriter::convertPrimitive(const tinygltf::Primitive & primitive)
{
    spdlog::trace("converting primitive with mode {}", primitive.mode);

    switch (primitive.mode) {
    case TINYGLTF_MODE_TRIANGLES:
        return convertTrianglesPrimitive(primitive);
    default:
        spdlog::warn("skipping unsupported primitive with mode {}", primitive.mode);
        return true;
    }
}

bool GltfIvWriter::convertTrianglesPrimitive(const tinygltf::Primitive & primitive)
{
    spdlog::trace("converting triangles primitive");    

    const std::vector<position_t> positions{ this->positions(primitive) };
    const std::vector<int> indices{ this->indices(primitive) };

    m_ivModel->addChild(convertPositions(positions, indices));
    m_ivModel->addChild(convertTriangles(indices));

    return true;
}

void GltfIvWriter::ensureAccessorType(const tinygltf::Accessor & accessor, int accessorType) const
{
    if (accessor.type != accessorType) {
        throw std::domain_error(std::format("expected accessor type {} instead of {}", accessorType, accessor.type));
    }
}

std::vector<GltfIvWriter::position_t> GltfIvWriter::positions(const tinygltf::Primitive & primitive)
{
    spdlog::trace("retrieve positions from primitive");

    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.attributes.at("POSITION"));

    ensureAccessorType(accessor, TINYGLTF_TYPE_VEC3);

    return accessorContents<position_t>(accessor);
}

std::vector<GltfIvWriter::normal_t> GltfIvWriter::normals(const tinygltf::Primitive & primitive)
{
    spdlog::trace("retrieve normals from primitive");

    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.attributes.at("NORMAL"));

    ensureAccessorType(accessor, TINYGLTF_TYPE_VEC3);

    return accessorContents<normal_t>(accessor);
}

std::vector<GltfIvWriter::texture_coordinate_t> GltfIvWriter::textureCoordinates(const tinygltf::Primitive & primitive)
{
    spdlog::trace("retrieve texture coordinates from primitive");

    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.attributes.at("TEXCOORD_0"));

    ensureAccessorType(accessor, TINYGLTF_TYPE_VEC2);

    return accessorContents<texture_coordinate_t>(accessor);
}

std::vector<int> GltfIvWriter::indices(const tinygltf::Primitive & primitive)
{
    spdlog::trace("retrieve indices from primitive");

    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.indices);

    ensureAccessorType(accessor, TINYGLTF_TYPE_SCALAR);

    return accessorContents<int>(accessor);
}

size_t GltfIvWriter::countUnqiueIndexedPositions(const std::vector<position_t> & positions, const std::vector<int> & indices)
{
    std::set<position_t> indexedPositions{ };

    for (int index : indices) {
        indexedPositions.insert(positions.at(index));
    }

    return indexedPositions.size();
}

SoCoordinate3 * GltfIvWriter::convertPositions(const std::vector<position_t> & positions, const std::vector<int> & indices)
{
    SoCoordinate3 * coords = new SoCoordinate3;
    coords->point.setNum(static_cast<int>(countUnqiueIndexedPositions(positions, indices)));
    SbVec3f * ivPointsPtr = coords->point.startEditing();
    
    std::map<position_t, int32_t> mappedIndexByPosition;

    int32_t nextMappedIndex{ static_cast<int32_t>(m_positionIndexMap.size()) };

    for (int32_t index : indices) {

        if (m_positionIndexMap.contains(index)) {
            continue;
        }

        const position_t & position{ positions.at(index) };

        if (!mappedIndexByPosition.contains(position)) {
            mappedIndexByPosition[position] = nextMappedIndex++;

            float * ivPointPtr = (float *) ivPointsPtr;
            ivPointPtr[0] = position[0];
            ivPointPtr[1] = position[1];
            ivPointPtr[2] = position[2];
            ivPointsPtr++;
        }

        m_positionIndexMap[index] = mappedIndexByPosition[position];
    }

    coords->point.finishEditing();

    return coords;
}

SoIndexedTriangleStripSet * GltfIvWriter::convertTriangles(const std::vector<int> & indices)
{
    SoIndexedTriangleStripSet * triangles = new SoIndexedTriangleStripSet;

    SoMFInt32 coordIndex{};
    coordIndex.setNum(static_cast<int>(indices.size() + indices.size() / 4U));

    int32_t * posIndex = coordIndex.startEditing();

    for (size_t i = 0; i + 3 < indices.size(); i += 4) {
        *posIndex++ = m_positionIndexMap[indices[i]];
        *posIndex++ = m_positionIndexMap[indices[i + 1]];
        *posIndex++ = m_positionIndexMap[indices[i + 2]];
        *posIndex++ = m_positionIndexMap[indices[i + 3]];
        *posIndex++ = -1;
    }
    coordIndex.finishEditing();
    triangles->coordIndex = coordIndex;

    return triangles;
}
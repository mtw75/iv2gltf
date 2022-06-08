#include "GltfIvWriter.h"

#include <set>
#include <stdexcept>

GltfIvWriter::GltfIvWriter(tinygltf::Model && gltfModel)
    : m_gltfModel{ std::move(gltfModel) }
    , m_ivModel{ new SoSeparator() }
    , m_positionIndexMap{ }
    , m_normalMap{ }
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
    const std::vector<normal_t> normals{ this->normals(primitive) };
    const std::vector<index_t> indices{ this->indices(primitive) };

    m_ivModel->addChild(convertMaterial(primitive.material));

    SoMaterialBinding * materialBinding = new SoMaterialBinding;

    materialBinding->value = SoMaterialBinding::Binding::OVERALL;

    m_ivModel->addChild(materialBinding);

    const std::vector<position_t> uniquePositions{ unique<position_t>(positions) };

    m_ivModel->addChild(convertPositions(uniquePositions));

    updatePositionIndexMap(uniquePositions, positions, indices);

    SoNormalBinding * normalBinding = new SoNormalBinding;

    normalBinding->value = SoNormalBinding::Binding::PER_VERTEX_INDEXED;

    m_ivModel->addChild(normalBinding);

    m_ivModel->addChild(convertNormals(normals));

    m_ivModel->addChild(convertTriangles(indices, normals));

    return true;
}

int GltfIvWriter::indexTypeSize(const tinygltf::Primitive & primitive) const
{
    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.indices);
    const tinygltf::BufferView & bufferView = m_gltfModel.bufferViews.at(accessor.bufferView);

    return accessor.ByteStride(bufferView);
}

std::vector<GltfIvWriter::index_t> GltfIvWriter::indices(const tinygltf::Primitive & primitive)
{
    spdlog::trace("retrieve indices from primitive");

    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.indices);

    ensureAccessorType(accessor, TINYGLTF_TYPE_SCALAR);

    const int indexTypeSize{ this->indexTypeSize(primitive) };

    switch (indexTypeSize){
    case 1:
        return indices<uint8_t>(accessor);
    case 2:
        return indices<uint16_t>(accessor);
    case 4:
        return indices<uint32_t>(accessor);
    case 8:
        return indices<uint64_t>(accessor);
    default:
        throw std::invalid_argument(std::format("unsupported index size of {}", indexTypeSize));
    }
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

SoCoordinate3 * GltfIvWriter::convertPositions(const std::vector<position_t> & positions)
{
    SoCoordinate3 * coords = new SoCoordinate3;

    coords->point.setNum(static_cast<int>(positions.size()));
    SbVec3f * ivPointsPtr = coords->point.startEditing();
    
    std::memcpy(ivPointsPtr, &positions[0], sizeof(SbVec3f) * positions.size());

    coords->point.finishEditing();

    return coords;
}

void GltfIvWriter::updatePositionIndexMap(const std::vector<position_t> & uniquePositions, const std::vector<position_t> & positions, const std::vector<index_t> & indices)
{
    std::map<position_t, int32_t> positionMap{};

    for (int32_t i = 0; i < uniquePositions.size(); ++i) {
        positionMap[uniquePositions[i]] = i;
    }

    for (const index_t & index : indices) {
        m_positionIndexMap[index] = static_cast<uint64_t>(positionMap[positions[index]]);
    }
}

SoNormal * GltfIvWriter::convertNormals(const std::vector<normal_t> & normals)
{
    SoNormal * normalNode = new SoNormal;

    std::vector<normal_t> uniqueNormals{ unique<normal_t>(normals) };
    
    SoMFVec3f normalVectors{};
    normalVectors.setNum(static_cast<int>(uniqueNormals.size()));

    SbVec3f * pos = normalVectors.startEditing();

    int32_t nextNormalIndex{ static_cast<int32_t>(m_normalMap.size()) } ;
    for (const normal_t & normal : uniqueNormals) {

        if (!m_normalMap.contains(normal)) {
            m_normalMap[normal] = nextNormalIndex++;
            *pos++ = SbVec3f(normal[0], normal[1], normal[2]);
        }
    }

    normalVectors.finishEditing();

    normalNode->vector = normalVectors;

    return normalNode;
}

SoIndexedTriangleStripSet * GltfIvWriter::convertTriangles(const std::vector<index_t> & indices, const std::vector<normal_t> & normals)
{ 
    SoIndexedTriangleStripSet * triangles = new SoIndexedTriangleStripSet;

    const int indexSize{ static_cast<int>(indices.size() + indices.size() / 4U)  };

    SoMFInt32 coordIndex{};
    SoMFInt32 normalIndex{};

    coordIndex.setNum(indexSize);
    normalIndex.setNum(indexSize);

    int32_t * coordIndexPosition = coordIndex.startEditing();
    int32_t * normalIndexPosition = normalIndex.startEditing();

    constexpr size_t triangle_strip_size{ 4U };

    for (size_t i = 0; i + triangle_strip_size -1 < indices.size(); i += triangle_strip_size) {
        for (size_t j = 0; j < triangle_strip_size; ++j)
        {
            const size_t k = i + j;
            *coordIndexPosition++ = m_positionIndexMap[indices.at(k)];
            *normalIndexPosition++ = m_normalMap[normals.at(k)];
        }
        
        *coordIndexPosition++ = -1;
        *normalIndexPosition++ = -1;
    }
    coordIndex.finishEditing();
    normalIndex.finishEditing();

    triangles->materialIndex = 0;
    triangles->coordIndex = coordIndex;
    triangles->normalIndex = normalIndex;

    return triangles;
}

SoMaterial * GltfIvWriter::convertMaterial(int materialIndex)
{
    return convertMaterial(m_gltfModel.materials.at(materialIndex));
}

SbColor GltfIvWriter::diffuseColor(const tinygltf::Material & material)
{
    const std::vector<double> & baseColorFactor{ material.pbrMetallicRoughness.baseColorFactor };
    SbColor color{
        static_cast<float>(baseColorFactor[0]),
        static_cast<float>(baseColorFactor[1]),
        static_cast<float>(baseColorFactor[2])};
    
    return color;
}

SoMaterial * GltfIvWriter::convertMaterial(const tinygltf::Material & material)
{
    SoMaterial * result = new SoMaterial;

    result->ambientColor = SbColor{0.2f, 0.2f, 0.2f};
    result->diffuseColor = diffuseColor(material);
    result->specularColor = SbColor{ 0.0f, 0.0f, 0.0f };
    result->emissiveColor = SbColor{ 0.0f, 0.0f, 0.0f };
    result->shininess = 0.2f;
    result->transparency = 0.0f;

    return result;
}
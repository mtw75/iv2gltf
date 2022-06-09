#include "GltfIvWriter.h"

#include <set>
#include <stdexcept>

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

bool GltfIvWriter::write(std::string filename, bool writeBinary)
{
    if (!m_ivModel) {
        spdlog::error("open inventor model uninitilized");
        return false;
    }

    try {
        convertModel(m_ivModel);
        return GltfIv::write(filename, m_ivModel, writeBinary);
    }
    catch (std::exception exception) {
        spdlog::error(std::format("failed to convert model: {}", exception.what()));
        return false;
    }
}

void GltfIvWriter::convertModel(iv_root_t root)
{
    spdlog::trace("converting gltf model to open inventor model");

    std::for_each(
        m_gltfModel.scenes.cbegin(), 
        m_gltfModel.scenes.cend(), 
        [this, root](const tinygltf::Scene & scene)
        {
            convertScene(root, scene);
        }
    );
}

void GltfIvWriter::convertScene(iv_root_t root, const tinygltf::Scene & scene)
{
    spdlog::trace("converting scene with name '{}'", scene.name);

    SoSeparator * sceneRoot{ new SoSeparator };

    convertNodes(sceneRoot, scene.nodes);

    root->addChild(sceneRoot);
}

void GltfIvWriter::convertNodes(iv_root_t root, const std::vector<int> & nodeIndices)
{
    std::for_each(
        nodeIndices.cbegin(),
        nodeIndices.cend(),
        [this, root] (int nodeIndex)
        {
            convertNode(root, m_gltfModel.nodes.at(nodeIndex));
        }
    );
}

void GltfIvWriter::convertNode(iv_root_t root, const tinygltf::Node & node)
{
    spdlog::trace("converting node with name '{}'", node.name);

    SoSeparator * nodeRoot{ new SoSeparator };

    if (node.mesh >= 0) {
        convertMesh(nodeRoot, m_gltfModel.meshes.at(node.mesh));
    }
    convertNodes(nodeRoot, node.children);

    root->addChild(nodeRoot);
}

void GltfIvWriter::convertMesh(iv_root_t root, const tinygltf::Mesh & mesh)
{
    spdlog::trace("converting mesh with name '{}'", mesh.name);

    convertPrimitives(root, mesh.primitives);
}

void GltfIvWriter::convertPrimitives(iv_root_t root, const std::vector<tinygltf::Primitive> & primitives)
{
    std::for_each(
        primitives.cbegin(),
        primitives.cend(),
        [this, root] (const tinygltf::Primitive & primitive) 
        { 
            convertPrimitive(root, primitive);
        }
    );
}

void GltfIvWriter::convertPrimitive(iv_root_t root, const tinygltf::Primitive & primitive)
{
    spdlog::trace("converting primitive with mode {}", primitive.mode);

    switch (primitive.mode) {
    case TINYGLTF_MODE_TRIANGLES:
        convertTrianglesPrimitive(root, primitive);
        break;
    default:
        spdlog::warn("skipping unsupported primitive with mode {}", primitive.mode);
    }
}

void GltfIvWriter::convertTrianglesPrimitive(iv_root_t root, const tinygltf::Primitive & primitive)
{
    spdlog::trace("converting triangles primitive");

    if (primitive.material >= 0) {
        convertMaterial(root, m_gltfModel.materials.at(primitive.material));
    }
        
    convertTriangles(root, convertPositions(root, primitive), convertNormals(root, primitive));
}

GltfIvWriter::iv_indices_t GltfIvWriter::convertPositions(iv_root_t root, const tinygltf::Primitive & primitive)
{
    const positions_t positions{ GltfIvWriter::positions(primitive) };

    const positions_t uniquePositions{ unique<position_t>(positions) };
    
    convertPositions(root, uniquePositions);

    return positionIndices(indices(primitive), positions, positionMap(uniquePositions));
}

GltfIvWriter::iv_indices_t GltfIvWriter::convertNormals(iv_root_t root, const tinygltf::Primitive & primitive)
{
    SoNormalBinding * normalBinding = new SoNormalBinding;
    normalBinding->value = SoNormalBinding::Binding::PER_VERTEX_INDEXED;
    root->addChild(normalBinding);

    const normals_t normals{ GltfIvWriter::normals(primitive) };
    const normals_t uniqueNormals{ unique(normals) };

    convertNormals(root, uniqueNormals);

    return normalIndices(normals, normalMap(uniqueNormals));
}

GltfIvWriter::indices_t GltfIvWriter::indices(const tinygltf::Primitive & primitive)
{
    spdlog::trace("retrieve indices from primitive");

    const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(primitive.indices);

    ensureAccessorType(accessor, TINYGLTF_TYPE_SCALAR);

    switch (accessor.componentType){
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        return accessorContents<int8_t>(accessor);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return accessorContents<uint8_t>(accessor);
    case TINYGLTF_COMPONENT_TYPE_SHORT:
        return accessorContents<int16_t>(accessor);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        return accessorContents<uint16_t>(accessor);
        // TINYGLTF_COMPONENT_TYPE_INT not supported as per specification https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#accessor-data-types
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        return accessorContents<uint32_t>(accessor);
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
        return accessorContents<float>(accessor);
    default:
        throw std::invalid_argument(std::format("unsupported component type {}", accessor.componentType));
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

void GltfIvWriter::convertPositions(iv_root_t root, const positions_t & positions)
{
    SoCoordinate3 * coords = new SoCoordinate3;

    coords->point.setNum(static_cast<int>(positions.size()));
    SbVec3f * ivPointsPtr = coords->point.startEditing();
    
    static_assert(sizeof(SbVec3f) == sizeof(position_t));

    std::memcpy(ivPointsPtr, &positions[0], sizeof(SbVec3f) * positions.size());

    coords->point.finishEditing();

    root->addChild(coords);
}

void GltfIvWriter::convertNormals(iv_root_t root, const normals_t & normals)
{
    SoNormal * normalNode = new SoNormal;
    
    SoMFVec3f normalVectors{};
    normalVectors.setNum(static_cast<int>(normals.size()));

    SbVec3f * normalVectorPos = normalVectors.startEditing();

    std::memcpy(normalVectorPos, &normals[0], sizeof(SbVec3f) * normals.size());

    normalVectors.finishEditing();

    normalNode->vector = normalVectors;

    root->addChild(normalNode);
}

std::map<GltfIvWriter::normal_t, GltfIvWriter::iv_index_t> GltfIvWriter::normalMap(const normals_t & normals)
{
    normal_map_t result;

    for (iv_index_t index = 0; index < normals.size(); ++index) {
        result[normals[index]] = index;
    }

    return result;
}

GltfIvWriter::iv_indices_t GltfIvWriter::normalIndices(const normals_t & normals, const normal_map_t & normalMap)
{
    iv_indices_t normalIndices;
    normalIndices.reserve(normals.size());

    std::transform(
        normals.cbegin(),
        normals.cend(),
        std::back_inserter(normalIndices),
        [&normalMap] (const normal_t & normal)
        {
            return normalMap.at(normal);
        }
    );

    return normalIndices;
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

void GltfIvWriter::convertMaterial(iv_root_t root, const tinygltf::Material & material)
{
    SoMaterial * materialNode = new SoMaterial;

    materialNode->diffuseColor = diffuseColor(material);

    root->addChild(materialNode);

    SoMaterialBinding * materialBinding = new SoMaterialBinding;

    materialBinding->value = SoMaterialBinding::Binding::OVERALL;

    root->addChild(materialBinding);
}
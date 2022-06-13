#pragma once

#include "GltfIv.h"

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>

#include <gsl/gsl>
#include <spdlog/stopwatch.h>

#include <map>
#include <variant>

class GltfIvWriter {
public:
    GltfIvWriter(tinygltf::Model && gltfModel) 
        : m_gltfModel{ std::move(gltfModel) }
    {
    }

    bool write(std::string filename, bool writeBinary)
    {
        spdlog::trace("convert gltf model to open inventor and write it to file {} as {}", filename, writeBinary ? "binary" : "ascii");
        try {
            iv_root_t root { new SoSeparator };
            root->ref();
            convertModel(root);
            const bool success{ GltfIv::write(filename, root, writeBinary) };
            root->unref();
            return success;
        }
        catch (std::exception exception) {
            spdlog::error(std::format("failed to convert model: {}", exception.what()));
            return false;
        }
    }

private:

    using iv_index_t = int32_t;
    using iv_indices_t = std::vector<iv_index_t>;

    using position_t = std::array<float, 3>;
    using positions_t = std::vector<position_t>;
    using position_map_t = std::map<position_t, iv_index_t>;

    using normal_t = std::array<float, 3>;
    using normals_t = std::vector<normal_t>;
    using normal_map_t = std::map<normal_t, iv_index_t>;

    using iv_root_t = gsl::not_null<SoSeparator *>;
    
    void convertModel(iv_root_t root) const
    {
        spdlog::stopwatch stopwatch;
        spdlog::trace("convert gltf model to open inventor model");

        std::for_each(
            m_gltfModel.scenes.cbegin(),
            m_gltfModel.scenes.cend(),
            [this, root] (const tinygltf::Scene & scene)
            {
                convertScene(root, scene);
            }
        );
        spdlog::debug("finished converting gltf model to open inventor model ({:.3} seconds)", stopwatch);
    }

    void convertScene(iv_root_t root, const tinygltf::Scene & scene) const
    {
        spdlog::trace("converting gltf scene with name '{}'", scene.name);

        SoSeparator * sceneRoot{ new SoSeparator };

        convertNodes(sceneRoot, scene.nodes);

        root->addChild(sceneRoot);
    }

    void convertNodes(iv_root_t root, const std::vector<int> & nodeIndices) const
    {
        spdlog::trace("converting {} gltf nodes", nodeIndices.size());

        std::for_each(
            nodeIndices.cbegin(),
            nodeIndices.cend(),
            [this, root] (int nodeIndex)
            {
                convertNode(root, m_gltfModel.nodes.at(static_cast<size_t>(nodeIndex)));
            }
        );
    }

    void convertNode(iv_root_t root, const tinygltf::Node & node) const
    {
        spdlog::trace("converting gltf node with name '{}'", node.name);

        SoSeparator * nodeRoot{ new SoSeparator };

        if (node.mesh >= 0) {
            convertMesh(nodeRoot, m_gltfModel.meshes.at(static_cast<size_t>(node.mesh)));
        }
        convertNodes(nodeRoot, node.children);

        root->addChild(nodeRoot);
    }

    void convertMesh(iv_root_t root, const tinygltf::Mesh & mesh) const
    {
        spdlog::trace("converting gltf mesh with name '{}'", mesh.name);

        const std::vector<tinygltf::Primitive> & primitives{ mesh.primitives };

        std::for_each(
            primitives.cbegin(),
            primitives.cend(),
            [this, root] (const tinygltf::Primitive & primitive)
            {
                convertPrimitive(root, primitive);
            }
        );
    }

    void convertPrimitive(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf primitive with mode {}", primitive.mode);

        switch (primitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:
            convertTrianglesPrimitive(root, primitive);
            break;
        default:
            spdlog::warn("skipping unsupported primitive with mode {}", primitive.mode);
        }
    }

    void convertTrianglesPrimitive(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf triangles primitive");

        if (primitive.material >= 0) {
            convertMaterial(root, m_gltfModel.materials.at(static_cast<size_t>(primitive.material)));
        }

        convertTriangles(root, convertPositions(root, primitive), convertNormals(root, primitive));
    }

    static void convertMaterial(iv_root_t root, const tinygltf::Material & material)
    {
        spdlog::trace("converting gltf material with name '{}'", material.name);

        SoMaterial * materialNode = new SoMaterial;
        materialNode->diffuseColor = diffuseColor(material);
        root->addChild(materialNode);

        SoMaterialBinding * materialBinding = new SoMaterialBinding;
        materialBinding->value = SoMaterialBinding::Binding::OVERALL;
        root->addChild(materialBinding);
    }

    static SbColor diffuseColor(const tinygltf::Material & material)
    {
        spdlog::trace("extracting diffuse color from gltf material");

        const std::vector<double> & baseColorFactor{ material.pbrMetallicRoughness.baseColorFactor };

        return SbColor{
            static_cast<float>(baseColorFactor[0]),
            static_cast<float>(baseColorFactor[1]),
            static_cast<float>(baseColorFactor[2]) 
        };
    }

    static void convertTriangles(iv_root_t root, const iv_indices_t & positionIndices, const iv_indices_t & normalIndices)
    {
        if (positionIndices.size() != normalIndices.size()) {
            throw std::invalid_argument(std::format("mismatching number of indices ({}) and normals ({})", positionIndices.size(), normalIndices.size()));
        }

        constexpr size_t triangle_strip_size{ 3U };

        if (positionIndices.size() % triangle_strip_size != 0) {
            throw std::invalid_argument(std::format("number of indices ({}) is not divisible by the triangel stip size", positionIndices.size(), triangle_strip_size));
        }

        spdlog::trace("converting {} triangle strips from gltf primitive", positionIndices.size() / triangle_strip_size);

        SoIndexedTriangleStripSet * triangles = new SoIndexedTriangleStripSet;

        const int indexSize{ static_cast<int>(positionIndices.size() + positionIndices.size() / triangle_strip_size) };

        SoMFInt32 coordIndex{};
        SoMFInt32 normalIndex{};

        coordIndex.setNum(indexSize);
        normalIndex.setNum(indexSize);

        iv_index_t * coordIndexPosition = coordIndex.startEditing();
        iv_index_t * normalIndexPosition = normalIndex.startEditing();

        for (size_t i = 0; i + triangle_strip_size - 1 < positionIndices.size(); i += triangle_strip_size) {

            std::memcpy(coordIndexPosition, &positionIndices[i], sizeof(iv_index_t) * triangle_strip_size);
            std::memcpy(normalIndexPosition, &normalIndices[i], sizeof(iv_index_t) * triangle_strip_size);

            std::advance(coordIndexPosition, triangle_strip_size);
            std::advance(normalIndexPosition, triangle_strip_size);

            *coordIndexPosition++ = -1;
            *normalIndexPosition++ = -1;
        }
        coordIndex.finishEditing();
        normalIndex.finishEditing();

        triangles->materialIndex = 0;
        triangles->coordIndex = coordIndex;
        triangles->normalIndex = normalIndex;

        root->addChild(triangles);
    }

    iv_indices_t convertPositions(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf positions from gltf primitive");

        const positions_t positions{ GltfIvWriter::positions(primitive) };
        const positions_t uniquePositions{ unique<position_t>(positions) };

        convertPositions(root, uniquePositions);

        return positionIndices( positions, positionMap(uniquePositions));
    }

    static void convertPositions(iv_root_t root, const positions_t & positions)
    {
        spdlog::trace("converting {} gltf positions", positions.size());

        static_assert(sizeof(SbVec3f) == sizeof(position_t));

        SoCoordinate3 * coords = new SoCoordinate3;
        coords->point.setNum(static_cast<int>(positions.size()));
        SbVec3f * ivPointsPtr = coords->point.startEditing();
        std::memcpy(ivPointsPtr, &positions[0], sizeof(SbVec3f) * positions.size());
        coords->point.finishEditing();

        root->addChild(coords);
    }

    iv_indices_t convertNormals(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf normals from gltf primitive");

        SoNormalBinding * normalBinding = new SoNormalBinding;
        normalBinding->value = SoNormalBinding::Binding::PER_VERTEX_INDEXED;
        root->addChild(normalBinding);

        const normals_t normals{ GltfIvWriter::normals(primitive) };

        if (normals.empty()) {
            return {};
        }

        const normals_t uniqueNormals{ unique(normals) };

        convertNormals(root, uniqueNormals);

        return normalIndices(normals, normalMap(uniqueNormals));
    }

    static void convertNormals(iv_root_t root, const normals_t & normals)
    {
        spdlog::trace("converting {} gltf normals", normals.size());

        SoMFVec3f normalVectors{};
        normalVectors.setNum(static_cast<int>(normals.size()));
        SbVec3f * normalVectorPos = normalVectors.startEditing();
        std::memcpy(normalVectorPos, &normals[0], sizeof(SbVec3f) * normals.size());
        normalVectors.finishEditing();

        SoNormal * normalNode = new SoNormal;
        normalNode->vector = normalVectors;
        root->addChild(normalNode);
    }

    static normal_map_t normalMap(const normals_t & normals)
    {
        spdlog::trace("create index map for {} normals", normals.size());

        normal_map_t result;

        for (size_t index = 0; index < normals.size(); ++index) {
            result[normals[index]] = static_cast<iv_index_t>(index);
        }

        return result;
    }

    static iv_indices_t normalIndices(const normals_t & normals, const normal_map_t & normalMap)
    {
        spdlog::trace("create index for {} normals", normals.size());

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

    static iv_indices_t positionIndices(const positions_t & positions, const position_map_t & positionMap)
    {
        spdlog::trace("create index for {} positions", positions.size());

        iv_indices_t positionIndices;
        positionIndices.reserve(positions.size());

        std::transform(
            positions.cbegin(),
            positions.cend(),
            std::back_inserter(positionIndices),
            [&positions, &positionMap] (const position_t & position)
            {
                return positionMap.at(position);               
            }
        );

        return positionIndices;
    }

    static position_map_t positionMap(const positions_t & positions)
    {
        spdlog::trace("create index map for {} positions", positions.size());

        position_map_t positionMap;

        for (size_t i = 0; i < positions.size(); ++i) {
            positionMap[positions[i]] = static_cast<iv_index_t>(i);
        }

        return positionMap;
    }

    positions_t positions(const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("retrieve positions from primitive");
        const int accessorIndex{ primitive.attributes.at("POSITION") };
        if (accessorIndex >= 0) {
            const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(static_cast<size_t>(accessorIndex));
            ensureAccessorType(accessor, TINYGLTF_TYPE_VEC3);
            return accessorContents<position_t>(accessor);
        } else {
            spdlog::warn("positions accessor at index {} not found", accessorIndex);
            return {};
        }
        
    }

    normals_t normals(const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("retrieve normals from primitive");
        const int accessorIndex{ primitive.attributes.at("NORMAL") };
        if (accessorIndex >= 0) {
            const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(static_cast<size_t>(accessorIndex));
            ensureAccessorType(accessor, TINYGLTF_TYPE_VEC3);
            return accessorContents<normal_t>(accessor);
        } else {
            spdlog::warn("normals accessor at index {} not found", accessorIndex);
            return {};
        }
    }

    static void ensureAccessorType(const tinygltf::Accessor & accessor, int accessorType)
    {
        spdlog::trace("ensure gltf accessor type is {}", accessorType);

        if (accessor.type != accessorType) {
            throw std::domain_error(std::format("expected accessor type {} instead of {}", accessorType, accessor.type));
        }
    }

    template<class T>
    std::vector<T> accessorContents(const tinygltf::Accessor & accessor) const
    {
        spdlog::trace("read contents of gltf accessor with name '{}'", accessor.name);

        const int bufferViewIndex{ accessor.bufferView };
        if (bufferViewIndex < 0) {
            spdlog::warn("accessor buffer view not found at index {}", bufferViewIndex);
            return {};
        }
        const tinygltf::BufferView & bufferView = m_gltfModel.bufferViews.at(static_cast<size_t>(bufferViewIndex));

        const int bufferIndex{ bufferView.buffer };
        if (bufferIndex < 0) {
            spdlog::warn("accessor buffer not found at index {}", bufferIndex);
            return {};
        }

        const tinygltf::Buffer & buffer = m_gltfModel.buffers.at(static_cast<size_t>(bufferIndex));

        const int byteStride{ accessor.ByteStride(bufferView) };
        if (byteStride != sizeof(T)) {
            throw std::invalid_argument(
                std::format(
                    "mismatching size of the buffer's byte stride ({}) and the size of the target type ({})",
                    byteStride,
                    sizeof(T)
                )
            );
        }

        std::vector<T> contents;
        contents.resize(accessor.count);

        std::memcpy(&contents[0], &buffer.data[bufferView.byteOffset], bufferView.byteLength);

        return contents;
    }

    template<class T>
    static std::vector<T> unique(const std::vector<T> & items) {
        spdlog::trace("remove duplicates among {} items", items.size());
        std::vector<T> unqiueItems{ items };
        std::sort(unqiueItems.begin(), unqiueItems.end());
        unqiueItems.erase(std::unique(unqiueItems.begin(), unqiueItems.end()), unqiueItems.end());
        return unqiueItems;

    }

    const tinygltf::Model m_gltfModel;
};

#pragma once

#include "GltfIv.h"

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>

#include <map>

class GltfIvWriter {
public:
    GltfIvWriter(tinygltf::Model && gltfModel);
    ~GltfIvWriter();
    bool write(std::string filename, bool writeBinary);

private:

    using position_t = std::array<float, 3>;
    using normal_t = std::array<float, 3>;
    using texture_coordinate_t = std::array<float, 2>;
    using index_t = uint32_t;

    bool convertModel();
    bool convertScene(const tinygltf::Scene & scene);
    bool convertNodes(const std::vector<int> nodeIndices);
    bool convertNode(int nodeIndex);
    bool convertNode(const tinygltf::Node & node);
    bool convertMesh(int meshIndex);
    bool convertMesh(const tinygltf::Mesh & mesh);
    bool convertPrimitives(const std::vector<tinygltf::Primitive> & primitives);
    bool convertPrimitive(const tinygltf::Primitive & primitive);
    bool convertTrianglesPrimitive(const tinygltf::Primitive & primitive);
    void ensureAccessorType(const tinygltf::Accessor & accessor, int accessorType) const;
    std::vector<position_t> positions(const tinygltf::Primitive & primitive);
    std::vector<normal_t> normals(const tinygltf::Primitive & primitive);
    std::vector<texture_coordinate_t> textureCoordinates(const tinygltf::Primitive & primitive);

    int indexTypeSize(const tinygltf::Primitive & primitive) const;

    template<typename original_index_t>
    std::vector<index_t> indices(const tinygltf::Accessor & accessor)
    {
        if constexpr (std::same_as<original_index_t, index_t>) {
            return accessorContents<index_t>(accessor);
        }
        else {
            std::vector<original_index_t> originalIndices{ accessorContents<original_index_t>(accessor) };
            std::vector< index_t> result{};
            result.reserve(originalIndices.size());

            for (original_index_t originalIndex : originalIndices) {
                result.push_back(static_cast<index_t>(originalIndex));
            }
            return result;
        }
    }

    std::vector<index_t> indices(const tinygltf::Primitive & primitive);

    SoMaterial * convertMaterial(int materialIndex);
    SbColor diffuseColor(const tinygltf::Material & material);
    SoMaterial * convertMaterial(const tinygltf::Material & material);

    template<class T>
    std::vector<T> accessorContents(const tinygltf::Accessor & accessor)
    {
        const tinygltf::BufferView & bufferView = m_gltfModel.bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer & buffer = m_gltfModel.buffers.at(bufferView.buffer);

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
    std::vector<T> unique(const std::vector<T> & items) {
        std::vector<T> unqiueItems{ items };
        std::sort(unqiueItems.begin(), unqiueItems.end());
        unqiueItems.erase(std::unique(unqiueItems.begin(), unqiueItems.end()), unqiueItems.end());
        return unqiueItems;

    }

    SoCoordinate3 * convertPositions(const std::vector<position_t> & positions);
    void updatePositionIndexMap(const std::vector<position_t> & uniquePositions, const std::vector<position_t> & positions, const std::vector<index_t> & indices);

    SoNormal * convertNormals(const std::vector<normal_t> & normals);
    SoIndexedTriangleStripSet * convertTriangles(const std::vector<index_t> & indices, const std::vector<normal_t> & normals);


    const tinygltf::Model m_gltfModel;
    SoSeparator * m_ivModel{ nullptr };
    std::unordered_map<index_t, int32_t> m_positionIndexMap;
    std::map<normal_t, int32_t> m_normalMap;
};
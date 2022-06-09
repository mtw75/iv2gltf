#pragma once

#include "GltfIv.h"

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoMaterial.h>

#include <gsl/gsl>

#include <map>
#include <variant>

class GltfIvWriter {
public:
    GltfIvWriter(tinygltf::Model && gltfModel);
    ~GltfIvWriter();
    bool write(std::string filename, bool writeBinary);

private:

    using position_t = std::array<float, 3>;
    using normal_t = std::array<float, 3>;
    using texture_coordinate_t = std::array<float, 2>;
    using indices_t = std::variant<std::vector<uint8_t>, std::vector<int8_t>, std::vector<uint16_t>, std::vector<int16_t>, std::vector<uint32_t>, std::vector<float>>;
    using index_map_t = std::variant<std::unordered_map<uint8_t, int32_t>, std::unordered_map<int8_t, int32_t>, std::unordered_map<uint16_t, int32_t>, std::unordered_map<int16_t, int32_t>, std::unordered_map<uint32_t, int32_t>, std::unordered_map<float, int32_t>>;
    using normal_map_t = std::map<normal_t, int32_t>;
    using iv_root_t = gsl::not_null<SoSeparator *>;

    void convertModel(iv_root_t root);
    void convertScene(iv_root_t root, const tinygltf::Scene & scene);
    void convertNodes(iv_root_t root, const std::vector<int> & nodeIndices);
    void convertNode(iv_root_t root, const tinygltf::Node & node);
    void convertMesh(iv_root_t root, const tinygltf::Mesh & mesh);
    void convertPrimitives(iv_root_t root, const std::vector<tinygltf::Primitive> & primitives);
    void convertPrimitive(iv_root_t root, const tinygltf::Primitive & primitive);
    void convertTrianglesPrimitive(iv_root_t root, const tinygltf::Primitive & primitive);

    void ensureAccessorType(const tinygltf::Accessor & accessor, int accessorType) const;
    std::vector<position_t> positions(const tinygltf::Primitive & primitive);
    std::vector<normal_t> normals(const tinygltf::Primitive & primitive);
    std::vector<texture_coordinate_t> textureCoordinates(const tinygltf::Primitive & primitive);

    indices_t indices(const tinygltf::Primitive & primitive);

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

    template<typename index_t>
    std::unordered_map<index_t, int32_t> positionIndexMap(const std::vector<position_t> & uniquePositions, const std::vector<position_t> & positions, const std::vector<index_t> & indices)
    {
        std::map<position_t, int32_t> positionMap{};
        std::unordered_map<index_t, int32_t> positionIndexMap{};

        for (int32_t i = 0; i < uniquePositions.size(); ++i) {
            positionMap[uniquePositions[i]] = i;
        }

        for (const index_t & index : indices) {
            positionIndexMap[index] = positionMap.at(positions.at(static_cast<size_t>(index)));
        }

        return positionIndexMap;
    }

    index_map_t positionIndexMap(const std::vector<position_t> & uniquePositions, const std::vector<position_t> & positions, const indices_t & indices)
    {
        if (std::holds_alternative<std::vector<uint8_t>>(indices)) {
            return positionIndexMap<uint8_t>(uniquePositions, positions, std::get<std::vector<uint8_t>>(indices));
        } else if (std::holds_alternative<std::vector<int8_t>>(indices)) {
            return positionIndexMap<int8_t>(uniquePositions, positions, std::get<std::vector<int8_t>>(indices));
        } else if (std::holds_alternative<std::vector<uint16_t>>(indices)) {
            return positionIndexMap<uint16_t>(uniquePositions, positions, std::get<std::vector<uint16_t>>(indices));
        } else if (std::holds_alternative<std::vector<int16_t>>(indices)) {
            return positionIndexMap<int16_t>(uniquePositions, positions, std::get<std::vector<int16_t>>(indices));
        } else if (std::holds_alternative<std::vector<uint32_t>>(indices)) {
            return positionIndexMap<uint32_t>(uniquePositions, positions, std::get<std::vector<uint32_t>>(indices));
        } else if (std::holds_alternative<std::vector<float>>(indices)) {
            return positionIndexMap<float>(uniquePositions, positions, std::get<std::vector<float>>(indices));
        }

        throw std::invalid_argument("unsupported index type");
    }

    normal_map_t normalMap(const std::vector<normal_t> & normals);
    SoNormal * convertNormals(const std::vector<normal_t> & normals);

    template<typename index_t>
    SoIndexedTriangleStripSet * convertTriangles(const std::vector<index_t> & indices, const std::vector<normal_t> & normals, const normal_map_t & normalMap, const std::unordered_map<index_t, int32_t> & positionIndexMap)
    {
        if (indices.size() != normals.size()) {
            throw std::invalid_argument(std::format("mismatching number of indices ({}) and normals ({})", indices.size(), normals.size()));
        }

        constexpr size_t triangle_strip_size{ 3U };

        if (indices.size() % triangle_strip_size != 0) {
            throw std::invalid_argument(std::format("number of indices ({}) is not divisible by the triangel stip size", indices.size(), triangle_strip_size));
        }

        SoIndexedTriangleStripSet * triangles = new SoIndexedTriangleStripSet;

        const int indexSize{ static_cast<int>(indices.size() + indices.size() / triangle_strip_size) };

        SoMFInt32 coordIndex{};
        SoMFInt32 normalIndex{};

        coordIndex.setNum(indexSize);
        normalIndex.setNum(indexSize);

        int32_t * coordIndexPosition = coordIndex.startEditing();
        int32_t * normalIndexPosition = normalIndex.startEditing();

        for (size_t i = 0; i + triangle_strip_size - 1 < indices.size(); i += triangle_strip_size) {
            for (size_t j = 0; j < triangle_strip_size; ++j)
            {
                const size_t k = i + j;
                *coordIndexPosition++ = positionIndexMap.at(indices.at(k));
                *normalIndexPosition++ = normalMap.at(normals.at(k));
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

    SoIndexedTriangleStripSet * convertTriangles(const indices_t & indices, const std::vector<normal_t> & normals, const normal_map_t & normalMap, const index_map_t & positionIndexMap)
    {
        if (std::holds_alternative<std::vector<uint8_t>>(indices) && std::holds_alternative<std::unordered_map<uint8_t, int32_t>>(positionIndexMap)) {
            return convertTriangles<uint8_t>(std::get<std::vector<uint8_t>>(indices), normals, normalMap, std::get<std::unordered_map<uint8_t, int32_t>>(positionIndexMap));
        } else if (std::holds_alternative<std::vector<int8_t>>(indices) && std::holds_alternative<std::unordered_map<int8_t, int32_t>>(positionIndexMap)) {
            return convertTriangles<int8_t>(std::get<std::vector<int8_t>>(indices), normals, normalMap, std::get<std::unordered_map<int8_t, int32_t>>(positionIndexMap));
        } else if (std::holds_alternative<std::vector<uint16_t>>(indices) && std::holds_alternative<std::unordered_map<uint16_t, int32_t>>(positionIndexMap)) {
            return convertTriangles<uint16_t>(std::get<std::vector<uint16_t>>(indices), normals, normalMap, std::get<std::unordered_map<uint16_t, int32_t>>(positionIndexMap));
        } else if (std::holds_alternative<std::vector<int16_t>>(indices) && std::holds_alternative<std::unordered_map<int16_t, int32_t>>(positionIndexMap)) {
            return convertTriangles<int16_t>(std::get<std::vector<int16_t>>(indices), normals, normalMap, std::get<std::unordered_map<int16_t, int32_t>>(positionIndexMap));
        } else if (std::holds_alternative<std::vector<uint32_t>>(indices) && std::holds_alternative<std::unordered_map<uint32_t, int32_t>>(positionIndexMap)) {
            return convertTriangles<uint32_t>(std::get<std::vector<uint32_t>>(indices), normals, normalMap, std::get<std::unordered_map<uint32_t, int32_t>>(positionIndexMap));
        } else if (std::holds_alternative<std::vector<float>>(indices) && std::holds_alternative<std::unordered_map<float, int32_t>>(positionIndexMap)) {
            return convertTriangles<float>(std::get<std::vector<float>>(indices), normals, normalMap, std::get<std::unordered_map<float, int32_t>>(positionIndexMap));
        }

        throw std::invalid_argument("unsupported index type");
    }

    const tinygltf::Model m_gltfModel;
    SoSeparator * m_ivModel{ nullptr };
};
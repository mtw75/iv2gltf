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

    using iv_index_t = int32_t;
    using iv_indices_t = std::vector<iv_index_t>;

    using position_t = std::array<float, 3>;
    using positions_t = std::vector<position_t>;
    using position_map_t = std::map<position_t, iv_index_t>;

    using normal_t = std::array<float, 3>;
    using normals_t = std::vector<normal_t>;
    using normal_map_t = std::map<normal_t, iv_index_t>;

    using texture_coordinate_t = std::array<float, 2>;
    using indices_t = std::variant<std::vector<uint8_t>, std::vector<int8_t>, std::vector<uint16_t>, std::vector<int16_t>, std::vector<uint32_t>, std::vector<float>>;

    using iv_root_t = gsl::not_null<SoSeparator *>;
    

    void convertModel(iv_root_t root);
    void convertScene(iv_root_t root, const tinygltf::Scene & scene);
    void convertNodes(iv_root_t root, const std::vector<int> & nodeIndices);
    void convertNode(iv_root_t root, const tinygltf::Node & node);
    void convertMesh(iv_root_t root, const tinygltf::Mesh & mesh);
    void convertPrimitives(iv_root_t root, const std::vector<tinygltf::Primitive> & primitives);
    void convertPrimitive(iv_root_t root, const tinygltf::Primitive & primitive);
    void convertTrianglesPrimitive(iv_root_t root, const tinygltf::Primitive & primitive);

    SbColor diffuseColor(const tinygltf::Material & material);
    void convertMaterial(iv_root_t root, const tinygltf::Material & material);

    iv_indices_t convertPositions(iv_root_t root, const tinygltf::Primitive & primitive);
    void convertPositions(iv_root_t root, const positions_t & positions);

    iv_indices_t convertNormals(iv_root_t root, const tinygltf::Primitive & primitive);
    void convertNormals(iv_root_t root, const normals_t & normals);

    void ensureAccessorType(const tinygltf::Accessor & accessor, int accessorType) const;
    positions_t positions(const tinygltf::Primitive & primitive);
    normals_t normals(const tinygltf::Primitive & primitive);
    std::vector<texture_coordinate_t> textureCoordinates(const tinygltf::Primitive & primitive);

    indices_t indices(const tinygltf::Primitive & primitive);

    
    

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

    

    template<typename index_t>
    std::unordered_map<index_t, iv_index_t> positionIndexMap(const positions_t & uniquePositions, const positions_t & positions, const std::vector<index_t> & indices)
    {
        std::map<position_t, iv_index_t> positionMap{};
        std::unordered_map<index_t, iv_index_t> positionIndexMap{};

        for (iv_index_t i = 0; i < uniquePositions.size(); ++i) {
            positionMap[uniquePositions[i]] = i;
        }

        for (const index_t & index : indices) {
            positionIndexMap[index] = positionMap.at(positions.at(static_cast<size_t>(index)));
        }

        return positionIndexMap;
    }

    position_map_t positionMap(const positions_t & positions) 
    {
        position_map_t positionMap;

        for (iv_index_t i = 0; i < positions.size(); ++i) {
            positionMap[positions[i]] = i;
        }

        return positionMap;
    }

    template<typename index_t>
    iv_indices_t positionIndices(const std::vector<index_t> & indices, const positions_t & positions, const position_map_t & positionMap)
    {
        iv_indices_t positionIndices;
        positionIndices.reserve(indices.size());

        std::transform(
            indices.cbegin(),
            indices.cend(),
            std::back_inserter(positionIndices),
            [&positions, &positionMap] (const index_t & index)
            {
                if constexpr (std::is_same<index_t, float>::value) {
                    return positionMap.at(positions.at(static_cast<size_t>(index)));
                }
                else {
                    return positionMap.at(positions.at(index));
                }
            }
        );

        return positionIndices;
    }

    iv_indices_t positionIndices(const indices_t & indices, const positions_t & positions, const position_map_t & positionMap)
    {
        if (std::holds_alternative<std::vector<uint8_t>>(indices)) {
            return positionIndices<uint8_t>(std::get<std::vector<uint8_t>>(indices), positions, positionMap);
        } else if(std::holds_alternative<std::vector<int8_t>>(indices)) {
            return positionIndices<int8_t>(std::get<std::vector<int8_t>>(indices), positions, positionMap);
        } else if (std::holds_alternative<std::vector<uint16_t>>(indices)) {
            return positionIndices<uint16_t>(std::get<std::vector<uint16_t>>(indices), positions, positionMap);
        } else if (std::holds_alternative<std::vector<int16_t>>(indices)) {
            return positionIndices<int16_t>(std::get<std::vector<int16_t>>(indices), positions, positionMap);
        } else if (std::holds_alternative<std::vector<uint32_t>>(indices)) {
            return positionIndices<uint32_t>(std::get<std::vector<uint32_t>>(indices), positions, positionMap);
        } else if (std::holds_alternative<std::vector<float>>(indices)) {
            return positionIndices<float>(std::get<std::vector<float>>(indices), positions, positionMap);
        }

        throw std::invalid_argument("unsupported index type");
    }

    normal_map_t normalMap(const normals_t & normals);
    
    iv_indices_t normalIndices(const normals_t & normals, const normal_map_t & normalMap);

    void convertTriangles(iv_root_t root, const iv_indices_t & positionIndices, const iv_indices_t & normalIndices)
    {
        if (positionIndices.size() != normalIndices.size()) {
            throw std::invalid_argument(std::format("mismatching number of indices ({}) and normals ({})", positionIndices.size(), normalIndices.size()));
        }

        constexpr size_t triangle_strip_size{ 3U };

        if (positionIndices.size() % triangle_strip_size != 0) {
            throw std::invalid_argument(std::format("number of indices ({}) is not divisible by the triangel stip size", positionIndices.size(), triangle_strip_size));
        }

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

    const tinygltf::Model m_gltfModel;
    SoSeparator * m_ivModel{ nullptr };
};
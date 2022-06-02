#pragma once

#include "GltfIv.h"

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedTriangleStripSet.h>

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
    std::vector<int> indices(const tinygltf::Primitive & primitive);

    template<class T> 
    std::vector<T> accessorContents(const tinygltf::Accessor & accessor)
    {
        std::vector<T> contents;
        contents.resize(accessor.count);

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

        std::memcpy(&contents[0], &buffer.data[bufferView.byteOffset], bufferView.byteLength);

        return contents;
    }

    size_t countUnqiueIndexedPositions(const std::vector<position_t> & positions, const std::vector<int> & indices);
    SoCoordinate3 * convertPositions(const std::vector<position_t> & positions, const std::vector<int> & indices);
    SoIndexedTriangleStripSet * convertTriangles(const std::vector<int> & indices);
    
    const tinygltf::Model m_gltfModel;
    SoSeparator * m_ivModel{ nullptr };
    std::unordered_map<int, size_t> m_positionIndexMap;
};
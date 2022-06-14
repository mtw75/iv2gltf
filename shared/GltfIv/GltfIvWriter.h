#pragma once

#include "GltfIv.h"


#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoTransform.h>

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
        , m_nodes{}
        , m_meshes{}
        , m_materials{}
    {
    }

    bool write(std::string filename, bool writeBinary)
    {
        spdlog::trace("convert gltf model to open inventor and write it to file {} as {}", filename, writeBinary ? "binary" : "ascii");
        try {
            m_nodes.clear();
            m_meshes.clear();
            m_materials.clear();

            iv_root_t root{ new SoSeparator };
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

    using gltf_index_t = size_t;
    using gltf_indices_t = std::vector<gltf_index_t>;

    using position_t = std::array<float, 3>;
    using positions_t = std::vector<position_t>;
    using position_map_t = std::map<position_t, iv_index_t>;

    using normal_t = std::array<float, 3>;
    using normals_t = std::vector<normal_t>;
    using normal_map_t = std::map<normal_t, iv_index_t>;

    using iv_root_t = gsl::not_null<SoSeparator *>;
    using iv_material_t = gsl::not_null<SoMaterial *>;

    void convertModel(iv_root_t root)
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

    void convertScene(iv_root_t root, const tinygltf::Scene & scene)
    {
        spdlog::trace("converting gltf scene with name '{}'", scene.name);

        SoSeparator * sceneRoot{ new SoSeparator };

        convertNodes(sceneRoot, scene.nodes);

        root->addChild(sceneRoot);
    }

    void convertNodes(iv_root_t root, const std::vector<int> & nodeIndices)
    {
        spdlog::trace("converting {} gltf nodes", nodeIndices.size());

        std::for_each(
            nodeIndices.cbegin(),
            nodeIndices.cend(),
            [this, root] (int nodeIndex)
            {
                convertNode(root, static_cast<size_t>(nodeIndex));
            }
        );
    }

    void convertNode(iv_root_t root, size_t nodeIndex)
    {
        spdlog::trace("converting gltf node with index {}", nodeIndex);

        if (m_nodes.contains(nodeIndex)) {
            spdlog::debug("re-using already converted gltf node with index {}", nodeIndex);
            root->addChild(m_nodes.at(nodeIndex));
        }
        else {
            const tinygltf::Node node{ m_gltfModel.nodes.at(nodeIndex) };
            spdlog::debug("converting gltf node with name '{}'", node.name);

            if (hasZeroScale(node)) {
                spdlog::debug("skipping gltf node with zero scale");
                return;
            }

            SoSeparator * nodeRoot{ new SoSeparator };

            convertTransform(nodeRoot, node);
            convertScale(nodeRoot, node);
            convertRotation(nodeRoot, node);
            convertTranslation(nodeRoot, node);

            if (hasMesh(node)) {
                convertMesh(nodeRoot, static_cast<size_t>(node.mesh));
            }

            if (!node.children.empty()) {
                convertNodes(nodeRoot, node.children);
            }

            root->addChild(nodeRoot);
            m_nodes.insert(std::make_pair(nodeIndex, nodeRoot));
        }
    }

    static inline bool hasMesh(const tinygltf::Node & node) {
        return node.mesh >= 0;
    }

    static inline bool hasZeroScale(const tinygltf::Node & node)
    {
        return hasScale(node) && node.scale[0] == 0.0 && node.scale[1] == 0.0 && node.scale[2] == 0.0;
    }

    static void convertTransform(iv_root_t root, const tinygltf::Node & node)
    {
        if (hasTransform(node)) {
            spdlog::trace("converting transform [{:.4}, {:.4}, {:.4}, {:.4}; {:.4}, {:.4}, {:.4}, {:.4}; {:.4}, {:.4}, {:.4}, {:.4}; {:.4}, {:.4}, {:.4}, {:.4};] for gltf node",
                node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
                node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
                node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]
            );

            SoTransform * transformNode{ new SoTransform };

            transformNode->setMatrix(
                SbMatrix{
                    static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]), static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
                    static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]), static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
                    static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]), static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
                    static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]), static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15])
                }
            );

            root->addChild(transformNode);
        }
    }

    inline static bool hasTransform(const tinygltf::Node & node)
    {
        return node.matrix.size() == 16U;
    }

    static void convertScale(iv_root_t root, const tinygltf::Node & node)
    {
        if (hasScale(node)) {
            spdlog::trace("converting scale [{:.4}, {:.4}, {:.4}] for gltf node", node.scale[0], node.scale[1], node.scale[2]);

            SoScale * scaleNode{ new SoScale };

            SoSFVec3f scaleVector;

            scaleVector.setValue(
                static_cast<float>(node.scale[0]),
                static_cast<float>(node.scale[1]),
                static_cast<float>(node.scale[2])
            );

            scaleNode->scaleFactor = scaleVector;

            root->addChild(scaleNode);
        }
    }

    inline static bool hasScale(const tinygltf::Node & node)
    {
        return node.scale.size() == 3U;
    }

    static void convertRotation(iv_root_t root, const tinygltf::Node & node)
    {
        if (hasRotation(node)) {
            spdlog::trace("converting rotation [{:.4}, {:.4}, {:.4}, {:.4}] for gltf node", node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);

            SoRotation * rotationNode{ new SoRotation };

            SoSFRotation rotation;

            rotation.setValue(
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2]),
                static_cast<float>(node.rotation[3])
            );

            rotationNode->rotation = rotation;

            root->addChild(rotationNode);
        }
    }

    inline static bool hasRotation(const tinygltf::Node & node)
    {
        return node.rotation.size() == 4U;
    }

    static void convertTranslation(iv_root_t root, const tinygltf::Node & node)
    {
        if (hasTranslation(node)) {

            spdlog::trace("converting translation [{:.4}, {:.4}, {:.4}] for gltf node", node.translation[0], node.translation[1], node.translation[2]);

            SoTranslation * translationNode{ new SoTranslation };

            SoSFVec3f translation;

            translation.setValue(
                static_cast<float>(node.translation[0]),
                static_cast<float>(node.translation[1]),
                static_cast<float>(node.translation[2])
            );

            translationNode->translation = translation;

            root->addChild(translationNode);
        }
    }

    inline static bool hasTranslation(const tinygltf::Node & node)
    {
        return node.translation.size() == 3U;
    }


    void convertMesh(iv_root_t root, size_t meshIndex)
    {
        spdlog::trace("converting gltf mesh with index {}", meshIndex);
        if (m_meshes.contains(meshIndex)) {
            spdlog::debug("re-using already converted gltf mesh with index {}", meshIndex);
            root->addChild(m_meshes.at(meshIndex));
        }
        else {
            const tinygltf::Mesh mesh{ m_gltfModel.meshes.at(meshIndex) };

            SoSeparator * meshNode{ new SoSeparator };

            const std::vector<tinygltf::Primitive> & primitives{ mesh.primitives };

            std::for_each(
                primitives.cbegin(),
                primitives.cend(),
                [this, meshNode] (const tinygltf::Primitive & primitive)
                {
                    convertPrimitive(meshNode, primitive);
                }
            );

            root->addChild(meshNode);
            m_meshes.insert(std::make_pair(meshIndex, meshNode));
        }
    }

    static constexpr std::string stringifyPrimitiveMode(int primitiveMode)
    {
        switch (primitiveMode) {
        case TINYGLTF_MODE_POINTS: return std::format("POINTS ({})", TINYGLTF_MODE_POINTS);
        case TINYGLTF_MODE_LINE: return std::format("LINE ({})", TINYGLTF_MODE_LINE);
        case TINYGLTF_MODE_LINE_LOOP: return std::format("LINE_LOOP ({})", TINYGLTF_MODE_LINE_LOOP);
        case TINYGLTF_MODE_LINE_STRIP: return std::format("LINE_STRIP ({})", TINYGLTF_MODE_LINE_STRIP);
        case TINYGLTF_MODE_TRIANGLES: return std::format("TRIANGLES ({})", TINYGLTF_MODE_TRIANGLES);
        case TINYGLTF_MODE_TRIANGLE_STRIP: return std::format("TRIANGLE_STRIP ({})", TINYGLTF_MODE_TRIANGLE_STRIP);
        case TINYGLTF_MODE_TRIANGLE_FAN: return std::format("TRIANGLE_FAN ({})", TINYGLTF_MODE_TRIANGLE_FAN);
        default: return std::format("UNKNOWN ({})", primitiveMode);
        }
    }

    void convertPrimitive(iv_root_t root, const tinygltf::Primitive & primitive)
    {
        spdlog::trace("converting gltf primitive with mode {}", stringifyPrimitiveMode(primitive.mode));

        switch (primitive.mode) {
        case TINYGLTF_MODE_TRIANGLES:
            convertTrianglesPrimitive(root, primitive);
            break;
        default:
            spdlog::warn("skipping primitive with unsupported mode {}", stringifyPrimitiveMode(primitive.mode));
        }
    }

    void convertTrianglesPrimitive(iv_root_t root, const tinygltf::Primitive & primitive)
    {
        spdlog::trace("converting gltf triangles primitive {}");

        convertMaterial(root, primitive);
        convertTriangles(root, primitive);
    }

    void convertMaterial(iv_root_t root, const tinygltf::Primitive & primitive)
    {
        if (hasMaterial(primitive)) {
            convertMaterial(root, static_cast<size_t>(primitive.material));
        }
    }

    static inline bool hasMaterial(const tinygltf::Primitive & primitive)
    {
        return primitive.material >= 0;
    }

    void convertMaterial(iv_root_t root, int materialIndex)
    {
        spdlog::trace("converting gltf material with index {}", materialIndex);
        if (m_materials.contains(materialIndex)) {
            spdlog::debug("re-using already converted gltf material with index {}", materialIndex);
            root->addChild(m_materials.at(materialIndex));
        }
        else {
            const tinygltf::Material material{ m_gltfModel.materials.at(materialIndex) };

            SoMaterial * materialNode = new SoMaterial;
            materialNode->diffuseColor = diffuseColor(material);
            root->addChild(materialNode);
            m_materials.insert(std::make_pair(materialIndex, materialNode));
        }

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

    void convertTriangles(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        if (hasIndices(primitive)) {
            const gltf_indices_t indices{ this->indices(primitive) };
            convertTriangles(root, convertPositions(root, indices, primitive), convertNormals(root, indices, primitive));
        }
        else {
            convertTriangles(root, convertPositions(root, primitive), convertNormals(root, primitive));
        }
    }

    static inline bool hasIndices(const tinygltf::Primitive & primitive)
    {
        return primitive.indices >= 0;
    }

    static void convertTriangles(iv_root_t root, const iv_indices_t & positionIndices, const iv_indices_t & normalIndices)
    {
        if (positionIndices.size() != normalIndices.size()) {
            throw std::invalid_argument(std::format("mismatching number of positions ({}) and normals ({})", positionIndices.size(), normalIndices.size()));
        }

        constexpr size_t triangle_strip_size{ 3U };

        if (positionIndices.size() % triangle_strip_size != 0) {
            throw std::invalid_argument(std::format("number of positions ({}) is not divisible by the triangle size ({})", positionIndices.size(), triangle_strip_size));
        }

        spdlog::trace("converting {} triangles from gltf primitive", positionIndices.size() / triangle_strip_size);

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

    iv_indices_t convertPositions(iv_root_t root, const gltf_indices_t & indices, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf positions from gltf primitive");

        const positions_t positions{ GltfIvWriter::positions(primitive) };
        const positions_t uniquePositions{ unique<position_t>(positions) };

        convertPositions(root, uniquePositions);

        return positionIndices(indices, positions, positionMap(uniquePositions));
    }

    iv_indices_t convertPositions(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf positions from gltf primitive");

        const positions_t positions{ GltfIvWriter::positions(primitive) };
        const positions_t uniquePositions{ unique<position_t>(positions) };

        convertPositions(root, uniquePositions);

        return positionIndices(positions, positionMap(uniquePositions));
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

    iv_indices_t convertNormals(iv_root_t root, const gltf_indices_t & indices, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf normals from gltf primitive");

        SoNormalBinding * normalBinding = new SoNormalBinding;
        normalBinding->value = SoNormalBinding::Binding::PER_VERTEX_INDEXED;
        root->addChild(normalBinding);

        const normals_t normals{ GltfIvWriter::normals(primitive) };

        const normals_t uniqueNormals{ unique(normals) };

        convertNormals(root, uniqueNormals);

        return normalIndices(indices, normals, normalMap(uniqueNormals));
    }

    iv_indices_t convertNormals(iv_root_t root, const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("converting gltf normals from gltf primitive");

        SoNormalBinding * normalBinding = new SoNormalBinding;
        normalBinding->value = SoNormalBinding::Binding::PER_VERTEX_INDEXED;
        root->addChild(normalBinding);

        const normals_t normals{ GltfIvWriter::normals(primitive) };

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

    static iv_indices_t normalIndices(const gltf_indices_t & indices, const normals_t & normals, const normal_map_t & normalMap)
    {
        spdlog::trace("create index for {} indexed normals", indices.size());

        iv_indices_t normalIndices;
        normalIndices.reserve(indices.size());

        std::transform(
            indices.cbegin(),
            indices.cend(),
            std::back_inserter(normalIndices),
            [&normals, &normalMap] (const gltf_index_t & index)
            {
                return normalMap.at(normals.at(index));
            }
        );

        return normalIndices;
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

    static iv_indices_t positionIndices(const gltf_indices_t & indices, const positions_t & positions, const position_map_t & positionMap)
    {
        spdlog::trace("create index for {} indexed positions", indices.size());

        iv_indices_t positionIndices;
        positionIndices.reserve(indices.size());

        std::transform(
            indices.cbegin(),
            indices.cend(),
            std::back_inserter(positionIndices),
            [&positions, &positionMap] (const gltf_index_t & index)
            {
                return positionMap.at(positions.at(index));
            }
        );

        return positionIndices;
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
            [&positionMap] (const position_t & position)
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

    template<class U, class T>
    static constexpr std::vector<U> cast(const std::vector<T> & source)
    {
        std::vector<U> target;
        target.reserve(source.size());
        std::copy(source.cbegin(), source.cend(), std::back_inserter(target));
        return target;
    }

    gltf_indices_t indices(const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("retrieve indices from primitive");
        const int accessorIndex{ primitive.indices };
        if (accessorIndex >= 0) {
            const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(static_cast<size_t>(accessorIndex));
            ensureAccessorType(accessor, TINYGLTF_TYPE_SCALAR);
            spdlog::debug("retrieving index of type {} with compoment type {}", stringifyAccessorType(accessor.type), stringifyAccessorComponentType(accessor.componentType));

            switch (accessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return cast<gltf_index_t, uint8_t>(accessorContents<uint8_t>(accessor));
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return cast<gltf_index_t, uint16_t>(accessorContents<uint16_t>(accessor));
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return cast<gltf_index_t, uint32_t>(accessorContents<uint32_t>(accessor));
            default: throw std::invalid_argument(std::format("component type {} is unsupported for indices", stringifyAccessorComponentType(accessor.componentType)));
            }
        }
        else {
            spdlog::warn("index accessor at index {} not found", accessorIndex);
            return {};
        }
    }

    positions_t positions(const tinygltf::Primitive & primitive) const
    {
        spdlog::trace("retrieve positions from primitive");
        const int accessorIndex{ primitive.attributes.at("POSITION") };
        if (accessorIndex >= 0) {
            const tinygltf::Accessor & accessor = m_gltfModel.accessors.at(static_cast<size_t>(accessorIndex));
            ensureAccessorType(accessor, TINYGLTF_TYPE_VEC3);
            ensureAccessorComponentType(accessor, TINYGLTF_COMPONENT_TYPE_FLOAT);
            return accessorContents<position_t>(accessor);
        }
        else {
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
            ensureAccessorComponentType(accessor, TINYGLTF_COMPONENT_TYPE_FLOAT);
            return accessorContents<normal_t>(accessor);
        }
        else {
            spdlog::warn("normals accessor at index {} not found", accessorIndex);
            return {};
        }
    }

    static constexpr std::string stringifyAccessorType(int accessorType)
    {
        switch (accessorType) {
        case TINYGLTF_TYPE_VEC2: return std::format("VEC2 ({})", TINYGLTF_TYPE_VEC2);
        case TINYGLTF_TYPE_VEC3: return std::format("VEC3 ({})", TINYGLTF_TYPE_VEC3);
        case TINYGLTF_TYPE_VEC4: return std::format("VEC4 ({})", TINYGLTF_TYPE_VEC4);
        case TINYGLTF_TYPE_MAT2: return std::format("MAT2 ({})", TINYGLTF_TYPE_MAT2);
        case TINYGLTF_TYPE_MAT3: return std::format("MAT3 ({})", TINYGLTF_TYPE_MAT3);
        case TINYGLTF_TYPE_MAT4: return std::format("MAT4 ({})", TINYGLTF_TYPE_MAT4);
        case TINYGLTF_TYPE_SCALAR: return std::format("SCALAR ({})", TINYGLTF_TYPE_SCALAR);
        case TINYGLTF_TYPE_VECTOR: return std::format("VECTOR ({})", TINYGLTF_TYPE_VECTOR);
        case TINYGLTF_TYPE_MATRIX: return std::format("MATRIX ({})", TINYGLTF_TYPE_MATRIX);
        default: return std::format("UNKNOWN ({})", accessorType);
        }
    }

    static void ensureAccessorType(const tinygltf::Accessor & accessor, int accessorType)
    {
        spdlog::trace("ensure gltf accessor type is {}", stringifyAccessorType(accessorType));

        if (accessor.type != accessorType) {
            throw std::domain_error(
                std::format(
                    "expected accessor type {} instead of {}",
                    stringifyAccessorType(accessorType),
                    stringifyAccessorType(accessor.type)
                )
            );
        }
    }

    static constexpr std::string stringifyAccessorComponentType(int accessorComponentType)
    {
        switch (accessorComponentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE: return std::format("BYTE ({})", TINYGLTF_COMPONENT_TYPE_BYTE);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return std::format("UNSIGNED_BYTE ({})", TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);
        case TINYGLTF_COMPONENT_TYPE_SHORT: return std::format("SHORT ({})", TINYGLTF_COMPONENT_TYPE_SHORT);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return std::format("UNSIGNED_SHORT ({})", TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
        case TINYGLTF_COMPONENT_TYPE_INT: return std::format("INT ({})", TINYGLTF_COMPONENT_TYPE_INT);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return std::format("UNSIGNED_INT ({})", TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return std::format("FLOAT ({})", TINYGLTF_COMPONENT_TYPE_FLOAT);
        case TINYGLTF_COMPONENT_TYPE_DOUBLE: return std::format("DOUBLE ({})", TINYGLTF_COMPONENT_TYPE_DOUBLE);
        default: return std::format("UNKNOWN ({})", accessorComponentType);
        }
    }

    static void ensureAccessorComponentType(const tinygltf::Accessor & accessor, int accessorComponentType)
    {
        spdlog::trace("ensure gltf accessor component type is {}", stringifyAccessorComponentType(accessorComponentType));

        if (accessor.componentType != accessorComponentType) {
            throw std::domain_error(
                std::format(
                    "expected accessor component type {} instead of {}",
                    stringifyAccessorComponentType(accessorComponentType),
                    stringifyAccessorComponentType(accessor.componentType)
                )
            );
        }
    }

    template<class T>
    static void ensureByteStrideMatchesType(const tinygltf::Accessor & accessor, const tinygltf::BufferView & bufferView)
    {
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
    }

    static void ensureByteOffsetWithinBuffer(size_t byteOffset, const tinygltf::Buffer & buffer)
    {
        if (byteOffset >= buffer.data.size()) {
            throw std::out_of_range(
                std::format(
                    "byte offset {} is outside of the range of a buffer with size {}",
                    byteOffset,
                    buffer.data.size()
                )
            );
        }
    }

    static void ensureByteOffsetPlusBytesToCopyWithinBuffer(size_t byteOffset, size_t bytesToCopy, const tinygltf::Buffer & buffer)
    {
        if (bytesToCopy > buffer.data.size() || byteOffset > buffer.data.size() - bytesToCopy) {
            throw std::out_of_range(
                std::format(
                    "byte offset ({}) plus the number of bytes to copy ({}) is beyond the length of the buffer ({})",
                    byteOffset,
                    bytesToCopy,
                    buffer.data.size()
                )
            );
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

        ensureByteStrideMatchesType<T>(accessor, bufferView);

        const int bufferIndex{ bufferView.buffer };
        if (bufferIndex < 0) {
            spdlog::warn("accessor buffer not found at index {}", bufferIndex);
            return {};
        }

        const tinygltf::Buffer & buffer = m_gltfModel.buffers.at(static_cast<size_t>(bufferIndex));

        const size_t byteOffset{ bufferView.byteOffset + accessor.byteOffset };
        const size_t bytesToCopy{ accessor.count * sizeof(T) };

        ensureByteOffsetWithinBuffer(byteOffset, buffer);
        ensureByteOffsetPlusBytesToCopyWithinBuffer(byteOffset, bytesToCopy, buffer);

        std::vector<T> contents;
        contents.resize(accessor.count);

        std::memcpy(&contents[0], &buffer.data[byteOffset], bytesToCopy);

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
    std::unordered_map<size_t, iv_root_t> m_nodes;
    std::unordered_map<size_t, iv_root_t> m_meshes;
    std::unordered_map<size_t, iv_material_t> m_materials;
};

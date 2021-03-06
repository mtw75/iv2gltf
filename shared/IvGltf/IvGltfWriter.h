#pragma once 
#include "IvGltf.h"
#include <Inventor/actions/SoCallbackAction.h>
#include <string>
#include <vector>
#include "tiny_gltf.h"

class SoSeparator;
class SoCallbackAction;
class SoPrimitiveVertex;
class SoNode; 

enum class GltfWritingMode { UNKNOWN, TRIANGLE, LINE };

class IVGLTF_EXPORT IvGltfWriter {
public:
    IvGltfWriter(SoSeparator * root);
    ~IvGltfWriter();
    bool write(std::string dtr);

    SoCallbackAction *m_action = nullptr; 
    static SoCallbackAction::Response preShapeCB(void * userdata, SoCallbackAction * action, const SoNode * node);
    static SoCallbackAction::Response postShapeCB(void * userdata, SoCallbackAction * action, const SoNode * node);
    static void triangle_cb(
            void * userdata,
            SoCallbackAction * action,
            const SoPrimitiveVertex * v1,
            const SoPrimitiveVertex * v2,
            const SoPrimitiveVertex * v3);
    static void line_cb(
        void* userdata,
        SoCallbackAction* action,
        const SoPrimitiveVertex* v1,
        const SoPrimitiveVertex* v2);
    void addTriangle(SbVec3f * vtx, SbVec3f * ntx, SbVec4f * txx, uint32_t * colors, const SbMatrix & mm);
    void addLineSegment(const SbVec3f& vecA, const SbVec3f& vecB, const SbMatrix& modelMatrix);
    void setWriteBinary(bool isBinary)
    {
        m_writeBinary = isBinary;
    }

protected:
    SoCallbackAction::Response onPostShape(SoCallbackAction * action, const SoNode * node);
    SoCallbackAction::Response onPreShape(SoCallbackAction * action, const SoNode * node);
    struct vec3 {
        float x;
        float y;
        float z;
    };
    struct uv {
        float u;
        float v;
    };    

    std::vector<vec3> m_positions; 
    std::vector<vec3> m_normals; 
    std::vector<uv> m_texCoords; 
    uv m_uvMin;
    uv m_uvMax;
    vec3 m_posMin;
    vec3 m_posMax;


    std::map<std::string, int> m_materialIndexByMatInfo; 
    std::vector<uint32_t> m_indices;
    tinygltf::Buffer m_buffer;
    tinygltf::Model m_model;
    tinygltf::Scene m_scene;
    SoSeparator * m_root=nullptr;
    bool m_writeBinary = false; 
    GltfWritingMode m_drawingMode{ GltfWritingMode::UNKNOWN };
};
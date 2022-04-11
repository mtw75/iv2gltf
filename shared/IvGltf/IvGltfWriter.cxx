
#include "IvGltfWriter.h"

#include "tiny_gltf.h"
#include <sstream>
#include <iostream>

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/SoPrimitiveVertex.h>
#ifdef _WIN32
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#endif 
#include <png++/png.hpp>

IvGltfWriter::IvGltfWriter(SoSeparator * root): m_root(root)
{
    if (m_root)
        m_root->ref();
    m_action = new SoCallbackAction;
    m_action->addPreCallback(SoShape::getClassTypeId(), preShapeCB, this);
    m_action->addPostCallback(SoShape::getClassTypeId(), postShapeCB, this);    
    m_action->addTriangleCallback(SoShape::getClassTypeId(), triangle_cb, this);    
    m_action->addLineSegmentCallback(SoShape::getClassTypeId(), line_cb, this);
}

IvGltfWriter::~IvGltfWriter()
{
    if (m_root) {
        m_root->unref();
    }
    delete (m_action);
}



template <typename T> uint32_t serialize(std::vector<T> const & from, std::vector<uint8_t> & to, std::size_t offset)
{
    if (from.empty()) {
        return 0;
    }
    uint32_t bytesToSerialize = sizeof(T) * static_cast<uint32_t>(from.size());

    to.resize(to.size() + bytesToSerialize);
    std::memcpy(&to[offset], &from[0], bytesToSerialize);

    return bytesToSerialize;
}

bool IvGltfWriter::write(std::string outputFilename)
{
    if (!m_root) {
        return false; 
    } 

    

    m_action->apply(m_root);

    // asset info
    tinygltf::Asset asset;
    asset.generator = "ivgltfwriter";
    asset.version = "2.0";
    m_model.asset = asset;
    
    
    // add scene 
    m_model.scenes.push_back(m_scene);
    
    // Save it to a file
    tinygltf::TinyGLTF gltf;
    gltf.WriteGltfSceneToFile(
            &m_model,
            outputFilename,
            true,   // embedImages
            true,   // embedBuffers
            true,   // pretty print
            m_writeBinary); // write binary
    
    return false;
}


SoCallbackAction::Response IvGltfWriter::onPreShape(SoCallbackAction * action, const SoNode * node)
{
    m_buffer.data.clear();
    m_indices.clear();
    m_positions.clear();
    m_normals.clear();
    m_texCoords.clear();
    float fmax = std::numeric_limits<float>::max();
    float fmin = std::numeric_limits<float>::min();
    m_uvMin = { fmax, fmax };
    m_uvMax = { fmin, fmin };
    m_posMin = { fmax, fmax, fmax };
    m_posMax = { fmin, fmin, fmin };
    return SoCallbackAction::CONTINUE;
}

std::string materialHash(SbColor ambient, SbColor diffuse, SbColor specular, SbColor emission, float shininess, float transparency) {
    std::ostringstream so;
    so << diffuse[0] << diffuse[1] << diffuse[2];
    return so.str();
}
SoCallbackAction::Response IvGltfWriter::onPostShape(SoCallbackAction * action, const SoNode * ivNode)
{
    // write buffers vbo 
    uint32_t currentOffset = 0;// m_buffer.data.size();
    uint32_t indexSize = serialize(m_indices, m_buffer.data, currentOffset+0);
    uint32_t positionSize = serialize(m_positions, m_buffer.data, currentOffset + indexSize);
    uint32_t normalSize = serialize(m_normals, m_buffer.data, currentOffset + indexSize + positionSize);
    uint32_t uvSize = serialize(m_texCoords, m_buffer.data, currentOffset + indexSize + positionSize + normalSize);    
    m_model.buffers.push_back(m_buffer);
    int currBufIdx = m_model.buffers.size() - 1;

    // write texure image buffer 

    int imgSize = 0;
    SbVec2s size;
    int nc = 0;
    const unsigned char* ivImg = action->getTextureImage(size, nc);
    //std::ostringstream si;
    //si << size[0] << size[1] << " " << nc;
    //std::cout << si.str() << std::endl;
    imgSize = size[0] * size[1] *nc;
    

    SbColor ambient, diffuse, specular, emission;
    float shininess = 0, transparency = 0;
    action->getMaterial(ambient, diffuse, specular, emission, shininess, transparency);
    uint32_t materialIdx = -1;




    if (imgSize > 0) {
        png::image< png::rgb_pixel > image(size[0], size[1]);
        for (png::uint_32 y = 0; y < image.get_height(); ++y)
        {
            for (png::uint_32 x = 0; x < image.get_width(); ++x)
            {
                const unsigned char* d = ivImg + (y * size[0] + x) * nc;
                image.set_pixel(x, y, png::rgb_pixel(d[0], d[1], d[2]));
            }
        }
        
        std::ostringstream sout; 
        image.write_stream(sout);
        //image.write(std::string("testout.png"));
        tinygltf::Buffer imgBuffer;
        std::string s = sout.str();
        std::vector<unsigned char> data(s.begin(), s.end()); //(size_t)imgSize);// (ba.begin(), ba.end());
        
        imgBuffer.data = data;
        imgBuffer.name = "imageBuffer";
        m_model.buffers.push_back(imgBuffer);

        tinygltf::BufferView bufferView;
        bufferView.buffer = m_model.buffers.size() - 1;
        bufferView.name = "imageBufferView";
        bufferView.byteLength = imgBuffer.data.size();
        m_model.bufferViews.push_back(bufferView);


        tinygltf::Image img;
        img.name = "image";
        img.mimeType = "image/png";
        img.bufferView = m_model.bufferViews.size() - 1; 
        // img.uri = "test.png";
        m_model.images.push_back(img);

        tinygltf::Sampler sampler;
        sampler.wrapS = action->getTextureWrapS();
        sampler.wrapT = action->getTextureWrapT();
        m_model.samplers.push_back(sampler);

        tinygltf::Texture texture;
        texture.source = 0;
        texture.sampler = 0;
        m_model.textures.push_back(texture);

        tinygltf::Material texmat;
        texmat.pbrMetallicRoughness.baseColorTexture.index = 0;
        texmat.pbrMetallicRoughness.metallicFactor = 0.0;
        texmat.name = "Texture";
        m_model.materials.push_back(texmat);
        materialIdx = m_model.materials.size() - 1;
    }
    else {
        std::string matHash = materialHash(ambient, diffuse, specular, emission, shininess, transparency);
        if (m_materialIndexByMatInfo.contains(matHash)) {
            materialIdx = m_materialIndexByMatInfo[matHash];
        }
        else {
            tinygltf::Material tmat;
            tmat.pbrMetallicRoughness.baseColorFactor = { diffuse[0], diffuse[1], diffuse[2], 1.0f };
            tmat.doubleSided = true;
            
            m_model.materials.push_back(tmat);
            materialIdx = m_model.materials.size()-1;
            m_materialIndexByMatInfo[matHash] = materialIdx;
        }
    }


    // Build the "views" into the "buffer"
    tinygltf::BufferView indexBufferView {};
    tinygltf::BufferView positionBufferView {};
    tinygltf::BufferView normalBufferView {};
    tinygltf::BufferView uvBufferView {};

    indexBufferView.buffer = currBufIdx;
    indexBufferView.byteLength = indexSize;
    indexBufferView.byteOffset = currentOffset + 0;
    indexBufferView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;

    positionBufferView.buffer = currBufIdx;
    positionBufferView.byteLength = positionSize;
    positionBufferView.byteOffset = currentOffset + indexSize;
    positionBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

    normalBufferView.buffer = currBufIdx;
    normalBufferView.byteLength = normalSize;
    normalBufferView.byteOffset = currentOffset + indexSize + positionSize;
    normalBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

    uvBufferView.buffer = currBufIdx;
    uvBufferView.byteLength = uvSize;
    uvBufferView.byteOffset = currentOffset + indexSize + positionSize + normalSize;
    uvBufferView.target = TINYGLTF_TARGET_ARRAY_BUFFER;

    uint32_t bufIdx = m_model.bufferViews.size();
    m_model.bufferViews.push_back(indexBufferView);
    m_model.bufferViews.push_back(positionBufferView);
    m_model.bufferViews.push_back(normalBufferView);
    m_model.bufferViews.push_back(uvBufferView);

    // Now we need "accessors" to know how to interpret each of those "views"
    tinygltf::Accessor indexAccessor {};
    tinygltf::Accessor positionAccessor {};
    tinygltf::Accessor normalAccessor {};
    tinygltf::Accessor uvAccessor {};

    
    indexAccessor.bufferView = bufIdx + 0;
    indexAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
    indexAccessor.count = static_cast<uint32_t>(m_indices.size());
    indexAccessor.type = TINYGLTF_TYPE_SCALAR;

    positionAccessor.bufferView = bufIdx + 1;
    positionAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    positionAccessor.count = static_cast<uint32_t>(m_positions.size());
    positionAccessor.type = TINYGLTF_TYPE_VEC3;

    normalAccessor.bufferView = bufIdx + 2;
    normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    normalAccessor.count = static_cast<uint32_t>(m_normals.size());
    normalAccessor.type = TINYGLTF_TYPE_VEC3;

    uvAccessor.bufferView = bufIdx + 3;
    uvAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
    uvAccessor.count = static_cast<uint32_t>(m_texCoords.size());
    uvAccessor.type = TINYGLTF_TYPE_VEC2;

    // My viewer requires min/max (shame)
    indexAccessor.minValues = {0};
    indexAccessor.maxValues = { (float)m_positions.size()-1 };
    positionAccessor.minValues = {m_posMin.x, m_posMin.y, m_posMin.z };
    positionAccessor.maxValues = { m_posMax.x, m_posMax.y, m_posMax.z };
    normalAccessor.minValues = {-1, -1, -1};
    normalAccessor.maxValues = {1, 1, 1};
    uvAccessor.minValues = {m_uvMin.u, m_uvMin.v};
    uvAccessor.maxValues = {m_uvMax.u, m_uvMax.v};
    

    uint32_t accessorIdx = m_model.accessors.size(); 
    m_model.accessors.push_back(indexAccessor);
    m_model.accessors.push_back(positionAccessor);
    m_model.accessors.push_back(normalAccessor);
    m_model.accessors.push_back(uvAccessor);
    

    
    

    //---

    // add mesh 
    tinygltf::Mesh mesh {};
    tinygltf::Primitive triMeshPrim {};
    std::ostringstream so;
    so << "Mesh_" << m_model.meshes.size(); 
    mesh.name = so.str();
    triMeshPrim.indices = accessorIdx + 0;                    // accessor 0
    triMeshPrim.attributes["POSITION"] = accessorIdx + 1;   // accessor 1
    triMeshPrim.attributes["NORMAL"] = accessorIdx + 2;       // accessor 2
    triMeshPrim.attributes["TEXCOORD_0"] = accessorIdx + 3; // accessor 3
    triMeshPrim.mode = TINYGLTF_MODE_TRIANGLES;
//    triMeshPrim.mode = TINYGLTF_MODE_LINE;

    // now material 
    // Create a simple material
    

    
    
    

    
    

    
    if (materialIdx != -1) {
        triMeshPrim.material = materialIdx;
    }
    mesh.primitives.push_back(triMeshPrim);

    m_model.meshes.push_back(mesh);

    // We need "nodes" to list what "meshes" to use...
    tinygltf::Node node {};
    int meshIdx = m_model.meshes.size() - 1;
    node.mesh = meshIdx;

    m_model.nodes.push_back(node);

    // We need a "scene" to list what "nodes" to use...
    int nodeIdx = m_model.nodes.size() - 1;
    m_scene.nodes.push_back(nodeIdx);
    return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response IvGltfWriter::preShapeCB(void * userdata, SoCallbackAction * action, const SoNode * node)
{

    IvGltfWriter * that = (IvGltfWriter *)userdata;
    return that->onPreShape(action, node);
}

SoCallbackAction::Response IvGltfWriter::postShapeCB(void * userdata, SoCallbackAction * action, const SoNode * node)
{
    IvGltfWriter * that = (IvGltfWriter *)userdata;
    return that->onPostShape(action, node);    
}

uint32_t toPackedColor(SoCallbackAction * action, const SoPrimitiveVertex * v)
{
    uint32_t result = 0;
    SbColor amb, dif, spec, em;
    float sh, tp;
    action->getMaterial(amb, dif, spec, em, sh, tp, v->getMaterialIndex());

    return dif.getPackedValue();
}

void IvGltfWriter::triangle_cb(
        void * userdata,
        SoCallbackAction * action,
        const SoPrimitiveVertex * vertex1,
        const SoPrimitiveVertex * vertex2,
        const SoPrimitiveVertex * vertex3)
{
    IvGltfWriter * that = (IvGltfWriter *)userdata;

    const SbVec3f points[] = {vertex1->getPoint(), vertex2->getPoint(), vertex3->getPoint()};

    const uint32_t colors[] = {
            toPackedColor(action, vertex1), toPackedColor(action, vertex2), toPackedColor(action, vertex3)};
    const SbVec3f normals[] = {vertex1->getNormal(), vertex2->getNormal(), vertex3->getNormal()};
    const SbVec4f textureCoords[] = {
            vertex1->getTextureCoords(), vertex2->getTextureCoords(), vertex3->getTextureCoords()};
    const SbMatrix modelMatrix = action->getModelMatrix();
    that->addTriangle((SbVec3f *)points, (SbVec3f *)normals, (SbVec4f *)textureCoords, (uint32_t *)colors, modelMatrix);
}

void IvGltfWriter::addTriangle(
        SbVec3f * points,
        SbVec3f * normals,
        SbVec4f * textureCoords,
        uint32_t * colors,
        const SbMatrix & modelMatrix)
{
    
    SbVec3f transformedPoint, transformedNormal;

    for (int j = 0; j < 3; j++) {
        modelMatrix.multVecMatrix(points[j], transformedPoint);
        m_positions.push_back({transformedPoint[0], transformedPoint[1], transformedPoint[2]});
        m_posMin = { std::min<float>(transformedPoint[0], m_posMin.x), std::min<float>(transformedPoint[1], m_posMin.y),std::min<float>(transformedPoint[2], m_posMin.z) };
        m_posMax = { std::max<float>(transformedPoint[0], m_posMax.x), std::max<float>(transformedPoint[1], m_posMax.y),std::max<float>(transformedPoint[2], m_posMax.z) };
        modelMatrix.multDirMatrix(normals[j], transformedNormal);
        m_normals.push_back({transformedNormal[0], transformedNormal[1], transformedNormal[2]});
        m_indices.push_back(m_positions.size() - 1);
        uv texUv = {textureCoords[j][0], textureCoords[j][1]};
        
        m_texCoords.push_back(texUv);
        m_uvMin = {std::min<float>(texUv.u, m_uvMin.u), std::min<float>(texUv.v, m_uvMin.v)};
        m_uvMax = {std::max<float>(texUv.u, m_uvMax.u), std::max<float>(texUv.v, m_uvMax.v)};        

        //m_colors.append(colors[j]);
    }
}

void IvGltfWriter::line_cb(
    void* userdata,
    SoCallbackAction* action,
    const SoPrimitiveVertex* vertex1,
    const SoPrimitiveVertex* vertex2)
{
    IvGltfWriter* that = (IvGltfWriter*)userdata;
    const SbMatrix modelMatrix = action->getModelMatrix();
    that->addLineSegment(vertex1->getPoint(), vertex2->getPoint(), modelMatrix);
}

void IvGltfWriter::addLineSegment(
    const SbVec3f& vecA,
    const SbVec3f& vecB,
    const SbMatrix& modelMatrix)
{
    for (const SbVec3f& point : { vecA, vecB }) {
        SbVec3f transformedPoint;
        modelMatrix.multVecMatrix(point, transformedPoint);
        m_positions.push_back({ transformedPoint[0], transformedPoint[1], transformedPoint[2] });

        m_posMin = {
                std::min<float>(transformedPoint[0], m_posMin.x),
                std::min<float>(transformedPoint[1], m_posMin.y),
                std::min<float>(transformedPoint[2], m_posMin.z) };
        m_posMax = {
                std::max<float>(transformedPoint[0], m_posMax.x),
                std::max<float>(transformedPoint[1], m_posMax.y),
                std::max<float>(transformedPoint[2], m_posMax.z) };
        m_indices.push_back(m_positions.size() - 1);
    }
}
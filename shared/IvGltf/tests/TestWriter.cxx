#include <gtest/gtest.h>
#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTransform.h>
#include "IvGltfWriter.h"

TEST(IvGltfWriter, WriteSimpleCube)
{
    SoSeparator* s = new SoSeparator;
    SoMaterial* m1 = new SoMaterial;
    m1->diffuseColor = SbColor(1, 0, 0);
    SoCube* c = new SoCube;
    s->addChild(m1);
    s->addChild(c);
 
    IvGltfWriter gltf(s);
    gltf.write("testwriter_simplecube.gltf");
}

TEST(IvGltfWriter, WriteSimpleMultiCube)
{
    SoSeparator* s = new SoSeparator;
    SoMaterial* m1 = new SoMaterial;
    m1->diffuseColor = SbColor(1, 0, 0);
    SoMaterial* m2 = new SoMaterial;
    m2->diffuseColor = SbColor(0, 1, 0);
    SoCube* c = new SoCube;
    SoTransform* t = new SoTransform;
    t->translation = SbVec3f(0, 0, 3);
    s->addChild(m1);
    s->addChild(c);
    s->addChild(m2);
    s->addChild(t);
    s->addChild(c);
    s->addChild(t);
    s->addChild(c);
    s->addChild(m1);
    s->addChild(t);
    s->addChild(c);
    IvGltfWriter gltf(s);
    gltf.write("testwriter_multicube.gltf");
}

TEST(IvGltfWriter, WriteTexture)
{
    SoSeparator* s = new SoSeparator;
    SoMaterial* m1 = new SoMaterial;
    m1->diffuseColor = SbColor(1, 1, 1);
    SoTexture2* t = new SoTexture2;
    const unsigned char pixels[2 * 2 * 3] = {};
    t->image.setValue(SbVec2s(2, 2), 3, pixels);
    //t->filename.setValue("test.png");

    SoCube* c = new SoCube;
    s->addChild(m1);
    s->addChild(t);
    s->addChild(c);
    //Iv3dUtils::save(s, "testOut.iv");
    IvGltfWriter gltf(s);
    gltf.write("testwriter_texture.gltf"); 
    /*
    tinygltf::Model model;
    tinygltf::TinyGLTF ctx;
    std::string err;
    std::string warn;

    bool ret = ctx.LoadASCIIFromFile(&model, &err, &warn, "testwriter.gltf");
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }
    */
}

int main(int ac, char* av[])
{
	testing::InitGoogleTest(&ac, av);
	SoDB::init();
	return RUN_ALL_TESTS();
}
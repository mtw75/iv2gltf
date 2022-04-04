#include <gtest/gtest.h>
#include <Inventor/SoDB.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoTransform.h>
#include "IvGltfWriter.h"
#ifdef _WIN32
#define strerror_r(errno,buf,len) strerror_s(buf,len,errno)
#endif 
#include <png++/png.hpp>

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

    png::image< png::rgb_pixel > image("test.png");    
    int sx = image.get_width();
    int sy = image.get_height();
    int nc = 3; 
    std::vector<unsigned char> data((size_t)sy* sx*nc);
    
    for (png::uint_32 y = 0; y < sy; ++y)
    {
        for (png::uint_32 x = 0; x < sx; ++x)
        {
            auto p = image.get_pixel(x,y);
            data[nc * (y * sx + x)] = p.red;
            data[nc * (y * sx + x)+1] = p.green;
            data[nc * (y * sx + x)+2] = p.blue;            
        }
    }
    SoTexture2* t = new SoTexture2;
    
    t->image.setValue(SbVec2s(sx, sy), 3, data.data());
    //t->filename.setValue("test.png");

    SoCube* c = new SoCube;
    s->addChild(m1);
    s->addChild(t);
    s->addChild(c);
    
    IvGltfWriter gltf(s);
    gltf.write("testwriter_texture.gltf"); 
    
}

int main(int ac, char* av[])
{
	testing::InitGoogleTest(&ac, av);
	SoDB::init();
	return RUN_ALL_TESTS();
}
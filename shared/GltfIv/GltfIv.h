#pragma once 

#if defined(_WIN32)
#    if defined(GltfIv_EXPORTS)
#        define GLTFIV_EXPORT __declspec(dllexport)
#    else
#        define GLTFIV_EXPORT __declspec(dllimport)
#    endif /* GltfIv_EXPORTS */
#else      /* defined (_WIN32) */
#    define GLTFIV_EXPORT
#endif

#include <tiny_gltf.h>

#include <string>
#include <optional>

class GLTFIV_EXPORT GltfIv
{
public:
    static std::optional<tinygltf::Model> read(std::string filename);
};
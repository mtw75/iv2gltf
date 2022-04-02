#pragma once 

#if defined(_WIN32)
#    if defined(IvGltf_EXPORTS)
#        define IVGLTF_EXPORT __declspec(dllexport)
#    else
#        define IVGLTF_EXPORT __declspec(dllimport)
#    endif /* IvGltf_EXPORTS */
#else      /* defined (_WIN32) */
#    define IVGLTF_EXPORT
#endif

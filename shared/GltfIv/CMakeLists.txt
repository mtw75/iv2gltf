find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

set(CMAKE_CXX_STANDARD 20)

set(SRC
	GltfIv.h
	GltfIv.cxx
	GltfIvWriter.h
	GltfIvWriter.cxx
)

add_library(GltfIv ${SRC})

target_link_libraries(GltfIv Coin::Coin)

target_include_directories(GltfIv 
	PUBLIC ${TINYGLTF_INCLUDE_DIRS}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

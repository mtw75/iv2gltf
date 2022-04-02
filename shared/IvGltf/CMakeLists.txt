find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

set(SRC
	IvGltfReader.h
	IvGltfReader.cxx
	IvGltfWriter.h
	IvGltfWriter.cxx
	IvGltf.h
	IvGltf.cxx
)

add_library(IvGltf SHARED ${SRC})
target_link_libraries(IvGltf Coin3d)

target_include_directories(IvGltf PRIVATE ${TINYGLTF_INCLUDE_DIRS})
target_include_directories(IvGltf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

INSTALL(TARGETS IvGltf RUNTIME DESTINATION .)
add_subdirectory(tests)
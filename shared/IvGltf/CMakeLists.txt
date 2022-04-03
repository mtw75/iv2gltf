
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(PNGPP_INCLUDE_DIRS "png++/color.hpp")
set(CMAKE_CXX_STANDARD 20)
set(SRC
	IvGltfReader.h
	IvGltfReader.cxx
	IvGltfWriter.h
	IvGltfWriter.cxx
	IvGltf.h
	IvGltf.cxx
)

add_library(IvGltf SHARED ${SRC})
target_link_libraries(IvGltf Coin::Coin simage::simage)

target_include_directories(IvGltf 
	PRIVATE ${TINYGLTF_INCLUDE_DIRS} ${PNGPP_INCLUDE_DIRS} 
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)




INSTALL(TARGETS IvGltf RUNTIME DESTINATION .)
add_subdirectory(tests)


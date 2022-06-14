
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(PNGPP_INCLUDE_DIRS "png++/color.hpp")
find_package(libpng CONFIG REQUIRED)
set(CMAKE_CXX_STANDARD 20)
set(SRC
	IvGltfWriter.h
	IvGltfWriter.cxx
	IvGltf.h
	IvGltf.cxx
)
#target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
add_library(IvGltf SHARED ${SRC})
target_link_libraries(IvGltf Coin::Coin simage::simage png)

target_include_directories(IvGltf 
	PRIVATE ${TINYGLTF_INCLUDE_DIRS} ${PNGPP_INCLUDE_DIRS} 
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)




INSTALL(TARGETS IvGltf RUNTIME DESTINATION .)
add_subdirectory(tests)


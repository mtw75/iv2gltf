find_package(coin CONFIG REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_path(PNGPP_INCLUDE_DIRS "png++/color.hpp")

set(SRC
	IvGltfReader.h
	IvGltfReader.cxx
	IvGltfWriter.h
	IvGltfWriter.cxx
	IvGltf.h
	IvGltf.cxx
)

add_library(IvGltf SHARED ${SRC})
target_link_libraries(IvGltf Coin::Coin)

target_include_directories(IvGltf 
	PRIVATE ${TINYGLTF_INCLUDE_DIRS} ${PNGPP_INCLUDE_DIRS} 
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)


if(WIN32) 
	set(VCPKGDIR ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})
	set(COIN4_DLL ${VCPKGDIR}/bin/Coin4.dll)
	set(COIN4_DEBUG_DLL ${VCPKGDIR}/debug/bin/Coin4d.dll)
	install(FILES ${COIN4_DLL} DESTINATION . CONFIGURATIONS Release MinSizeRel) 
	install(FILES ${COIN4_DEBUG_DLL} DESTINATION . COMPONENT ${IPO_INSTALL_COMPONENT} CONFIGURATIONS Debug)
endif()

INSTALL(TARGETS IvGltf RUNTIME DESTINATION .)
add_subdirectory(tests)


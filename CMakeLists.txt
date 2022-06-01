cmake_minimum_required(VERSION 3.22)

if(CMAKE_TOOLCHAIN_FILE)
	message(STATUS "Using vcpkg from ${CMAKE_TOOLCHAIN_FILE}")
else()
	message(FATAL_ERROR "Could not find vcpkg")
endif()



PROJECT(iv2gltf)

message(STATUS "INSTALLED:" ${VCPKG_INSTALLED_DIR})
include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()


find_package(coin CONFIG REQUIRED)
find_package(simage CONFIG REQUIRED)
if(WIN32) 
	set(VCPKGDIR ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET})
	set(COIN4_DLL ${VCPKGDIR}/bin/Coin4.dll)
	set(COIN4_DEBUG_DLL ${VCPKGDIR}/debug/bin/Coin4d.dll)
	
	set(SIMAGE_DLL ${VCPKGDIR}/bin/simage1.dll)
	set(SIMAGE_DEBUG_DLL ${VCPKGDIR}/debug/bin/simage1d.dll)

	set(PNG_DLL ${VCPKGDIR}/bin/libpng16.dll)
	set(PNG_DEBUG_DLL ${VCPKGDIR}/debug/bin/libpng16d.dll)

	set(ZLIB_DLL ${VCPKGDIR}/bin/zlib1.dll)
	set(ZLIB_DEBUG_DLL ${VCPKGDIR}/debug/bin/zlibd1.dll)

	install(FILES ${COIN4_DLL} ${SIMAGE_DLL} ${PNG_DLL} ${ZLIB_DLL} DESTINATION . CONFIGURATIONS Release MinSizeRel) 
	install(FILES ${COIN4_DEBUG_DLL} ${SIMAGE_DEBUG_DLL} ${PNG_DEBUG_DLL} ${ZLIB_DEBUG_DLL} DESTINATION . COMPONENT ${IPO_INSTALL_COMPONENT} CONFIGURATIONS Debug)
endif()

add_subdirectory(shared/IvGltf)
add_subdirectory(shared/GltfIv)
add_subdirectory(apps/iv2gltf)
add_subdirectory(apps/gltf2iv)

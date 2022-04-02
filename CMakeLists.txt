#cmake_minimum_required(VERSION 3.22)
 cmake_minimum_required(VERSION 3.22)

if(CMAKE_TOOLCHAIN_FILE)
	message(STATUS "Using vcpkg from ${CMAKE_TOOLCHAIN_FILE}")
else()
	message(FATAL_ERROR "Could not find vcpkg")
endif()



PROJECT(iv2gltf)

message(STATUS "INSTALLED:" ${VCPKG_INSTALLED_DIR})
install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/gtest.dll ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/gtest_main.dll DESTINATION . )

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(shared/IvGltf)
add_subdirectory(apps/iv2gltf)
enable_testing()
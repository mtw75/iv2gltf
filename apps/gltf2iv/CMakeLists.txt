set(CMAKE_CXX_STANDARD 20)

find_package(cxxopts REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(gltf2iv main_gltf2iv.cxx)
target_link_libraries(gltf2iv GltfIv cxxopts::cxxopts fmt::fmt-header-only spdlog::spdlog_header_only)

target_compile_options(gltf2iv 
	PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall -Werror
		  >
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4 /WX
		  >
)

install(TARGETS gltf2iv DESTINATION .)
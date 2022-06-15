set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(cxxopts REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(gltf2iv main_gltf2iv.cxx)
target_link_libraries(gltf2iv GltfIv cxxopts::cxxopts fmt::fmt-header-only spdlog::spdlog_header_only)

target_compile_options(gltf2iv 
	PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall -Wno-c++98-compat -Wno-c++98-compat-pedantic -Werror
		  >
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4 /external:anglebrackets /external:W0 /WX
		  >
)
if(WIN32)
set_property(TARGET gltf2iv
             PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS gltf2iv DESTINATION .)

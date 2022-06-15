set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

set(SRC
	GltfIv.h
	GltfIv.cxx
	GltfIvWriter.h
)


if (WIN32)
	add_library(GltfIv STATIC ${SRC})
else()
	add_library(GltfIv SHARED ${SRC})
endif()

target_link_libraries(GltfIv Coin::Coin fmt::fmt-header-only spdlog::spdlog_header_only Microsoft.GSL::GSL ${CMAKE_DL_LIBS})

target_include_directories(GltfIv 
	PUBLIC ${TINYGLTF_INCLUDE_DIRS}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(GltfIv 
	PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall -Wno-c++98-compat -Wno-c++98-compat-pedantic -Werror
		  >
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4 /external:anglebrackets /external:W0 /WX
		  >
)

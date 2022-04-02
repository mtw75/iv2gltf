find_package(cxxopts CONFIG REQUIRED)
add_executable(iv2gltf main_iv2gltf.cxx)
target_link_libraries(iv2gltf IvGltf cxxopts::cxxopts)
install(TARGETS iv2gltf DESTINATION .)
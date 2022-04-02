find_package(GTest REQUIRED)
add_executable(TestWriter TestWriter.cxx)
target_link_libraries(TestWriter IvGltf GTest::gtest GTest::gtest_main)
install(TARGETS TestWriter DESTINATION .)

gtest_discover_tests(TestWriter WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX} DISCOVERY_MODE POST_BUILD)


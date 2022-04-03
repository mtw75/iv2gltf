
add_executable(TestWriter TestWriter.cxx)
target_link_libraries(TestWriter IvGltf gtest )
install(TARGETS TestWriter DESTINATION .)	# this makes it available for debugging in vs 
install(FILES test.png DESTINATION .)
add_test(NAME TestWriter_Test COMMAND TestWriter WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX} ) # this adds it to ctest 
#gtest_discover_tests(TestWriter 
#    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX} 
#    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX} 
#    DISCOVERY_MODE POST_BUILD)


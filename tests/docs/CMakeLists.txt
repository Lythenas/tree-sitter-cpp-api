# NOTE: This "executable" does nothing it basically only ensures that the code
# in the documentation compiles.

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} DOCTEST_SOURCES)

add_executable(${PROJECT_NAME}-doc-tests ${DOCTEST_SOURCES})
target_include_directories(${PROJECT_NAME}-doc-tests PRIVATE ${MiniLua_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME}-doc-tests
    PRIVATE ${PROJECT_NAME}
    PRIVATE TreeSitterLua)

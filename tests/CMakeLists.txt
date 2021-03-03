add_executable(${PROJECT_NAME}-tests
    main.cpp
    unit_tests.cpp)
target_include_directories(${PROJECT_NAME}-tests PRIVATE ${tree-sitter_SOURCE_DIR}/lib/include)
# add include directory to be able to test internal headers
target_include_directories(${PROJECT_NAME}-tests PRIVATE ${MiniLua_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME}-tests
        PRIVATE ${PROJECT_NAME}
        PRIVATE Catch2::Catch2
        PRIVATE TreeSitterLua)

if(COVERAGE)
    setup_target_for_coverage(${PROJECT_NAME}-tests-coverage ${PROJECT_NAME}-tests coverage)
endif()

add_subdirectory(docs)

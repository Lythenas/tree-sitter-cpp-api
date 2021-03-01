aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)

add_library(${PROJECT_NAME} STATIC ${SRC_LIST})
target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_options(${PROJECT_NAME} PUBLIC -fPIC)

target_link_libraries(${PROJECT_NAME}
    PUBLIC TreeSitter)



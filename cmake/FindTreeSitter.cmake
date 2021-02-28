# Finds the Tree-Sitter library
#
# Adds the following imported targets
#
#   TreeSitter

find_path(TreeSitter_SOURCE_DIR
    NAMES Makefile
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/extern/tree-sitter"
    NO_DEFAULT_PATH)

find_path(TreeSitter_INCLUDE_DIR
    NAMES tree_sitter/api.h tree_sitter/parser.h
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/extern/tree-sitter/lib/include"
    NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TreeSitter
    REQUIRED_VARS TreeSitter_SOURCE_DIR TreeSitter_INCLUDE_DIR)

# build tree-sitter static library using the provided Makefile
add_custom_command(OUTPUT libtree-sitter.a
    COMMAND make -C "${TreeSitter_SOURCE_DIR}" libtree-sitter.a
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Building Tree-Sitter"
    )
add_custom_target(TreeSitter_PRIVATE
    DEPENDS libtree-sitter.a)

add_library(TreeSitter STATIC IMPORTED)
set_target_properties(TreeSitter
    PROPERTIES IMPORTED_LOCATION "${TreeSitter_SOURCE_DIR}/libtree-sitter.a")
target_include_directories(TreeSitter INTERFACE "${TreeSitter_INCLUDE_DIR}")
add_dependencies(TreeSitter TreeSitter_PRIVATE)

# Finds the Tree-Sitter-Lua grammar library
#
# Adds the following imported targets
#
#   TreeSitterLua

find_path(TreeSitterLua_SOURCE_DIR
    NAMES parser.c scanner.cc
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/extern/tree-sitter-lua/src"
    NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TreeSitterLua
    REQUIRED_VARS TreeSitterLua_SOURCE_DIR)

add_library(TreeSitterLua STATIC
    "${TreeSitterLua_SOURCE_DIR}/parser.c"
    "${TreeSitterLua_SOURCE_DIR}/scanner.cc")
target_link_libraries(TreeSitterLua PUBLIC TreeSitter)
set_property(TARGET TreeSitterLua PROPERTY POSITION_INDEPENDENT_CODE ON)

#include <tree_sitter/tree_sitter.hpp>

//! [Define Language]
extern "C" const TSLanguage* tree_sitter_lua();
const ts::Language LUA_LANGUAGE{tree_sitter_lua()};
//! [Define Language]

//! [Use Parser]
void use_parser() {
    ts::Parser parser(LUA_LANGUAGE);
    ts::Tree tree = parser.parse_string("print('Hello, World!')");
}
//! [Use Parser]

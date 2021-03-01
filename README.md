# Tree-Sitter C++ Api

This library is a C++ wrapper for Tree-Sitters C api.

The Api is analogous to the official Tree-Sitter api but all the resource
management is done automatically by the classes. For example: Normally you would
have to create and delete a Tree-Sitter parser with `ts_parser_new` and
`ts_parser_delete`:

```c
TSParser* parser = ts_parser_new();
// set language and use
ts_parser_delete(parser);
```

With this library the constructor and destructor of `Parser` will take care of
this automatically:

```cpp
Parser parser{language};
// use
// will delete automatically when `parser` is destructed (i.e. goes out of scope)
```

## Additional Functionality

This library mostly just copies the api from Tree-Sitter but there are a few
additional pieces of functionality that is not directly offered by Tree-Sitter:

- `Tree` stores a copy of the parsed source code so you can retrieve the text of
  a node (`Node::text`)
- `edit_tree` can apply multiple edits to the tree at once and will return
  adjusted ranges of the applied edit (because early edits might move code
  around and change the line/column number of later edits). Currently this is
  very limited (see the docs).

## Usage

```cpp
extern "C" const TSLanguage* tree_sitter_lua();
const ts::Language LUA_LANGUAGE{tree_sitter_lua()};
```

## TODOs

- [ ] Test Queries
- [ ] Allow parsing without keeping a copy of the string in the `Tree` (but we
  need a way to access the original string so that `Node::text` can work)



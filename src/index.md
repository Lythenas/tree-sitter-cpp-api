# Tree-Sitter {#mainpage}

\tableofcontents

To use the library `#include <tree_sitter/tree_sitter.hpp>`. Everything lives in
the namespace [ts](\ref ts).

## Usage Examples

First you have to define the `ts::Language` you want to use:

\snippet example.cpp Define Language

\note You also have to link the compiled grammar.

\todo How to link the grammar

Now you can create and use a `ts::Parser` in a function:

\snippet example.cpp Use Parser

As you can see parsing returns a `ts::Tree`. You can walk this tree with a
`ts::Cursor` or query it using `ts::Query` or manually walk the nodes by calling
`ts::Tree::root_node`. Additionally you can edit the tree using
`ts::Tree::edit` (editing is currently relatively limited).

\note The current implementation of `Parser` and `Tree` will copy the string
into the tree. This is done to allow you to get the text content of a node
later on.





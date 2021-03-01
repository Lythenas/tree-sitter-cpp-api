#ifndef TREE_SITTER_HPP
#define TREE_SITTER_HPP

#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tree_sitter/api.h>
#include <utility>
#include <vector>

extern "C" const TSLanguage* tree_sitter_lua();

/**
 * @brief C++ wrapper for the C Tree-Sitter API.
 *
 * Wrapper types and helper functions for Tree-Sitter.
 *
 * Some of the methods and types by default use the lua tree-sitter grammar but
 * there are always also functions that accept a language as parameter.
 */
namespace ts {

/**
 * @brief Base exception class for errors in the Tree-Sitter wrapper.
 */
class TreeSitterException {};

/**
 * @brief Version missmatch between Tree-Sitter and the Language grammar.
 *
 * Thrown in the constructor of Parser if the version of Tree-Sitter and
 * the Language are not compatible.
 *
 * Check the version with TREE_SITTER_VERSION, TREE_SITTER_MIN_VERSION and
 * Language::version.
 */
class ParserLanguageException : public TreeSitterException {
public:
    /**
     * @brief Error message.
     */
    [[nodiscard]] const char* what() const noexcept;
};

/**
 * @brief Thrown by Parser::parse_string (should never actually be thrown).
 *
 * Because we:
 *
 * - always set a language
 * - never set a timeout
 * - never set the cancellation flag
 */
class ParseFailureException : public TreeSitterException {
public:
    /**
     * @brief Error message.
     */
    [[nodiscard]] const char* what() const noexcept;
};

/**
 * @brief Thrown by the constructor of Node if you try to create a null node.
 *
 * This should rarely be thrown.
 */
class NullNodeException : public TreeSitterException {
public:
    /**
     * @brief Error message.
     */
    [[nodiscard]] const char* what() const noexcept;
};

/**
 * @brief Syntax error in a Query string.
 *
 * Thrown by the constructor of Query if there is an error in the syntax of
 * the query string.
 *
 * Contains the raw error type from Tree-Sitter and the position of the error
 * in the query string.
 */
class QueryException : public TreeSitterException, public std::runtime_error {
    const TSQueryError error_;
    const std::uint32_t error_offset_;

public:
    QueryException(TSQueryError error, std::uint32_t error_offset);

    /**
     * @brief Raw Tree-Sitter query error.
     */
    [[nodiscard]] TSQueryError query_error() const;
    /**
     * @brief Offset of the error in the query string.
     */
    [[nodiscard]] std::uint32_t error_offset() const;
};

/**
 * @brief Base class for exceptions related to applying edits to the tree.
 *
 * Subclasses of this are thrown by Tree::edit.
 */
class EditException : public TreeSitterException {};

/**
 * @brief Newlines are not allowed in [Edit](@ref Edit)s.
 *
 * Thrown by Tree::edit if any of the edits contain newlines.
 */
class MultilineEditException : public EditException, public std::runtime_error {
public:
    MultilineEditException();
};

/**
 * @brief Overlapping [Edit](@ref Edit)s are not allowed.
 *
 * Thrown by Tree::edit if any of the edits overlap.
 */
class OverlappingEditException : public EditException, public std::runtime_error {
public:
    OverlappingEditException();
};

/**
 * @brief Empty [Edit](@ref Edit)s are not allwed.
 *
 * Thrown by Tree::edit if any of the edits have size zero.
 */
class ZeroSizedEditException : public EditException, public std::runtime_error {
public:
    ZeroSizedEditException();
};

/**
 * @brief Tree-Sitter current language version.
 *
 * Version for langauges created using the current tree-sitter version.
 *
 * Can be thought of as the max version for langauges.
 */
const std::size_t TREE_SITTER_VERSION = TREE_SITTER_LANGUAGE_VERSION;

/**
 * @brief Tree-Sitter minimum supported language version.
 *
 * Minimum supported version of languages.
 */
const std::size_t TREE_SITTER_MIN_VERSION = TREE_SITTER_MIN_COMPATIBLE_LANGUAGE_VERSION;

/**
 * @brief Numeric representation of the type of a node.
 */
using TypeId = TSSymbol;

/**
 * @brief Numeric representation of a field.
 */
using FieldId = TSFieldId;

/**
 * @brief Kind of a TypeId.
 *
 * Analogous to Tree-Sitters TSSymbolType.
 */
enum class TypeKind {
    /**
     * @brief Named.
     */
    Named,
    /**
     * @brief Anonymous.
     */
    Anonymous,
    /**
     * @brief Hidden (should not be returned by the Api).
     */
    Hidden,
};

/**
 * @brief %Location in source code as row and column.
 *
 * Supports the comparison operators.
 */
struct Point {
    /**
     * @brief Row in the source code.
     */
    std::uint32_t row;
    /**
     * @brief Column in the source code.
     */
    std::uint32_t column;

    /**
     * @brief Pretty print to string.
     *
     * Returns the Point as a pretty printed string. By default the row and
     * column start at 0 (which is not usually how code locations are counted).
     * If you want the row and column to start at 1 you need to call it with
     * parameter `true`.
     *
     * @param start_at_one Default to `false`.
     *      If `true` the row and column will be printed starting at 1 otherwise
     *      it starts at 0.
     */
    std::string pretty(bool start_at_one = false) const;
};

bool operator==(const Point&, const Point&);
bool operator!=(const Point&, const Point&);
bool operator<(const Point&, const Point&);
bool operator<=(const Point&, const Point&);
bool operator>(const Point&, const Point&);
bool operator>=(const Point&, const Point&);
std::ostream& operator<<(std::ostream&, const Point&);

/**
 * @brief %Location in source code as row, column and byte offset.
 *
 * Supports the comparison operators. But you should only compare locations
 * created from the same source.
 */
struct Location {
    /**
     * @brief Row and colunm in the source code.
     */
    Point point;
    /**
     * @brief Byte offset in the source code.
     *
     * Absolute position from the start of the source code.
     */
    std::uint32_t byte;
};

bool operator==(const Location&, const Location&);
bool operator!=(const Location&, const Location&);
bool operator<(const Location&, const Location&);
bool operator<=(const Location&, const Location&);
bool operator>(const Location&, const Location&);
bool operator>=(const Location&, const Location&);
std::ostream& operator<<(std::ostream&, const Location&);

/**
 * @brief %Range in the source code (start and end [Point](@ref Point)s).
 *
 * Supports the equality operators.
 */
struct Range {
    /**
     * @brief Start of the range.
     */
    Location start;
    /**
     * @brief End of the range (exclusive).
     */
    Location end;

    /**
     * @brief Check if two ranges overlap.
     */
    bool overlaps(const Range&) const;
};

bool operator==(const Range&, const Range&);
bool operator!=(const Range&, const Range&);
std::ostream& operator<<(std::ostream&, const Range&);
std::ostream& operator<<(std::ostream&, const std::vector<Range>&);

/**
 * @brief Used to modify the source code and parse tree.
 *
 * Contains the Range that should be replaced and the string it should be
 * replaced with.
 *
 * Use this with Tree::edit.
 *
 * Note that the range and replacement string don't need to have the same size.
 *
 * Supports the equality operators.
 */
struct Edit {
    /**
     * @brief The range to replace in the source code.
     */
    Range range;
    /**
     * @brief The replacement.
     */
    std::string replacement;
};

bool operator==(const Edit&, const Edit&);
bool operator!=(const Edit&, const Edit&);
std::ostream& operator<<(std::ostream&, const Edit&);
std::ostream& operator<<(std::ostream&, const std::vector<Edit>&);

/**
 * @brief Tree-Sitter language grammar.
 *
 * This can be inspected (e.g. the nodes it can produce) and used for parsing.
 *
 * Use this when creating the parser.
 */
class Language {
    const TSLanguage* lang;

public:
    /**
     * @brief Constructor to create a Language from a raw TSLanguage pointer.
     */
    Language(const TSLanguage*) noexcept;

    /**
     * @brief Get the raw TSLanguage pointer.
     *
     * \warning Use with care. Only intended for internal use in the wrapper
     * types. Never free or delete this pointer.
     */
    [[nodiscard]] const TSLanguage* raw() const;

    /**
     * @brief The number of distinct node types in the language.
     */
    [[nodiscard]] std::uint32_t node_type_count() const;

    /**
     * @brief The node type string for the given numberic TypeId.
     */
    [[nodiscard]] const char* node_type_name(TypeId) const;

    /**
     * @brief The numeric TypeId for the given node type string.
     *
     * \note There can be multiple types with the same string name. This
     * function will only return one of them.
     */
    [[nodiscard]] TypeId node_type_id(std::string_view, bool is_named) const;

    /**
     * @brief The number of distrinct field names in the langauge.
     */
    [[nodiscard]] std::uint32_t field_count() const;

    /**
     * @brief The field name string for the given numeric FieldId.
     */
    [[nodiscard]] const char* field_name(FieldId) const;

    /**
     * @brief The numeric FieldId for the given field name string.
     *
     * \note There can be multiple fields with the same string name. This
     * function will only return one of them.
     */
    [[nodiscard]] FieldId field_id(std::string_view) const;

    /**
     * @brief The kind of a node TypeId.
     */
    [[nodiscard]] TypeKind node_type_kind(TypeId) const;

    /**
     * @brief The Tree-Sitter ABI version for this language.
     *
     * Used to check if language was generated by a compatible version of
     * Tree-Sitter.
     *
     * See: @ref TREE_SITTER_VERSION, @ref TREE_SITTER_MIN_VERSION
     */
    [[nodiscard]] std::uint32_t version() const;
};

/**
 * @brief Check if a language is compatible with the linked Tree-Sitter version.
 */
bool language_compatible(const Language&);

/**
 * @brief Lua language.
 */
const Language LUA_LANGUAGE = Language(tree_sitter_lua());

// TODO document them
// TODO we should really generate this automatically
// TODO how to deal with unnamed (e.g. operators "+", etc. are still useful)
const TypeId NODE_BREAK_STATEMENT = LUA_LANGUAGE.node_type_id("break_statement", true);
const TypeId NODE_SPREAD = LUA_LANGUAGE.node_type_id("spread", true);
const TypeId NODE_SELF = LUA_LANGUAGE.node_type_id("self", true);
const TypeId NODE_NUMBER = LUA_LANGUAGE.node_type_id("number", true);
const TypeId NODE_NIL = LUA_LANGUAGE.node_type_id("nil", true);
const TypeId NODE_TRUE = LUA_LANGUAGE.node_type_id("true", true);
const TypeId NODE_FALSE = LUA_LANGUAGE.node_type_id("false", true);
const TypeId NODE_IDENTIFIER = LUA_LANGUAGE.node_type_id("identifier", true);
const TypeId NODE_COMMENT = LUA_LANGUAGE.node_type_id("comment", true);
const TypeId NODE_STRING = LUA_LANGUAGE.node_type_id("string", true);
const TypeId NODE_PROGRAM = LUA_LANGUAGE.node_type_id("program", true);
const TypeId NODE_RETURN_STATEMENT = LUA_LANGUAGE.node_type_id("return_statement", true);
const TypeId NODE_VARIABLE_DECLARATION = LUA_LANGUAGE.node_type_id("variable_declaration", true);
const TypeId NODE_LOCAL_VARIABLE_DECLARATION =
    LUA_LANGUAGE.node_type_id("local_variable_declaration", true);
const TypeId NODE_FIELD_EXPRESSION = LUA_LANGUAGE.node_type_id("field_expression", true);
const TypeId NODE_TABLE_INDEX = LUA_LANGUAGE.node_type_id("table_index", true);
const TypeId NODE_VARIABLE_DECLARATOR = LUA_LANGUAGE.node_type_id("variable_declarator", true);
const TypeId NODE_LOCAL_VARIABLE_DECLARATOR =
    LUA_LANGUAGE.node_type_id("local_variable_declarator", true);
const TypeId NODE_DO_STATEMENT = LUA_LANGUAGE.node_type_id("do_statement", true);
const TypeId NODE_IF_STATEMENT = LUA_LANGUAGE.node_type_id("if_statement", true);
const TypeId NODE_ELSEIF = LUA_LANGUAGE.node_type_id("elseif", true);
const TypeId NODE_ELSE = LUA_LANGUAGE.node_type_id("else", true);
const TypeId NODE_WHILE_STATEMENT = LUA_LANGUAGE.node_type_id("while_statement", true);
const TypeId NODE_REPEAT_STATEMENT = LUA_LANGUAGE.node_type_id("repeat_statement", true);
const TypeId NODE_FOR_STATEMENT = LUA_LANGUAGE.node_type_id("for_statement", true);
const TypeId NODE_FOR_IN_STATEMENT = LUA_LANGUAGE.node_type_id("for_in_statement", true);
const TypeId NODE_LOOP_EXPRESSION = LUA_LANGUAGE.node_type_id("loop_expression", true);
const TypeId NODE_GOTO_STATEMENT = LUA_LANGUAGE.node_type_id("goto_statement", true);
const TypeId NODE_LABEL_STATEMENT = LUA_LANGUAGE.node_type_id("label_statement", true);
const TypeId NODE_FUNCTION = LUA_LANGUAGE.node_type_id("function", true);
const TypeId NODE_LOCAL_FUNCTION = LUA_LANGUAGE.node_type_id("local_function", true);
const TypeId NODE_FUNCTION_CALL = LUA_LANGUAGE.node_type_id("function_call", true);
const TypeId NODE_ARGUMENTS = LUA_LANGUAGE.node_type_id("ARGUMENTS", true);
const TypeId NODE_FUNCTION_NAME = LUA_LANGUAGE.node_type_id("function_name", true);
const TypeId NODE_FUNCTION_NAME_FIELD = LUA_LANGUAGE.node_type_id("function_name_field", true);
const TypeId NODE_PARAMETERS = LUA_LANGUAGE.node_type_id("parameters", true);
const TypeId NODE_FUNCTION_DEFINITION = LUA_LANGUAGE.node_type_id("function_definition", true);
const TypeId NODE_TABLE = LUA_LANGUAGE.node_type_id("table", true);
const TypeId NODE_FIELD = LUA_LANGUAGE.node_type_id("field", true);
const TypeId NODE_BINARY_OPERATION = LUA_LANGUAGE.node_type_id("binary_operation", true);
const TypeId NODE_UNARY_OPERATION = LUA_LANGUAGE.node_type_id("unary_operation", true);
const TypeId NODE_CONDITION_EXPRESSION = LUA_LANGUAGE.node_type_id("condition_expression", true);
const TypeId NODE_EXPRESSION = LUA_LANGUAGE.node_type_id("expression", true);
const TypeId NODE_METHOD = LUA_LANGUAGE.node_type_id("method", true);
const TypeId NODE_PROPERTY_IDENTIFIER = LUA_LANGUAGE.node_type_id("property_identifier", true);

const FieldId FIELD_OBJECT = LUA_LANGUAGE.field_id("object");

// forward declarations
class Cursor;
class Tree;

/**
 * @brief A syntax node in a parsed tree.
 *
 * Wrapper for a TSNode.
 *
 * Nodes can be named or anonymous (see [Named vs Anonymous
 * Nodes](https://tree-sitter.github.io/tree-sitter/using-parsers#named-vs-anonymous-nodes)).
 *
 * Nodes can't be null. If you try to create a null node the constructor will
 * throw a NullNodeException. (But this constructor should only be used
 * internally.)
 *
 * \note This object is only valid for as long as the Tree it was created
 * from. If the tree was edited, methods on the node might return wrong
 * results. In this case you should retrieve the node from the tree again.
 *
 * \note Node::type_id is called *symbol* in Tree-Sitter. We renamed it to keep
 * it in line with the type name (TypeId).
 *
 * Supports equality operators.
 *
 * Features not included (because we currently don't use them):
 *
 * - Get child by field:
 *   - `ts_node_child_by_field_name`
 *   - `ts_node_child_by_field_id`
 * - Get child/decendant for byte/point (range):
 *   - `ts_node_first_child_for_byte`
 *   - `ts_node_first_named_child_for_byte`
 *   - `ts_node_descendant_for_byte_range`
 *   - `ts_node_descendant_for_point_range`
 *   - `ts_node_named_descendant_for_byte_range`
 *   - `ts_node_named_descendant_for_point_range`
 * - Editing nodes directly (but we support this through the Tree):
 *   - `ts_node_edit`
 */
class Node {
    TSNode node;
    // not owned pointer
    const Tree* tree_;

    struct unsafe_t {};

public:
    /**
     * @brief Type tag for the unsafe constructor.
     *
     * \warning Should only be used internally.
     */
    static constexpr unsafe_t unsafe{};
    /**
     * Creates a new node from the given tree-sitter node and the tree.
     *
     * Will throw NullNodeException if the node is null.
     *
     * \warning Should only be used internally.
     */
    Node(TSNode node, const Tree& tree);

    /**
     * Unsafe constructor that does not check for null nodes.
     *
     * Only call this if you know the node is not null.
     *
     * \warning This might make other node methods segfault if the node was
     * null.
     *
     * \warning This should only be used internally.
     */
    Node(unsafe_t, TSNode node, const Tree& tree) noexcept;

    /**
     * @brief Create a Node if `node` is not null.
     */
    static std::optional<Node> or_null(TSNode node, const Tree& tree) noexcept;

    /**
     * @brief Returns the raw Tree-Sitter node.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Don't modify the return value by calling Tree-Sitter functions directly.
     */
    [[nodiscard]] TSNode raw() const;

    /**
     * @brief The Tree this node was created from.
     */
    [[nodiscard]] const Tree& tree() const;

    /**
     * @brief The substring of the source code this node represents.
     */
    [[nodiscard]] const char* type() const;

    /**
     * @brief The numeric TypeId of the node.
     *
     * In tree-sitter this is called *symbol*.
     */
    [[nodiscard]] TypeId type_id() const;

    /**
     * @brief Check if the node is named.
     */
    [[nodiscard]] bool is_named() const;

    /**
     * @brief Check if the node is *missing*.
     *
     * Missing nodes are used to recover from some kinds of syntax errors.
     */
    [[nodiscard]] bool is_missing() const;

    /**
     * @brief Check if the node is *extra*.
     *
     * Extra nodes represent things like comments.
     */
    [[nodiscard]] bool is_extra() const;

    /**
     * @brief Check if the node has been edited.
     */
    [[nodiscard]] bool has_changes() const;

    /**
     * @brief Check if the node (or any of its children) is a syntax error.
     */
    [[nodiscard]] bool has_error() const;

    /**
     * @brief The parent of the node.
     *
     * Returns `std::nullopt` when called with the root node of a tree.
     */
    [[nodiscard]] std::optional<Node> parent() const;

    /**
     * @brief The n-th child (0 indexed). Counts named and anonymous nodes.
     *
     * Returns `std::nullopt` if the child does not exist.
     */
    [[nodiscard]] std::optional<Node> child(std::uint32_t index) const;

    /**
     * @brief The number of all children (named and anonymous).
     */
    [[nodiscard]] std::uint32_t child_count() const;

    /**
     * @brief List of all children (named and anonymous).
     */
    [[nodiscard]] std::vector<Node> children() const;

    /**
     * @brief The n-th **named** child (0 indexed).
     *
     * This will not return anonymous nodes and the index only considers named
     * nodes.
     *
     * Returns `std::nullopt` if the child does not exist.
     */
    [[nodiscard]] std::optional<Node> named_child(std::uint32_t index) const;

    /**
     * @brief The number of named children.
     */
    [[nodiscard]] std::uint32_t named_child_count() const;

    /**
     * @brief List of all named children.
     */
    [[nodiscard]] std::vector<Node> named_children() const;

    /**
     * @brief The node's next sibling.
     *
     * This will also return anonymous nodes.
     *
     * Returns `std::nullopt` if there are no more siblings.
     */
    [[nodiscard]] std::optional<Node> next_sibling() const;

    /**
     * @brief The node's previous sibling.
     *
     * This will also return anonymous nodes.
     *
     * Returns `std::nullopt` if this node is already the first sibling.
     */
    [[nodiscard]] std::optional<Node> prev_sibling() const;

    /**
     * @brief The node's next *named* sibling.
     *
     * This will not return anonymous nodes.
     *
     * Returns `std::nullopt` if there are no more named siblings.
     */
    [[nodiscard]] std::optional<Node> next_named_sibling() const;

    /**
     * @brief The node's previous named sibling.
     *
     * This will not return anonymous nodes.
     *
     * Returns `std::nullopt` if this node is already the first named sibling.
     */
    [[nodiscard]] std::optional<Node> prev_named_sibling() const;

    /**
     * @brief Start position as byte offset.
     */
    [[nodiscard]] std::uint32_t start_byte() const;

    /**
     * @brief End position as byte offset.
     *
     * Returns the position **after** the last character.
     */
    [[nodiscard]] std::uint32_t end_byte() const;

    /**
     * @brief The start position as Point (row and column).
     */
    [[nodiscard]] Point start_point() const;

    /**
     * @brief The end position as Point (row and column).
     */
    [[nodiscard]] Point end_point() const;

    /**
     * @brief The start position as Location (row, column and byte offset).
     */
    [[nodiscard]] Location start() const;

    /**
     * @brief The end position as Location (row, column and byte offset).
     */
    [[nodiscard]] Location end() const;

    /**
     * @brief The Range of the node (start and end Location).
     */
    [[nodiscard]] Range range() const;

    /**
     * @brief The substring of source code this node represents.
     */
    [[nodiscard]] std::string text() const;

    /**
     * @brief A string representation of the syntax tree starting from the node
     * represented as an s-expression.
     */
    [[nodiscard]] std::string as_s_expr() const;
};

bool operator==(const Node& lhs, const Node& rhs);
bool operator!=(const Node& lhs, const Node& rhs);
std::ostream& operator<<(std::ostream&, const Node&);
std::ostream& operator<<(std::ostream&, const std::optional<Node>&);

/**
 * @brief Parser for a Tree-Sitter language.
 *
 * The default constructor uses @ref LUA_LANGUAGE.
 *
 * Features not included (because we currently don't use them):
 *
 * - Partial document parsing:
 *   - `ts_parser_set_included_ranges`
 *   - `ts_parser_included_ranges`
 * - alternative parse sources (other than utf8 string)
 *   - generalized `ts_parser_parse` (with TSInput)
 *   - `ts_parser_parse_string_encoding`
 * - parsing timeout/cancellation
 *   - `ts_parser_reset`
 *   - `ts_parser_set_timeout_micros`
 *   - `ts_parser_timeout_micros`
 *   - `ts_parser_set_cancellation_flag`
 *   - `ts_parser_cancellation_flag`
 * - Grammar debug features:
 *   - `ts_parser_set_logger`
 *   - `ts_parser_logger`
 *   - `ts_parser_print_dot_graphs`
 */
class Parser {
    std::unique_ptr<TSParser, void (*)(TSParser*)> parser;

public:
    /**
     * @brief Create a parser using the lua language.
     */
    Parser();

    /**
     * @brief Create a parser using the given language.
     */
    Parser(const Language&);

    /**
     * @brief Returns the raw Tree-Sitter parser.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Never free or otherwise delete this pointer.
     */
    [[nodiscard]] TSParser* raw() const;

    /**
     * @brief The Language the parser was created from.
     */
    [[nodiscard]] Language language() const;

    /**
     * @brief Parse a string and return its syntax tree.
     *
     * This takes the source code by copy (or move) and stores it in the tree.
     */
    Tree parse_string(std::string) const;

    /**
     * @brief Parse a string and return its syntax tree.
     *
     * This takes the source code by copy (or move) and a previously parsed tree.
     *
     * \note Only for internal use.
     */
    Tree parse_string(const TSTree* old_tree, std::string source) const;
};

/**
 * @brief Holds information about an applied Edit.
 *
 * `after` could for example be used to highlight changed locations in an editor.
 *
 * Supports equality operators.
 */
struct AppliedEdit {
    /**
     * @brief The Range in the old source code string.
     */
    Range before;
    /**
     * @brief The Range in the new source code string.
     */
    Range after;
    /**
     * @brief The string in the old source code that was replaced.
     */
    std::string old_source;
    /**
     * @brief The string that replaced the `old_source`.
     */
    std::string replacement;
};

bool operator==(const AppliedEdit&, const AppliedEdit&);
bool operator!=(const AppliedEdit&, const AppliedEdit&);
std::ostream& operator<<(std::ostream&, const AppliedEdit&);
std::ostream& operator<<(std::ostream&, const std::vector<AppliedEdit>&);

/**
 * @brief Holds information about all applied edits.
 *
 * Returned by Tree::edit.
 *
 * Supports equality operators.
 */
struct EditResult {
    /**
     * @brief The raw ranges of string that were changed.
     *
     * This does not directly correspond to the edits.
     */
    std::vector<Range> changed_ranges;
    /**
     * @brief The adjusted and applied edits.
     *
     * Holds information about the actually applied edits, including adjusted
     * locations.
     */
    std::vector<AppliedEdit> applied_edits;
};

bool operator==(const EditResult&, const EditResult&);
bool operator!=(const EditResult&, const EditResult&);
std::ostream& operator<<(std::ostream&, const EditResult&);

/**
 * @brief A syntax tree.
 *
 * This also contains a copy of the source code to allow the nodes to refer to
 * the text they were created from.
 */
class Tree {
    std::unique_ptr<TSTree, void (*)(TSTree*)> tree;
    // maybe a separate Input type is better to be more flexible
    std::string source_;

    // not owned pointer
    const Parser* parser_;

public:
    /**
     * @brief Create a new tree from the raw Tree-Sitter tree.
     *
     * \warning Should only be used internally. This class handles the TSTree
     * pointer as a reference. Calling this twice with the same pointer will
     * lead to double-frees.
     */
    explicit Tree(TSTree* tree, std::string source, const Parser& parser);

    /**
     * @brief Copy constructor.
     *
     * `TSTree*` can be safely (and fast) copied using `ts_tree_copy`.
     */
    Tree(const Tree&);
    /**
     * @brief Copy assignment operator.
     */
    Tree& operator=(const Tree&);

    /**
     * @brief Move constructor.
     */
    Tree(Tree&& other) noexcept = default;
    /**
     * @brief Move assignment operator.
     */
    Tree& operator=(Tree&& other) noexcept = default;
    /**
     * @brief Swap function.
     */
    friend void swap(Tree& self, Tree& other) noexcept;

    ~Tree() = default;

    /**
     * @brief Returns the raw Tree-Sitter tree.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSTree* raw() const;

    /**
     * @brief The source code the tree was created from.
     */
    [[nodiscard]] const std::string& source() const;

    /**
     * @brief The used parser.
     */
    [[nodiscard]] const Parser& parser() const;

    /**
     * @brief The root node of the tree.
     *
     * The returned node is only valid as long as this tree is not destructed.
     */
    [[nodiscard]] Node root_node() const;

    /**
     * @brief The language that was used to parse the syntax tree.
     */
    [[nodiscard]] Language language() const;

    /**
     * @brief Edit the syntax tree and source code and return the changed ranges.
     *
     * You need to specify all edits you want to apply to the syntax tree in
     * one call. Because this method changes both the syntax tree and source
     * code string any other [Edit](@ref Edit)s will be invalid and trying to
     * apply them is undefined behaviour.
     *
     * The edits can't be duplicate or overlapping. Multiline edits are also
     * currently not supported.
     *
     * The returned result contains information about the raw string ranges
     * that changed and it also contains the adjusted location of the edits that
     * can e.g. be used for highlighting in an editor.
     *
     * Any previously retrieved nodes will become (silently) invalid.
     *
     * \note This takes the edits by value because they should not be used after
     * calling this function and we need to modify the vector internally.
     */
    EditResult edit(std::vector<Edit>);

    /**
     * @brief Print a dot graph to the given file.
     *
     * `file` has to be a null-terminated string (e.g. from a std::string).
     */
    void print_dot_graph(std::string_view file) const;
};

EditResult edit_tree(std::vector<Edit> edits, Tree& tree, TSTree* old_tree);

/**
 * @brief Allows efficient walking of a Tree.
 *
 * This is more efficient than using the methods on Node because we don't create
 * a new Node after every navigation step.
 *
 * Features not included (because we currently don't use them):
 *
 * - `ts_tree_cursor_goto_first_child_for_byte`
 */
class Cursor {
    // TSTreeCursor internally allocates heap.
    // It can be copied with "ts_tree_cursor_copy"
    TSTreeCursor cursor;
    const Tree* tree;

public:
    /**
     * @brief Create a cursor starting at the given node.
     */
    explicit Cursor(Node) noexcept;

    /**
     * @brief Create a cursor starting at the root node of the given tree.
     */
    explicit Cursor(const Tree&) noexcept;
    ~Cursor() noexcept;

    /**
     * Copy constructor.
     */
    Cursor(const Cursor&) noexcept;
    /**
     * Copy assignment operator.
     */
    Cursor& operator=(const Cursor&) noexcept;

    /**
     * Move constructor.
     */
    Cursor(Cursor&&) = default;
    /**
     * Move assignment operator.
     */
    Cursor& operator=(Cursor&&) = default;

    /**
     * Swap function.
     */
    friend void swap(Cursor&, Cursor&) noexcept;

    /**
     * @brief Reset the cursor to the given node.
     */
    void reset(Node);

    /**
     * @brief Reset the cursor to the root node of the given tree.
     */
    void reset(const Tree&);

    /**
     * @brief The node the cursor is currently pointing at.
     */
    [[nodiscard]] Node current_node() const;

    /**
     * @brief The field name of the node the cursor is currently pointing at.
     */
    [[nodiscard]] const char* current_field_name() const;

    /**
     * @brief The FieldId of the node the cursor is currently pointing at.
     */
    [[nodiscard]] FieldId current_field_id() const;

    /**
     * @brief Move the cursor to the parent of the current node.
     *
     * Returns only false if the cursor is already at the root node.
     */
    bool goto_parent();

    /**
     * @brief Move the cursor to the next sibling of the current node.
     *
     * Returns false if there was no next sibling.
     */
    bool goto_next_sibling();

    /**
     * @brief Similar to calling Cursor::goto_next_sibling n times.
     *
     * Returns the number of siblings skipped.
     */
    int skip_n_siblings(int n);

    /**
     * @brief Move the cursor to the first child of the current node.
     *
     * Returns `false` if there were no children.
     */
    bool goto_first_child();

    /**
     * @brief Move the cursor to the next named sibling of the current node.
     *
     * Returns `false` if there was no next sibling.
     *
     * \note This method might move the cursor to another unnamed node and then
     * still return false if there is no named node.
     */
    bool goto_next_named_sibling();

    /**
     * @brief Move the cursor to the next named sibling of the current node.
     *
     * Returns `false` if there was no next sibling.
     *
     * NOTE: This method might move the cursor to another unnamed node and then
     * still return false if there is no named node.
     */
    bool goto_first_named_child();

    /**
     * @brief Skips over nodes while the given callback returns `true`.
     *
     * The method returns `false` if there were no more siblings to skip while
     * the callback still returned `true`.
     */
    template <typename Fn> bool skip_siblings_while(Fn fn) {
        if (!this->goto_next_sibling()) {
            return false;
        }
        while (fn(this->current_node())) {
            if (!this->goto_next_sibling()) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Calls the provided callback for every sibling and moves the cursor.
     *
     * The callback will also be called on the current node. So it will always
     * be called at least once.
     */
    template <typename Fn> void foreach_remaining_siblings(Fn fn) {
        do {
            fn(this->current_node());
        } while (this->goto_next_sibling());
    }

    /**
     * @brief List of all child nodes of the current node.
     *
     * This will also move the cursor to the last child but you can
     * Cursor::reset the cursor to point at any of the returned children or
     * call 'parent' to get back to the current node.
     */
    std::vector<Node> children();

    /**
     * @brief List of all named child nodes of the current node.
     *
     * This will also move the cursor to the last child but you can
     * Cursor::reset the cursor to point at any of the returned children or
     * call Cursor::parent to get back to the current node.
     */
    std::vector<Node> named_children();
};

/**
 * Visits all children of the cursor and call the given function.
 */
void visit_children(Cursor& cursor, const std::function<void(ts::Node)>& fn);

/**
 * Visit all siblings of the cursor and call the given function.
 */
void visit_siblings(Cursor& cursor, const std::function<void(ts::Node)>& fn);

/**
 * Visits a tree using a cursor.
 */
template <typename Fn> static void visit_tree(const ts::Tree& tree, Fn fn) {
    Cursor cursor(tree);

    fn(cursor.current_node());
    visit_children(cursor, fn);
}

/**
 * @brief A query is a "pre-compiled" string of S-expression patterns.
 *
 * Features not included (because we currently don't use them):
 *
 * - Predicates (evaluation is not handled by tree-sitter anyway):
 *   - `ts_query_predicates_for_pattern`
 *   - `ts_query_step_is_definite`
 *
 *   Can't be copied because the underlying `TSQuery` can't be copied.
 */
class Query {
    std::unique_ptr<TSQuery, void (*)(TSQuery*)> query;

public:
    /**
     * @brief Create query from the given query string.
     *
     * \note The string_view does not need to be null terminated.
     */
    Query(std::string_view);

    // no copying because TSQuery can't be copied
    Query(const Query&) = delete;
    Query& operator=(const Query&) = delete;

    /**
     * @brief Move constructor.
     */
    Query(Query&&) noexcept = default;
    /**
     * @brief Move assignment operator.
     */
    Query& operator=(Query&&) noexcept = default;
    /**
     * @brief Swap function.
     */
    friend void swap(Query& self, Query& other) noexcept;

    ~Query() = default;

    /**
     * @brief Returns the raw Tree-Sitter query.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSQuery* raw() const;

    /**
     * @brief Returns the raw Tree-Sitter query.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Never free or otherwise delete this pointer.
     */
    [[nodiscard]] TSQuery* raw();

    /**
     * @brief The number of patterns in the query.
     */
    [[nodiscard]] std::uint32_t pattern_count() const;

    /**
     * @brief The number of captures in the query.
     */
    [[nodiscard]] std::uint32_t capture_count() const;

    /**
     * @brief The number of string literals in the query.
     */
    [[nodiscard]] std::uint32_t string_count() const;

    /**
     * @brief The byte offset where the pattern starts in the query source.
     *
     * Can be useful when combining queries.
     */
    [[nodiscard]] std::uint32_t start_byte_for_pattern(std::uint32_t) const;

    /**
     * @brief The name of one of the query's captures.
     *
     * Each capture is associated with a numeric id based on the order
     * that it appeared in the query's source.
     *
     * \note The returned `string_view` is only valid as long the query is alive.
     */
    [[nodiscard]] std::string_view capture_name_for_id(std::uint32_t id) const;

    /**
     * @brief Get one of the query's string literals.
     *
     * Each string literal is associated with a numeric id based on the order
     * that it appeared in the query's source.
     *
     * \note The returned `string_view` is only valid as long the query is.
     */
    [[nodiscard]] std::string_view string_value_for_id(std::uint32_t id) const;

    /**
     * @brief Disable a capture within a query.
     *
     * This prevents the capture from being returned in matches and avoid
     * ressource usage.
     *
     * \warning This can not be undone.
     */
    void disable_capture(std::string_view);

    /**
     * @brief Disable a pattern within a query.
     *
     * This prevents the pattern from matching and removes most of the overhead.
     *
     * \warning This can not be undone.
     */
    void disable_pattern(std::uint32_t id);
};

/**
 * @brief A capture of a node in a syntax tree.
 *
 * Created by applying a query.
 */
struct Capture {
    /**
     * @brief The captured node.
     */
    Node node;
    /**
     * @brief The index of the capture in the match.
     */
    std::uint32_t index;

    Capture(TSQueryCapture, const Tree&) noexcept;
};

std::ostream& operator<<(std::ostream&, const Capture&);
std::ostream& operator<<(std::ostream& os, const std::vector<Capture>&);

/**
 * @brief A match of a pattern in a syntax tree.
 */
struct Match {
    uint32_t id;
    /**
     * @brief The index of the pattern in the query.
     */
    uint16_t pattern_index;
    /**
     * @brief The captures of the match.
     */
    std::vector<Capture> captures;

    Match(TSQueryMatch, const Tree&) noexcept;

    /**
     * @brief The first capture with the given index, if any.
     *
     * \note This does a linear search for a capture with the given index.
     */
    [[nodiscard]] std::optional<Capture> capture_with_index(std::uint32_t index) const;
};

std::ostream& operator<<(std::ostream&, const Match&);
std::ostream& operator<<(std::ostream& os, const std::vector<Match>&);

/**
 * @brief Stores the state needed to execute a query and iteratively search for
 * matches.
 *
 * You first have to call QueryCursor::exec with the Query and then you can
 * retrieve matches with the other functions.
 *
 * You can iterate over the result matches by calling QueryCursor::next_match.
 * This is only useful if you provided multiple patterns.
 *
 * You can also iterate over the captures if you don't care which patterns
 * matched.
 *
 * At any point you can call QueryCursor::exec again and start using the cursor
 * with another query.
 *
 * Can't be copied because the underlying `TSQueryCursor` can't be copied.
 *
 * Features not included (because we currently don't use them):
 *
 * - setting byte/point range to search in:
 *   - `ts_query_cursor_set_byte_range`
 *   - `ts_query_cursor_set_point_range`
 */
class QueryCursor {
    std::unique_ptr<TSQueryCursor, void (*)(TSQueryCursor*)> cursor;
    const Tree* tree;

public:
    /**
     * @brief Create a QueryCursor for a Tree.
     */
    explicit QueryCursor(const Tree&) noexcept;

    // can't copy because TSQueryCursor can't be copied
    QueryCursor(const QueryCursor&) = delete;
    QueryCursor& operator=(const QueryCursor&) = delete;

    /**
     * @brief Move constructor.
     */
    QueryCursor(QueryCursor&&) = default;
    /**
     * @brief Move assignment operator.
     */
    QueryCursor& operator=(QueryCursor&&) = default;

    ~QueryCursor() = default;

    /**
     * @brief Returns the raw Tree-Sitter query.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Never free or otherwise delete this pointer.
     */
    [[nodiscard]] const TSQueryCursor* raw() const;

    /**
     * @brief Returns the raw Tree-Sitter query.
     *
     * \warning
     * Use with care. Only intended vor internal use in the wrapper types.
     *
     * \warning
     * Never free or otherwise delete this pointer.
     */
    [[nodiscard]] TSQueryCursor* raw();

    /**
     * @brief Start running a given query on a given node.
     */
    void exec(const Query&, Node);

    /**
     * @brief Start running a given query on the root of the tree.
     */
    void exec(const Query&);

    /**
     * @brief Advance to the next match of the currently running query if
     * possible.
     */
    std::optional<Match> next_match();

    /**
     * @brief Advance to the next capture of the currently running query if
     * possible.
     */
    std::optional<Capture> next_capture();

    /**
     * @brief Get all matches.
     *
     * This needs to internally advance over the matches so you can only call
     * this once. Subsequent calls will return an empty vector.
     *
     * This will also omit matches that were already retrieved by calling
     * QueryCursor::next_match.
     */
    std::vector<Match> matches();
};

/**
 * @brief Prints a debug representation of the tree starting at the node.
 *
 * See debug_print_node.
 */
std::string debug_print_tree(Node node);

/**
 * @brief Prints a debug representation of the node (does not print children).
 *
 * This is easier to read than Node::as_s_expr and contains more information.
 *
 * Additional node properties are indicated by a symbol after the node name:
 *
 * - has_changes: *
 * - has_errors: E
 * - is_names: N
 * - is_missing: ?
 * - is_extra: +
 */
std::string debug_print_node(Node node);

} // namespace ts

#endif

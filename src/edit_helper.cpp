#include "tree_sitter/tree_sitter.hpp"
#include <algorithm>

namespace ts {
// helper functions for Tree::edit
namespace {
// helper function to apply one edit to the tree and source code
static AppliedEdit _apply_edit(const Edit& edit, TSTree* tree, std::string& source) {
    const long old_size = edit.range.end.byte - edit.range.start.byte;

    const std::string old_source = source.substr(edit.range.start.byte, old_size);

    source.replace(edit.range.start.byte, old_size, edit.replacement);

    const long end_byte_diff = static_cast<long>(edit.replacement.size()) - old_size;

    const Range before = edit.range;
    const auto column = static_cast<std::uint32_t>(before.end.point.column + end_byte_diff);
    const auto byte = static_cast<std::uint32_t>(before.end.byte + end_byte_diff);
    const Range after{
        .start = edit.range.start,
        .end = {
            .point = {.row = edit.range.end.point.row, .column = column},
            .byte = byte,
        }};

    const TSInputEdit input_edit{
        .start_byte = before.start.byte,
        .old_end_byte = before.end.byte,
        .new_end_byte = after.end.byte,
        .start_point =
            TSPoint{
                .row = before.start.point.row,
                .column = before.start.point.column,
            },
        .old_end_point =
            TSPoint{
                .row = before.end.point.row,
                .column = before.end.point.column,
            },
        .new_end_point =
            TSPoint{
                .row = after.end.point.row,
                .column = after.end.point.column,
            },
    };

    ts_tree_edit(tree, &input_edit);

    return AppliedEdit{
        .before = before,
        .after = after,
        .old_source = old_source,
        .replacement = edit.replacement,
    };
}

static inline Point _point(const TSPoint& point) {
    return Point{
        .row = point.row,
        .column = point.column,
    };
}

static inline Location _location(const TSPoint& point, const std::uint32_t byte) {
    return Location{
        .point = _point(point),
        .byte = byte,
    };
}

static inline Range _range(const TSRange& range) {
    return Range{
        .start = _location(range.start_point, range.start_byte),
        .end = _location(range.end_point, range.end_byte),
    };
}

static std::vector<Range> _get_changed_ranges(const TSTree* old_tree, const TSTree* new_tree) {
    // this will always be set by ts_tree_get_changed_ranges
    // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
    std::uint32_t length;

    const std::unique_ptr<TSRange, decltype(&free)> ranges{
        ts_tree_get_changed_ranges(old_tree, new_tree, &length), free};

    if (length == 0) {
        return {};
    }

    const TSRange* begin = ranges.get();
    // we have no other choise because we get the point + length from a c api
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const TSRange* end = begin + static_cast<std::size_t>(length);

    std::vector<Range> changed_ranges{length};

    std::transform(
        begin, end, changed_ranges.begin(), [](const TSRange& range) { return _range(range); });

    return changed_ranges;
}

static inline void _forbid_zero_sized_edit(const Edit& edit) {
    if (edit.range.start == edit.range.end) {
        throw ZeroSizedEditException();
    }
}
static inline void _forbid_multiline_edit(const Edit& edit) {
    if (edit.range.start.point.row != edit.range.end.point.row ||
        edit.replacement.find('\n') != std::string::npos) {
        throw MultilineEditException();
    }
}

static inline void _check_edits(const std::vector<Edit>& edits) {
    // NOTE: assumes that the ranges are already sorted by edit.range.start.byte

    for (int i = 0; i < edits.size(); ++i) {
        const auto& edit1 = edits[i];

        _forbid_zero_sized_edit(edit1);
        _forbid_multiline_edit(edit1);

        // forbid overlapping edits
        for (int j = 0; j < edits.size(); ++j) {
            if (i == j) {
                continue;
            }

            const auto& edit2 = edits[j];

            if (edit1.range.overlaps(edit2.range)) {
                throw OverlappingEditException();
            }
        }
    }
}

struct Adjustment {
    Point last_point;
    int last_width_change;
    int cumulative_byte_change;
};
static inline void _adjust_edit(Edit& edit, const Adjustment& adjustment) {
    // if the last edit was in the current line we need to adjust the
    // location of the next edit
    if (adjustment.last_width_change != 0 &&
        adjustment.last_point.row == edit.range.start.point.row) {
        edit.range.start.point.column += adjustment.last_width_change;
        edit.range.end.point.column += adjustment.last_width_change;
    }

    edit.range.start.byte += adjustment.cumulative_byte_change;
    edit.range.end.byte += adjustment.cumulative_byte_change;
}
static inline void _update_adjustment(
    Adjustment& adjustment, const AppliedEdit& applied_edit, const Point last_point) {
    const int before_width =
        applied_edit.before.end.point.column - applied_edit.before.start.point.column;
    const int after_width =
        applied_edit.after.end.point.column - applied_edit.after.start.point.column;
    adjustment.last_width_change = after_width - before_width;

    const int byte_change = applied_edit.after.end.byte - applied_edit.before.end.byte;
    adjustment.cumulative_byte_change += byte_change;
    adjustment.last_point = last_point;
}

static inline std::vector<AppliedEdit>
_apply_all_edits(std::vector<Edit>& edits, std::string& new_source, TSTree* old_tree) {
    std::vector<AppliedEdit> applied_edits;
    applied_edits.reserve(edits.size());

    Adjustment adjustment{};

    for (auto& edit : edits) {
        const Range range_before_adjustments = edit.range;
        _adjust_edit(edit, adjustment);

        AppliedEdit applied_edit = _apply_edit(edit, old_tree, new_source);
        applied_edit.before = range_before_adjustments;
        applied_edits.push_back(applied_edit);

        _update_adjustment(adjustment, applied_edit, edit.range.end.point);
    }

    return applied_edits;
}
} // namespace

EditResult edit_tree(std::vector<Edit> edits, Tree& tree, TSTree* old_tree) {
    std::string new_source = tree.source();

    // sorts the edits from the earliest in the source code to the latest in the source code.
    // this is done so the locations for edits in the same line can be adjusted
    // so we can return the ranges of the edit before and after
    std::sort(edits.begin(), edits.end(), [](const Edit& edit1, const Edit& edit2) {
        return edit1.range.start.byte <= edit2.range.start.byte;
    });

    // NOTE: this throws exceptions if there is something wrong with the edits
    _check_edits(edits);

    std::vector<AppliedEdit> applied_edits = _apply_all_edits(edits, new_source, old_tree);

    // reparse the source code
    Tree new_tree = tree.parser().parse_string(old_tree, std::move(new_source));

    std::vector<Range> changed_ranges = _get_changed_ranges(old_tree, new_tree.raw());

    // update this tree
    swap(tree, new_tree);

    return EditResult{
        .changed_ranges = changed_ranges,
        .applied_edits = applied_edits,
    };
}

} // namespace ts

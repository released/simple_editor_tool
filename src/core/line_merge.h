#pragma once
#include "text_codec.h"
#include "compare_rules.h"
#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

namespace simple_editor::core {
struct TextLine { std::wstring text, ending; };
struct LineOptions { bool ignore_space = false, ignore_case = false, ignore_eol = false; TextCompareRules rules; };
enum class MergeChoice { Unresolved, Base, Left, Right, Both };
struct MergeBlock {
    std::size_t base_begin = 0, base_end = 0, left_begin = 0, left_end = 0, right_begin = 0, right_end = 0;
    bool changed = false, important = true, conflict = false;
    MergeChoice choice = MergeChoice::Left;
};
struct TextMergeModel {
    std::vector<TextLine> base, left, right;
    std::vector<MergeBlock> blocks;
    bool three_way = false;
};
struct TextMergeRow { int base = -1, left = -1, right = -1; std::size_t block = 0; };
std::vector<TextLine> SplitLines(const std::wstring& text);
std::wstring TextForEditControl(const std::wstring& text);
std::wstring TextFromEditControl(const std::wstring& original, const std::wstring& displayed);
bool BuildTextMerge(const std::wstring& base, const std::wstring& left, const std::wstring& right,
                    bool three_way, const LineOptions& options, const std::atomic_bool& cancel,
                    TextMergeModel* model, std::wstring* error);
std::wstring RenderMerge(const TextMergeModel& model, std::size_t* unresolved = nullptr);
std::vector<TextMergeRow> MergeRows(const TextMergeModel& model);
} // namespace simple_editor::core

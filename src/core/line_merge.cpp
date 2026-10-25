#include "line_merge.h"
#include <algorithm>
#include <cwctype>
#include <functional>
#include <unordered_map>
#include <Windows.h>

namespace simple_editor::core {
namespace {
std::wstring ConvertNewlines(const std::wstring& text, const std::wstring& ending) {
    std::wstring result; result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == L'\r' || text[i] == L'\n') { if (text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n') ++i; result += ending; }
        else result += text[i];
    }
    return result;
}
bool WithinLineLimit(const std::wstring& text) {
    std::size_t lines = 0;
    for (std::size_t start = 0; start < text.size();) {
        if (++lines > 200000) return false;
        const auto end = text.find_first_of(L"\r\n",start); if (end == std::wstring::npos) break;
        start = end + (text[end] == L'\r' && end + 1 < text.size() && text[end + 1] == L'\n' ? 2 : 1);
    }
    return true;
}
}
std::vector<TextLine> SplitLines(const std::wstring& text) {
    std::vector<TextLine> lines;
    for (std::size_t start = 0; start < text.size();) {
        const auto end = text.find_first_of(L"\r\n", start);
        if (end == std::wstring::npos) { lines.push_back({text.substr(start), L""}); break; }
        const auto length = text[end] == L'\r' && end + 1 < text.size() && text[end + 1] == L'\n' ? 2u : 1u;
        lines.push_back({text.substr(start, end - start), text.substr(end, length)}); start = end + length;
    }
    return lines;
}
std::wstring TextForEditControl(const std::wstring& text) {
    return ConvertNewlines(text,L"\r\n");
}
namespace {
std::wstring Key(const TextLine& line, const LineOptions& options) {
    if (IgnoredLine(line.text,options.rules)) return L"";
    std::wstring key;
    std::wstring masked; const auto* content = &line.text; if (options.rules.mask_timestamps || !options.rules.mask_literal.empty()) { masked = MaskComparedText(line.text,options.rules); content = &masked; }
    for (wchar_t c : *content) {
        if (options.ignore_space && (c == L' ' || c == L'\t')) continue;
        key += c;
    }
    if (options.ignore_case && !key.empty()) {
        const int count = LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_UPPERCASE,key.data(),static_cast<int>(key.size()),nullptr,0,nullptr,nullptr,0);
        if (count > 0) { std::wstring mapped(count,L'\0'); LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_UPPERCASE,key.data(),static_cast<int>(key.size()),mapped.data(),count,nullptr,nullptr,0); key = std::move(mapped); }
    }
    if (!options.ignore_eol) { key += L'\0'; key += line.ending; }
    return key;
}
bool Exact(const TextLine& a, const TextLine& b) { return a.text == b.text && a.ending == b.ending; }
bool Equal(const std::vector<TextLine>& a, std::size_t begin, std::size_t end,
           const std::vector<TextLine>& b, std::size_t other_begin, std::size_t other_end) {
    if (end - begin != other_end - other_begin) return false;
    for (; begin < end; ++begin, ++other_begin) if (!Exact(a[begin], b[other_begin])) return false;
    return true;
}
struct Pair { int a, b; };
struct Alignment {
    const std::vector<std::wstring>& a; const std::vector<std::wstring>& b;
    const std::atomic_bool& cancel; std::vector<Pair> rows;
    void Gap(std::size_t ab, std::size_t ae, std::size_t bb, std::size_t be, unsigned depth = 0) {
        if (cancel) return;
        while (ab < ae && bb < be && a[ab] == b[bb]) rows.push_back({static_cast<int>(ab++), static_cast<int>(bb++)});
        std::size_t suffix = 0;
        while (ab < ae && bb < be && a[ae - 1] == b[be - 1]) { --ae; --be; ++suffix; }
        const auto n = ae - ab, m = be - bb;
        if (!n || !m) {
            for (std::size_t i = ab; i < ae; ++i) rows.push_back({static_cast<int>(i), -1});
            for (std::size_t j = bb; j < be; ++j) rows.push_back({-1, static_cast<int>(j)});
        } else if (n <= 2048 && m <= 2048 && n * m <= 2000000) {
            std::vector<unsigned int> table((n + 1) * (m + 1));
            for (std::size_t i = n; i-- > 0;) {
                if (cancel) return;
                for (std::size_t j = m; j-- > 0;) table[i * (m + 1) + j] = a[ab + i] == b[bb + j]
                    ? table[(i + 1) * (m + 1) + j + 1] + 1
                    : (std::max)(table[(i + 1) * (m + 1) + j], table[i * (m + 1) + j + 1]);
            }
            std::size_t i = 0, j = 0;
            while (i < n || j < m) {
                if (i < n && j < m && a[ab + i] == b[bb + j]) rows.push_back({static_cast<int>(ab + i++), static_cast<int>(bb + j++)});
                else if (i < n && (j == m || table[(i + 1) * (m + 1) + j] >= table[i * (m + 1) + j + 1])) rows.push_back({static_cast<int>(ab + i++), -1});
                else rows.push_back({-1, static_cast<int>(bb + j++)});
            }
        } else {
            struct Count { std::size_t index = 0, count = 0; };
            std::unordered_map<std::wstring, Count> ac, bc;
            for (std::size_t i = ab; i < ae; ++i) { auto& entry = ac[a[i]]; entry.index = i; ++entry.count; }
            for (std::size_t i = bb; i < be; ++i) { auto& entry = bc[b[i]]; entry.index = i; ++entry.count; }
            std::vector<std::pair<std::size_t, std::size_t>> candidates;
            if (depth < 32) for (std::size_t i = ab; i < ae; ++i) {
                const auto found = bc.find(a[i]);
                if (ac[a[i]].count == 1 && found != bc.end() && found->second.count == 1) candidates.push_back({i, found->second.index});
            }
            std::vector<std::size_t> tails, previous(candidates.size(), SIZE_MAX);
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                const auto it = std::lower_bound(tails.begin(), tails.end(), candidates[i].second,
                    [&](std::size_t index, std::size_t value) { return candidates[index].second < value; });
                const auto slot = static_cast<std::size_t>(it - tails.begin());
                if (slot) previous[i] = tails[slot - 1];
                if (it == tails.end()) tails.push_back(i); else *it = i;
            }
            std::vector<std::size_t> anchors;
            if (!tails.empty()) for (auto index = tails.back(); index != SIZE_MAX; index = previous[index]) anchors.push_back(index);
            std::reverse(anchors.begin(), anchors.end());
            if (anchors.empty()) {
                // A coarse changed block is conservative and preserves every line.
                for (std::size_t i = ab; i < ae; ++i) rows.push_back({static_cast<int>(i), -1});
                for (std::size_t i = bb; i < be; ++i) rows.push_back({-1, static_cast<int>(i)});
            } else {
                auto x = ab, y = bb;
                for (auto anchor : anchors) { const auto& p = candidates[anchor]; Gap(x, p.first, y, p.second, depth + 1); rows.push_back({static_cast<int>(p.first), static_cast<int>(p.second)}); x = p.first + 1; y = p.second + 1; }
                Gap(x, ae, y, be, depth + 1);
            }
        }
        for (std::size_t i = 0; i < suffix; ++i) rows.push_back({static_cast<int>(ae + i), static_cast<int>(be + i)});
    }
};
std::vector<MergeBlock> Diff(const std::vector<TextLine>& a, const std::vector<TextLine>& b,
                             const LineOptions& options, const std::atomic_bool& cancel) {
    std::vector<std::wstring> ak, bk; ak.reserve(a.size()); bk.reserve(b.size());
    for (const auto& line : a) ak.push_back(Key(line, options));
    for (const auto& line : b) bk.push_back(Key(line, options));
    Alignment alignment{ak, bk, cancel, {}}; alignment.Gap(0, a.size(), 0, b.size());
    std::vector<MergeBlock> blocks;
    std::size_t ai = 0, bi = 0;
    for (const auto& row : alignment.rows) {
        const bool changed = row.a < 0 || row.b < 0 || !Exact(a[row.a], b[row.b]);
        if (blocks.empty() || blocks.back().changed != changed) {
            MergeBlock block; block.base_begin = block.left_begin = ai; block.right_begin = bi; block.changed = changed; block.important = false; blocks.push_back(block);
        }
        auto& block = blocks.back();
        if (row.a >= 0) ++ai; if (row.b >= 0) ++bi;
        block.base_end = block.left_end = ai; block.right_end = bi;
        block.important = block.important || row.a < 0 || row.b < 0 || ak[row.a] != bk[row.b];
    }
    if (options.rules.ignore_blank || !options.rules.ignore_prefix.empty() || !options.rules.ignore_contains.empty()) for (auto& block : blocks) if (block.changed) { std::vector<std::wstring> first,second; for (auto i = block.left_begin; i < block.left_end; ++i) if (!IgnoredLine(a[i].text,options.rules)) first.push_back(Key(a[i],options)); for (auto i = block.right_begin; i < block.right_end; ++i) if (!IgnoredLine(b[i].text,options.rules)) second.push_back(Key(b[i],options)); block.important = first != second; }
    return blocks;
}
struct Edit { std::size_t begin, end, count; bool left; };
void Append(std::wstring& output, const std::vector<TextLine>& lines, std::size_t begin, std::size_t end) {
    for (auto i = begin; i < end; ++i) { output += lines[i].text; output += lines[i].ending; }
}
}
std::wstring TextFromEditControl(const std::wstring& original, const std::wstring& displayed) {
    if (displayed == TextForEditControl(original)) return original;
    std::wstring ending = L"\r\n"; bool found = false, mixed = false;
    for (std::size_t i = 0; i < original.size(); ++i) if (original[i] == L'\r' || original[i] == L'\n') {
        const std::size_t length = original[i] == L'\r' && i + 1 < original.size() && original[i + 1] == L'\n' ? 2 : 1;
        if (!found) { ending.assign(original,i,length); found = true; } else if (original.compare(i,length,ending) != 0) mixed = true;
        i += length - 1;
    }
    if (!mixed) return ConvertNewlines(displayed,ending);
    const auto before = SplitLines(original); auto after = SplitLines(displayed);
    for (auto& line : after) if (!line.ending.empty()) line.ending = ending;
    if (mixed) {
        std::vector<std::wstring> a,b; for (const auto& line : before) a.push_back(line.text); for (const auto& line : after) b.push_back(line.text);
        const std::atomic_bool cancelled{false}; Alignment alignment{a,b,cancelled,{}}; alignment.Gap(0,a.size(),0,b.size());
        std::vector<int> removed,inserted;
        const auto replacements = [&]() { for (std::size_t i = 0; i < (std::min)(removed.size(),inserted.size()); ++i) if (!after[inserted[i]].ending.empty() && !before[removed[i]].ending.empty()) after[inserted[i]].ending = before[removed[i]].ending; removed.clear(); inserted.clear(); };
        for (const auto& row : alignment.rows) {
            if (row.a >= 0 && row.b >= 0) { replacements(); if (!after[row.b].ending.empty() && !before[row.a].ending.empty()) after[row.b].ending = before[row.a].ending; }
            else { if (row.a >= 0) removed.push_back(row.a); if (row.b >= 0) inserted.push_back(row.b); }
        }
        replacements();
    }
    std::wstring result; Append(result,after,0,after.size()); return result;
}
bool BuildTextMerge(const std::wstring& base, const std::wstring& left, const std::wstring& right,
                    bool three_way, const LineOptions& options, const std::atomic_bool& cancel,
                    TextMergeModel* model, std::wstring* error) {
    if (!WithinLineLimit(left) || !WithinLineLimit(right) || (three_way && !WithinLineLimit(base))) { if (error) *error = L"Text comparison supports up to 200,000 lines per input. Use Hex for larger files."; return false; }
    TextMergeModel result; result.three_way = three_way; result.left = SplitLines(left); result.right = SplitLines(right); result.base = SplitLines(three_way ? base : left);
    if (!three_way) result.blocks = Diff(result.left, result.right, options, cancel);
    else {
        const auto ld = Diff(result.base, result.left, options, cancel), rd = Diff(result.base, result.right, options, cancel);
        std::vector<Edit> edits;
        for (const auto& block : ld) if (block.changed) edits.push_back({block.left_begin, block.left_end, block.right_end - block.right_begin, true});
        for (const auto& block : rd) if (block.changed) edits.push_back({block.left_begin, block.left_end, block.right_end - block.right_begin, false});
        std::stable_sort(edits.begin(), edits.end(), [](const Edit& a, const Edit& b) { return a.begin != b.begin ? a.begin < b.begin : a.end < b.end; });
        std::size_t bi = 0, li = 0, ri = 0;
        for (std::size_t i = 0; i < edits.size();) {
            if (cancel) return false;
            const auto start = edits[i].begin; auto end = edits[i].end; std::size_t j = i + 1;
            while (j < edits.size()) {
                if (cancel) return false;
                // Sorted ranges form a connected interval; boundary insertions commute.
                const bool overlaps = edits[j].begin < end || (start == end && edits[j].begin == start && edits[j].end == start);
                if (!overlaps) break; end = (std::max)(end, edits[j++].end);
            }
            if (bi < start) { MergeBlock same; same.base_begin = bi; same.base_end = start; same.left_begin = li; same.left_end = li + start - bi; same.right_begin = ri; same.right_end = ri + start - bi; result.blocks.push_back(same); li = same.left_end; ri = same.right_end; }
            std::ptrdiff_t lc = static_cast<std::ptrdiff_t>(end - start), rc = lc;
            for (auto k = i; k < j; ++k) (edits[k].left ? lc : rc) += static_cast<std::ptrdiff_t>(edits[k].count) - static_cast<std::ptrdiff_t>(edits[k].end - edits[k].begin);
            if (lc < 0 || rc < 0) { if (error) *error = L"Invalid merge alignment."; return false; }
            MergeBlock block; block.changed = true; block.base_begin = start; block.base_end = end; block.left_begin = li; block.left_end = li + lc; block.right_begin = ri; block.right_end = ri + rc;
            if (block.left_end > result.left.size() || block.right_end > result.right.size()) { if (error) *error = L"Invalid merge source ranges."; return false; }
            if (Equal(result.left, li, block.left_end, result.right, ri, block.right_end)) block.choice = MergeChoice::Left;
            else if (Equal(result.base, start, end, result.left, li, block.left_end)) block.choice = MergeChoice::Right;
            else if (Equal(result.base, start, end, result.right, ri, block.right_end)) block.choice = MergeChoice::Left;
            else { block.choice = MergeChoice::Unresolved; block.conflict = true; }
            result.blocks.push_back(block); bi = end; li = block.left_end; ri = block.right_end; i = j;
        }
        if (bi < result.base.size()) { MergeBlock same; same.base_begin = bi; same.base_end = result.base.size(); same.left_begin = li; same.left_end = result.left.size(); same.right_begin = ri; same.right_end = result.right.size(); result.blocks.push_back(same); }
    }
    if (cancel) { if (error) *error = L"Text comparison cancelled."; return false; }
    *model = std::move(result); return true;
}
std::wstring RenderMerge(const TextMergeModel& model, std::size_t* unresolved) {
    std::wstring output; std::size_t count = 0;
    std::wstring ending = L"\r\n";
    for (const auto& line : model.left) if (!line.ending.empty()) { ending = line.ending; break; }
    const auto marker = [&](const wchar_t* label) { if (!output.empty() && output.back() != L'\n' && output.back() != L'\r') output += ending; output += label; output += ending; };
    for (const auto& block : model.blocks) {
        switch (block.choice) {
        case MergeChoice::Base: Append(output, model.base, block.base_begin, block.base_end); break;
        case MergeChoice::Left: Append(output, model.left, block.left_begin, block.left_end); break;
        case MergeChoice::Right: Append(output, model.right, block.right_begin, block.right_end); break;
        case MergeChoice::Both:
            Append(output, model.left, block.left_begin, block.left_end);
            if (block.left_begin < block.left_end && block.right_begin < block.right_end && model.left[block.left_end - 1].ending.empty()) output += L"\r\n";
            Append(output, model.right, block.right_begin, block.right_end); break;
        default:
            ++count; marker(L"<<<<<<< LEFT"); Append(output, model.left, block.left_begin, block.left_end);
            marker(L"||||||| BASE"); Append(output, model.base, block.base_begin, block.base_end);
            marker(L"======="); Append(output, model.right, block.right_begin, block.right_end); marker(L">>>>>>> RIGHT"); break;
        }
    }
    if (unresolved) *unresolved = count; return output;
}
std::vector<TextMergeRow> MergeRows(const TextMergeModel& model) {
    std::vector<TextMergeRow> rows;
    for (std::size_t i = 0; i < model.blocks.size(); ++i) {
        const auto& b = model.blocks[i]; const auto count = (std::max)({model.three_way ? b.base_end - b.base_begin : 0, b.left_end - b.left_begin, b.right_end - b.right_begin});
        for (std::size_t j = 0; j < count; ++j) rows.push_back({model.three_way && j < b.base_end - b.base_begin ? static_cast<int>(b.base_begin + j) : -1, j < b.left_end - b.left_begin ? static_cast<int>(b.left_begin + j) : -1, j < b.right_end - b.right_begin ? static_cast<int>(b.right_begin + j) : -1, i});
    }
    return rows;
}
} // namespace simple_editor::core

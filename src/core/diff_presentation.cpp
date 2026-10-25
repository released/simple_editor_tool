#include "diff_presentation.h"
#include <algorithm>
#include <unordered_map>
#include <set>
#include <cwctype>
namespace simple_editor::core {
namespace {
struct Atom { std::uint32_t value; std::size_t begin, length; };
std::vector<Atom> Atoms(const std::wstring& text,const LineOptions& options) {
    std::vector<Atom> result;
    for (std::size_t i = 0; i < text.size();) { const auto begin = i; auto value = static_cast<std::uint32_t>(text[i++]); if (value >= 0xD800 && value <= 0xDBFF && i < text.size() && text[i] >= 0xDC00 && text[i] <= 0xDFFF) value = 0x10000 + (value - 0xD800) * 1024 + text[i++] - 0xDC00;
        if (options.ignore_space && value <= 0xFFFF && iswspace(static_cast<wchar_t>(value))) continue;
        if (options.ignore_case) { wchar_t folded[4]{}; const int size = LCMapStringEx(LOCALE_NAME_INVARIANT,LCMAP_LOWERCASE,text.data() + begin,static_cast<int>(i - begin),folded,4,nullptr,nullptr,0); if (size == 1) value = folded[0]; else if (size == 2 && folded[0] >= 0xD800 && folded[0] <= 0xDBFF && folded[1] >= 0xDC00 && folded[1] <= 0xDFFF) value = 0x10000 + (folded[0] - 0xD800) * 1024 + folded[1] - 0xDC00; }
        result.push_back({value,begin,i - begin});
    }
    return result;
}
void AddSpan(std::vector<InlineSpan>& spans,const Atom& atom) { if (!spans.empty() && spans.back().begin + spans.back().length == atom.begin) spans.back().length += atom.length; else spans.push_back({atom.begin,atom.length}); }
std::wstring Key(const std::wstring& text,const LineOptions& options) { std::wstring result; for (const auto& atom : Atoms(text,options)) { if (atom.value <= 0xFFFF) result.push_back(static_cast<wchar_t>(atom.value)); else { const auto value = atom.value - 0x10000; result.push_back(static_cast<wchar_t>(0xD800 + (value >> 10))); result.push_back(static_cast<wchar_t>(0xDC00 + (value & 1023))); } } return result; }
int Similarity(const std::wstring& a,const std::wstring& b) {
    if (a == b) return 100; if (a.empty() || b.empty()) return 0;
    const auto limit = (std::min)(a.size(),b.size()); std::size_t prefix = 0, suffix = 0; while (prefix < limit && a[prefix] == b[prefix]) ++prefix; while (suffix < limit - prefix && a[a.size() - suffix - 1] == b[b.size() - suffix - 1]) ++suffix;
    std::set<std::wstring> left_words, right_words; const auto words = [](const std::wstring& text,std::set<std::wstring>& output) { for (std::size_t i = 0; i < text.size();) { if (!(iswalnum(text[i]) || text[i] == L'_')) { ++i; continue; } const auto begin = i++; while (i < text.size() && (iswalnum(text[i]) || text[i] == L'_')) ++i; output.insert(text.substr(begin,i - begin)); } }; words(a,left_words); words(b,right_words); std::size_t common = 0; for (const auto& word : left_words) if (right_words.count(word)) ++common;
    const int edge = static_cast<int>(200 * (prefix + suffix) / (a.size() + b.size())), token = static_cast<int>(200 * common / (std::max<std::size_t>)(1,left_words.size() + right_words.size())); return (std::max)(edge,token);
}
}
InlineDifference CompareLineCharacters(const std::wstring& left,const std::wstring& right,const LineOptions& options,std::uint64_t* budget) {
    const auto a = Atoms(left,options), b = Atoms(right,options); std::size_t prefix = 0, suffix = 0;
    while (prefix < a.size() && prefix < b.size() && a[prefix].value == b[prefix].value) ++prefix;
    while (suffix < a.size() - prefix && suffix < b.size() - prefix && a[a.size() - suffix - 1].value == b[b.size() - suffix - 1].value) ++suffix;
    const auto n = a.size() - prefix - suffix, m = b.size() - prefix - suffix; InlineDifference result;
    const std::uint64_t cells = static_cast<std::uint64_t>(n + 1) * (m + 1);
    if (n && m && cells <= 262144 && (!budget || cells <= *budget)) {
        if (budget) *budget -= cells; const auto columns = m + 1; std::vector<std::uint16_t> table(static_cast<std::size_t>(cells));
        for (std::size_t i = n; i-- > 0;) for (std::size_t j = m; j-- > 0;) table[i * columns + j] = a[prefix + i].value == b[prefix + j].value ? static_cast<std::uint16_t>(1 + table[(i + 1) * columns + j + 1]) : (std::max)(table[(i + 1) * columns + j],table[i * columns + j + 1]);
        std::size_t i = 0,j = 0; while (i < n || j < m) { if (i < n && j < m && a[prefix + i].value == b[prefix + j].value) { ++i; ++j; } else if (j == m || (i < n && table[(i + 1) * columns + j] >= table[i * columns + j + 1])) AddSpan(result.left,a[prefix + i++]); else AddSpan(result.right,b[prefix + j++]); }
    } else { for (std::size_t i = prefix; i < a.size() - suffix; ++i) AddSpan(result.left,a[i]); for (std::size_t i = prefix; i < b.size() - suffix; ++i) AddSpan(result.right,b[i]); }
    return result;
}
bool BuildDiffPresentation(const TextMergeModel& model,const LineOptions& options,bool similar_lines,const std::atomic_bool& cancel,DiffPresentation* presentation) {
    DiffPresentation result; std::uint64_t similarity_budget = 16 * 1024 * 1024;
    for (std::size_t index = 0; index < model.blocks.size(); ++index) { if (cancel) return false; const auto& block = model.blocks[index];
        const auto append = [&](std::size_t ab,std::size_t ae,std::size_t bb,std::size_t be) { const auto count = (std::max)(ae - ab,be - bb); for (std::size_t i = 0; i < count; ++i) result.rows.push_back({model.three_way && block.base_begin + i < block.base_end ? static_cast<int>(block.base_begin + i) : -1,i < ae - ab ? static_cast<int>(ab + i) : -1,i < be - bb ? static_cast<int>(bb + i) : -1,index}); };
        const auto n = block.left_end - block.left_begin, m = block.right_end - block.right_begin;
        if (model.three_way) { const auto count = (std::max)({block.base_end - block.base_begin,n,m}); for (std::size_t i = 0; i < count; ++i) result.rows.push_back({i < block.base_end - block.base_begin ? static_cast<int>(block.base_begin + i) : -1,i < n ? static_cast<int>(block.left_begin + i) : -1,i < m ? static_cast<int>(block.right_begin + i) : -1,index}); continue; }
        if (!similar_lines || !block.changed || n > 128 || m > 128 || n < 2 || m < 2) { append(block.left_begin,block.left_end,block.right_begin,block.right_end); continue; }
        std::vector<std::wstring> keys_a,keys_b; for (auto i = block.left_begin; i < block.left_end; ++i) keys_a.push_back(Key(model.left[i].text.substr(0,8192),options)); for (auto i = block.right_begin; i < block.right_end; ++i) keys_b.push_back(Key(model.right[i].text.substr(0,8192),options));
        std::uint64_t characters = 0; for (const auto& key : keys_a) characters += key.size() * m; for (const auto& key : keys_b) characters += key.size() * n;
        if (characters > similarity_budget) { append(block.left_begin,block.left_end,block.right_begin,block.right_end); continue; } similarity_budget -= characters;
        std::vector<int> scores(n * m), table((n + 1) * (m + 1)); const auto columns = m + 1;
        for (std::size_t i = n; i-- > 0;) { if (cancel) return false; for (std::size_t j = m; j-- > 0;) { scores[i * m + j] = Similarity(keys_a[i],keys_b[j]); const auto take = scores[i * m + j] >= 35 ? 100 + scores[i * m + j] + table[(i + 1) * columns + j + 1] : 0; table[i * columns + j] = (std::max)({take,table[(i + 1) * columns + j],table[i * columns + j + 1]}); } }
        std::size_t i = 0,j = 0,ai = 0,bj = 0; while (i < n && j < m) { const auto score = scores[i * m + j]; if (score >= 35 && table[i * columns + j] == 100 + score + table[(i + 1) * columns + j + 1]) { append(block.left_begin + ai,block.left_begin + i,block.right_begin + bj,block.right_begin + j); result.rows.push_back({-1,static_cast<int>(block.left_begin + i),static_cast<int>(block.right_begin + j),index}); ai = ++i; bj = ++j; } else if (table[(i + 1) * columns + j] >= table[i * columns + j + 1]) ++i; else ++j; }
        append(block.left_begin + ai,block.left_end,block.right_begin + bj,block.right_end);
    }
    result.characters.resize(result.rows.size()); result.hints.resize(result.rows.size()); std::uint64_t budget = 32 * 1024 * 1024;
    struct Movement { std::size_t left = SIZE_MAX,right = SIZE_MAX; unsigned left_count = 0,right_count = 0; }; std::unordered_map<std::wstring,Movement> moved;
    for (std::size_t i = 0; i < result.rows.size(); ++i) { if (cancel) return false; const auto& row = result.rows[i]; if (!model.blocks[row.block].changed) continue;
        result.characters[i] = CompareLineCharacters(row.left >= 0 ? model.left[row.left].text.substr(0,8192) : L"",row.right >= 0 ? model.right[row.right].text.substr(0,8192) : L"",options,&budget);
        if (row.left >= 0 && row.right < 0 && model.left[row.left].text.size() <= 8192) { const auto key = Key(model.left[row.left].text,options); if (key.size() >= 3) { auto& entry = moved[key]; entry.left = i; ++entry.left_count; } }
        if (row.right >= 0 && row.left < 0 && model.right[row.right].text.size() <= 8192) { const auto key = Key(model.right[row.right].text,options); if (key.size() >= 3) { auto& entry = moved[key]; entry.right = i; ++entry.right_count; } }
    }
    for (const auto& [key,entry] : moved) if (entry.left_count == 1 && entry.right_count == 1) { const auto label = L"Moved L" + std::to_wstring(result.rows[entry.left].left + 1) + L" -> R" + std::to_wstring(result.rows[entry.right].right + 1); result.hints[entry.left] = result.hints[entry.right] = label; }
    *presentation = std::move(result); return true;
}
}

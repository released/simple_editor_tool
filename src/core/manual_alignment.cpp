#include "manual_alignment.h"
#include <sstream>
#include <algorithm>
namespace simple_editor::core {
bool ParseLineAnchors(const std::wstring& text,LineAnchors* output,std::wstring* error) {
    if (text.size() > 4096) return false; LineAnchors anchors; for (const auto& line : SplitLines(text)) { if (line.text.empty() || line.text[0] == L'#') continue; auto value = line.text; std::replace(value.begin(),value.end(),L',',L' '); std::wistringstream input(value); std::size_t a = 0,b = 0; std::wstring extra; if (!(input >> a >> b) || input >> extra || !a || !b || a > 200000 || b > 200000 || anchors.size() >= 128 || (!anchors.empty() && (a - 1 <= anchors.back().first || b - 1 <= anchors.back().second))) { if (error) *error = L"Use left-line,right-line with line numbers starting at 1, increasing on both sides (128 anchors maximum)."; return false; } anchors.emplace_back(a - 1,b - 1); } *output = std::move(anchors); return true;
}
bool BuildAnchoredComparison(const std::wstring& left,const std::wstring& right,const LineOptions& options,const LineAnchors& anchors,const std::atomic_bool& cancel,TextMergeModel* output,std::wstring* error) {
    TextMergeModel result; result.left = SplitLines(left); result.right = SplitLines(right); result.base = result.left; if (result.left.size() > 200000 || result.right.size() > 200000 || anchors.size() > 128) return false;
    std::size_t a = 0,b = 0; const auto segment = [&](std::size_t ae,std::size_t be) { std::wstring x,y; for (auto i = a; i < ae; ++i) { x += result.left[i].text; x += result.left[i].ending; } for (auto i = b; i < be; ++i) { y += result.right[i].text; y += result.right[i].ending; } TextMergeModel part; if (!BuildTextMerge(L"",x,y,false,options,cancel,&part,error)) return false; for (auto block : part.blocks) { block.base_begin += a; block.base_end += a; block.left_begin += a; block.left_end += a; block.right_begin += b; block.right_end += b; result.blocks.push_back(block); } return true; };
    for (const auto& anchor : anchors) { if (cancel || anchor.first < a || anchor.second < b || anchor.first >= result.left.size() || anchor.second >= result.right.size()) { if (error) *error = L"An alignment anchor is outside the current input or crosses another anchor."; return false; } if (!segment(anchor.first,anchor.second)) return false; a = anchor.first; b = anchor.second; if (!segment(a + 1,b + 1)) return false; ++a; ++b; }
    if (!segment(result.left.size(),result.right.size()) || cancel) return false; *output = std::move(result); return true;
}
}

#include "compare_region.h"
#include "search.h"
#include <algorithm>
#include <sstream>
#include <cwctype>
namespace simple_editor::core {
namespace {
std::wstring Trim(std::wstring text) { const auto a = text.find_first_not_of(L" \t\r\n"), b = text.find_last_not_of(L" \t\r\n"); return a == std::wstring::npos ? L"" : text.substr(a,b - a + 1); }
bool Ignored(const std::vector<AddressSpan>& spans,std::uint64_t address) { if (address == kMissingOffset) return false; const auto found = std::upper_bound(spans.begin(),spans.end(),address,[](std::uint64_t at,const AddressSpan& range) { return at < range.begin; }); return found != spans.begin() && address - (found - 1)->begin < (found - 1)->length; }
bool Normalize(std::vector<AddressSpan>* spans,std::uint64_t size) { std::sort(spans->begin(),spans->end(),[](const auto& a,const auto& b) { return a.begin < b.begin; }); std::vector<AddressSpan> result; for (const auto& span : *spans) { if (span.begin > size || span.length > size - span.begin) return false; if (!span.length) continue; if (!result.empty() && span.begin <= result.back().begin + result.back().length) result.back().length = (std::max)(result.back().length,span.begin + span.length - result.back().begin); else result.push_back(span); } *spans = std::move(result); return true; }
}
bool ParseCompareRegionSpec(const std::wstring& text,CompareRegion* region,std::wstring* error) {
    if (text.size() > 8192) return false; CompareRegion result; std::wistringstream sections(text); std::wstring first; if (!std::getline(sections,first,L';')) return false; std::wistringstream fields(first); std::wstring values[4];
    for (auto& value : values) { if (!std::getline(fields,value,L',')) { if (error) *error = L"Use left-start,left-length,right-start,right-length; lengths may be all."; return false; } value = Trim(value); }
    std::wstring extra; if (std::getline(fields,extra,L',')) return false;
    if (!ParseAddress(values[0],&result.left_begin) || !ParseAddress(values[2],&result.right_begin)) return false;
    const auto length = [](std::wstring value,std::uint64_t* parsed) { for (auto& ch : value) ch = static_cast<wchar_t>(towlower(ch)); if (value == L"all") { *parsed = UINT64_MAX; return true; } return ParseAddress(value,parsed); };
    if (!length(values[1],&result.left_length) || !length(values[3],&result.right_length)) return false;
    bool seen_left = false, seen_right = false;
    while (std::getline(sections,extra,L';')) { extra = Trim(extra); if (extra.empty()) continue; if (extra.size() < 2 || extra[1] != L':' || (extra[0] != L'L' && extra[0] != L'l' && extra[0] != L'R' && extra[0] != L'r')) return false; const bool left = extra[0] == L'L' || extra[0] == L'l'; auto& seen = left ? seen_left : seen_right; if (seen) return false; seen = true; auto& ranges = left ? result.ignore_left : result.ignore_right; std::wistringstream tokens(extra.substr(2)); std::wstring token;
        while (std::getline(tokens,token,L',')) { token = Trim(token); if (token.empty()) continue; const auto plus = token.find(L'+'); AddressSpan span; if (plus == std::wstring::npos || !ParseAddress(token.substr(0,plus),&span.begin) || !ParseAddress(token.substr(plus + 1),&span.length) || span.length > UINT64_MAX - span.begin || ranges.size() >= 64) { if (error) *error = L"Ignored spans use absolute offset+length pairs, at most 64 per side."; return false; } ranges.push_back(span); }
    }
    *region = std::move(result); return true;
}
bool CompareDocumentRegions(const BinaryDocument& left,const BinaryDocument& right,bool insertion_aware,const CompareRegion& region,const std::atomic_bool& cancelled,const BatchCallback& callback,const ProgressCallback& progress,std::wstring* error) {
    if (!region.left_begin && !region.right_begin && region.left_length == UINT64_MAX && region.right_length == UINT64_MAX && region.ignore_left.empty() && region.ignore_right.empty()) return CompareDocuments(left,right,insertion_aware,cancelled,callback,progress,error);
    auto ignored_left = region.ignore_left, ignored_right = region.ignore_right;
    if (region.left_begin > left.Size() || region.right_begin > right.Size() || !Normalize(&ignored_left,left.Size()) || !Normalize(&ignored_right,right.Size())) { if (error) *error = L"Comparison ranges/ignored spans exceed their input files."; return false; }
    const auto a = region.left_length == UINT64_MAX ? left.Size() - region.left_begin : region.left_length, b = region.right_length == UINT64_MAX ? right.Size() - region.right_begin : region.right_length;
    auto ls = left.SnapshotRange(region.left_begin,a,error), rs = right.SnapshotRange(region.right_begin,b,error); if (!ls || !rs) return false;
    return CompareDocuments(*ls,*rs,insertion_aware,cancelled,[&](const DifferenceBatch& source) { DifferenceBatch result; result.reserve(source.size()); for (auto difference : source) { if (difference.left_offset != kMissingOffset) difference.left_offset += region.left_begin; if (difference.right_offset != kMissingOffset) difference.right_offset += region.right_begin; if (difference.left_anchor != kMissingOffset) difference.left_anchor += region.left_begin; if (difference.right_anchor != kMissingOffset) difference.right_anchor += region.right_begin; if (!Ignored(ignored_left,difference.left_offset == kMissingOffset ? difference.left_anchor : difference.left_offset) && !Ignored(ignored_right,difference.right_offset == kMissingOffset ? difference.right_anchor : difference.right_offset)) result.push_back(difference); } return result.empty() || callback(result); },progress,error);
}
}

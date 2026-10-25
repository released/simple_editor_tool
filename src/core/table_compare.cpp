#include "table_compare.h"
#include "decimal_compare.h"
#include "line_merge.h"
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
namespace simple_editor::core {
namespace {
std::wstring Preview(const std::wstring* value) { if (!value) return L"<missing>"; auto text = value->substr(0,512); for (auto& c : text) if (c < 32) c = L' '; return text + (value->size() > 512 ? L"..." : L""); }
void Difference(StructuredComparison* r,const std::wstring& path,const std::wstring& a,const std::wstring& b,const wchar_t* state) { ++r->different; if (r->rows.size() < 20000) r->rows.push_back({path.substr(0,2048),a,b,state}); else r->truncated = true; }
}
bool ParseTableOptions(const std::wstring& text,TableCompareOptions* output,std::wstring* error) {
    if (text.size() > 65536) return false; TableCompareOptions options; std::set<unsigned> left,right;
    for (const auto& line : SplitLines(text)) { if (line.text.empty() || line.text[0] == L'#') continue; if (line.text == L"header=yes") { options.header = true; continue; } if (line.text == L"header=no") { options.header = false; continue; }
        auto normalized = line.text; std::replace(normalized.begin(),normalized.end(),L',',L' '); std::wistringstream input(normalized); TableColumn col; std::wstring mode,extra; if (!(input >> col.left >> col.right >> mode) || (input >> extra) || !col.left || !col.right || col.left > 200000 || col.right > 200000 || options.columns.size() >= 64 || !left.insert(col.left).second || !right.insert(col.right).second) { if (error) *error = L"Use left-column,right-column,key/text/numeric-tolerance. Numbers start at 1; use each column once (64 mappings maximum)."; return false; }
        --col.left; --col.right; col.key = mode == L"key"; if (!col.key && mode != L"text") { bool valid = false; if (!DecimalWithin(L"0",L"0",mode,&valid)) { if (error) *error = L"Use a nonnegative decimal tolerance (256 characters maximum; exponent +/-4096)."; return false; } col.tolerance = mode; } options.columns.push_back(std::move(col)); }
    *output = std::move(options); return true;
}
bool CompareMappedTable(const std::wstring& left,const std::wstring& right,StructuredFormat format,const TableCompareOptions& options,const std::atomic_bool& cancel,StructuredComparison* output,std::wstring* error) {
    std::vector<std::vector<std::wstring>> a,b; if (!ReadDelimitedTable(left,format == StructuredFormat::Csv ? L',' : L'\t',cancel,&a,error) || !ReadDelimitedTable(right,format == StructuredFormat::Csv ? L',' : L'\t',cancel,&b,error)) return false; StructuredComparison result;
    const auto compare = [&](const std::vector<std::wstring>* x,const std::vector<std::wstring>* y,const std::wstring& label) { auto maps = options.columns; if (maps.empty()) for (unsigned col = 0; col < (std::max)(x ? x->size() : 0,y ? y->size() : 0); ++col) maps.push_back({col,col,false,{}});
        for (const auto& col : maps) { ++result.compared; const auto* first = x && col.left < x->size() ? &(*x)[col.left] : nullptr; const auto* second = y && col.right < y->size() ? &(*y)[col.right] : nullptr; bool equal = first && second && *first == *second,valid = true; if (first && second && !col.tolerance.empty()) valid = DecimalWithin(*first,*second,col.tolerance,&equal); if (!first || !second || !equal || !valid) Difference(&result,label + L", L" + std::to_wstring(col.left + 1) + L"/R" + std::to_wstring(col.right + 1),Preview(first),Preview(second),!first ? L"Added" : !second ? L"Removed" : !valid ? L"Invalid numeric value" : L"Changed"); }
    };
    const bool keyed = std::any_of(options.columns.begin(),options.columns.end(),[](const auto& col) { return col.key; });
    if (!keyed) { for (std::size_t row = options.header ? 1 : 0; row < (std::max)(a.size(),b.size()); ++row) { if (cancel) return false; compare(row < a.size() ? &a[row] : nullptr,row < b.size() ? &b[row] : nullptr,L"Row " + std::to_wstring(row + 1)); } }
    else {
        struct Rows { std::vector<std::size_t> left,right; std::wstring label; }; std::map<std::wstring,Rows> keys;
        const auto collect = [&](const std::vector<std::vector<std::wstring>>& rows,bool first) { for (std::size_t i = options.header ? 1 : 0; i < rows.size() && !cancel; ++i) { std::wstring key,display; bool missing = false; for (const auto& col : options.columns) if (col.key) { const auto index = first ? col.left : col.right; if (index >= rows[i].size()) { missing = true; break; } const auto& value = rows[i][index]; key += std::to_wstring(value.size()) + L":" + value; if (display.size() < 256) display += (display.empty() ? L"" : L", ") + Preview(&value); }
            if (missing) { Difference(&result,L"Row " + std::to_wstring(i + 1),first ? L"Missing key" : L"",first ? L"" : L"Missing key",L"Ambiguous key"); continue; } auto& entry = keys[key]; entry.label = display.substr(0,512); (first ? entry.left : entry.right).push_back(i); }
        }; collect(a,true); collect(b,false);
        for (const auto& pair : keys) { if (cancel) return false; const auto& entry = pair.second; const auto label = L"Key [" + entry.label + L"]"; if (entry.left.size() > 1 || entry.right.size() > 1) { Difference(&result,label,std::to_wstring(entry.left.size()) + L" rows",std::to_wstring(entry.right.size()) + L" rows",L"Ambiguous duplicate key"); continue; } compare(entry.left.empty() ? nullptr : &a[entry.left[0]],entry.right.empty() ? nullptr : &b[entry.right[0]],label); }
    }
    if (cancel) { if (error) *error = L"Table comparison cancelled."; return false; } *output = std::move(result); return true;
}
}

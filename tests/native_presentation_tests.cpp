#include "../src/core/diff_presentation.h"
#include <iostream>
#include <filesystem>
#include <set>
using namespace simple_editor::core;
int RunPresentationTests(const std::filesystem::path&) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; auto spans = CompareLineCharacters(L"甲乙丙",L"甲丁丙",{});
    check(spans.left.size() == 1 && spans.right.size() == 1 && spans.left[0].begin == 1 && spans.left[0].length == 1,L"CJK intraline difference highlights only the changed character");
    spans = CompareLineCharacters(L"a\U0001F680z",L"a\U0001F600z",{}); check(spans.left.size() == 1 && spans.left[0].begin == 1 && spans.left[0].length == 2 && spans.right[0].length == 2,L"intraline spans never split a supplementary Unicode surrogate pair");
    spans = CompareLineCharacters(L"ab",L"a中b",{}); check(spans.left.empty() && spans.right.size() == 1 && spans.right[0].begin == 1 && spans.right[0].length == 1,L"inserted characters are distinguished from deletions");
    spans = CompareLineCharacters(L"abcXYZdef123ghi",L"abcUVWdef456ghi",{}); check(spans.left.size() == 2 && spans.right.size() == 2 && spans.left[0].begin == 3 && spans.left[1].begin == 9,L"bounded LCS retains unchanged words between separate character changes");
    spans = CompareLineCharacters(L"Ä A \t中",L"ä a中",{true,true,false}); check(spans.left.empty() && spans.right.empty(),L"intraline comparison applies invariant Unicode case and whitespace rules");
    std::uint64_t budget = 0; spans = CompareLineCharacters(L"prefix" + std::wstring(2000,L'x') + L"suffix",L"prefix" + std::wstring(2000,L'y') + L"suffix",{},&budget); check(spans.left.size() == 1 && spans.left[0].begin == 6 && spans.left[0].length == 2000,L"bounded fallback preserves equal prefixes/suffixes without allocating a large matrix");
    std::atomic_bool cancelled{false}; TextMergeModel model; std::wstring error; DiffPresentation presentation;
    BuildTextMerge(L"",L"alpha\nmove-this-line\nbeta\ngamma\n",L"alpha\nbeta\ngamma\nmove-this-line\n",false,{},cancelled,&model,&error); check(BuildDiffPresentation(model,{},false,cancelled,&presentation),L"build a line presentation from the unchanged merge model"); unsigned hints = 0; for (const auto& hint : presentation.hints) if (hint == L"Moved L2 -> R4") ++hints; check(hints == 2,L"unique moved lines show their original left/right line numbers");
    const std::wstring a = L"name=one\ncount=10\nstate=old\n", b = L"unrelated prefix\nname=two\ncount=11\nstate=new\n"; BuildTextMerge(L"",a,b,false,{},cancelled,&model,&error); BuildDiffPresentation(model,{},true,cancelled,&presentation);
    check(presentation.rows.size() == 4 && presentation.rows[0].left == -1 && presentation.rows[1].left == 0 && presentation.rows[1].right == 1,L"optional similar-line alignment retains an inserted line before matching changed lines");
    std::set<int> left,right; for (const auto& row : presentation.rows) { if (row.left >= 0) left.insert(row.left); if (row.right >= 0) right.insert(row.right); } check(left.size() == 3 && right.size() == 4 && RenderMerge(model) == a,L"presentation alignment preserves all source indices and merge output");
    cancelled = true; const auto count = presentation.rows.size(); check(!BuildDiffPresentation(model,{},true,cancelled,&presentation) && presentation.rows.size() == count,L"cancelled presentation planning retains the previous rows"); cancelled = false;
    BuildTextMerge(L"base\n",L"left\n",L"right\n",true,{},cancelled,&model,&error); check(BuildDiffPresentation(model,{},true,cancelled,&presentation) && presentation.rows.size() == MergeRows(model).size() && presentation.rows[0].base == 0,L"three-way presentation preserves the base alignment and original line number");
    return failures;
}

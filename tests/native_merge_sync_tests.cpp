#include "../src/core/line_merge.h"
#include "../src/core/folder_sync.h"
#include "../src/core/workspace_session.h"
#include <fstream>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <random>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path, const std::string& value) { std::filesystem::create_directories(path.parent_path()); std::ofstream stream(path, std::ios::binary); stream << value; }
std::string Read(const std::filesystem::path& path) { std::ifstream stream(path, std::ios::binary); return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()}; }
}
int RunMergeSyncTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool condition, const wchar_t* label) { std::wcout << (condition ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!condition) ++failures; };
    std::atomic_bool cancel{false}; std::wstring error; TextMergeModel model; std::size_t conflicts = 0;
    {
        const std::wstring left = L"start\n左\nkeep\ninsert\nend", right = L"start\n右\nkeep\nend\nextra\n";
        TextMergeModel copy; std::wstring output = L"sentinel"; std::vector<std::size_t> blocks;
        check(BuildTextMerge(L"",left,right,false,{},cancel,&copy,&error),L"build directional copy fixture");
        for (std::size_t i = 0; i < copy.blocks.size(); ++i) if (copy.blocks[i].changed) blocks.push_back(i);
        check(CopyTextBlocks(copy,true,false,blocks,cancel,&output,&error) && output == left,L"all selected blocks copy left to right with insertion/deletion/EOF and Unicode");
        check(CopyTextBlocks(copy,false,false,blocks,cancel,&output,&error) && output == right,L"selected blocks copy right to left with exact source endings");
        check(CopyTextBlocks(copy,true,true,{},cancel,&output,&error) && output == left,L"whole left document copy");
        check(CopyTextBlocks(copy,false,true,{},cancel,&output,&error) && output == right,L"whole right document copy");
        check(CopyTextBlocks(copy,true,false,{blocks.front()},cancel,&output,&error) && output.find(L"左") != std::wstring::npos && output.find(L"extra") != std::wstring::npos,L"one block leaves unrelated target differences intact");
        output = L"sentinel"; cancel = true;
        check(!CopyTextBlocks(copy,true,true,{},cancel,&output,&error) && output == L"sentinel",L"cancelled copy preserves caller output"); cancel = false;
        check(!CopyTextBlocks(copy,true,false,{SIZE_MAX},cancel,&output,&error) && output == L"sentinel",L"invalid copy index preserves output");
        check(!CopyTextBlocks(copy,true,false,{blocks.front(),blocks.front()},cancel,&output,&error),L"duplicate copy blocks are rejected");
        copy.three_way = true; check(!CopyTextBlocks(copy,true,true,{},cancel,&output,&error),L"side copy cannot rewrite a three-way merge");
        check(BuildTextMerge(L"",L"",L"only\r\n",false,{},cancel,&copy,&error) && CopyTextBlocks(copy,true,true,{},cancel,&output,&error) && output.empty(),L"copy an empty document to clear a target");
        error.clear();
        check(BuildTextMerge(L"",L"top\nleft\nsame\nleft-end\n",L"top\r\nright\r\nsame\r\nright-end\r\n",false,{false,false,true},cancel,&copy,&error) &&
            std::count_if(copy.blocks.begin(),copy.blocks.end(),[](const auto& b) { return b.changed && b.important; }) == 2,L"ignored EOL rows separate two independent meaningful copy/navigation blocks");
    }
    check(TextForEditControl(L"a\nb\rc\r\n") == L"a\r\nb\r\nc\r\n",L"native edit display presents all source newline styles");
    check(TextFromEditControl(L"a\nb\r\nc\r",L"a\r\nb\r\nc\r\n") == L"a\nb\r\nc\r",L"unchanged display preserves exact mixed source newlines");
    check(TextFromEditControl(L"a\nb\r\nc\r",L"a\r\nB\r\nc\r\n") == L"a\nB\r\nc\r",L"manual edit preserves surrounding mixed line endings");
    check(TextFromEditControl(L"a\nb\n",L"a\r\nx\r\nb\r\n") == L"a\nx\nb\n",L"new edited lines follow the source newline style");
    check(BuildTextMerge(L"", L"a\r\nb\n", L"a\r\nx\nb\n", false, {}, cancel, &model, &error) && RenderMerge(model) == L"a\r\nb\n", L"two-way default preserves exact left endings");
    for (auto& block : model.blocks) block.choice = MergeChoice::Right;
    check(RenderMerge(model) == L"a\r\nx\nb\n", L"right choices reconstruct exact right source");
    check(BuildTextMerge(L"one\ntwo\nthree\n", L"ONE\ntwo\nthree\n", L"one\ntwo\nTHREE\n", true, {}, cancel, &model, &error) && RenderMerge(model, &conflicts) == L"ONE\ntwo\nTHREE\n" && conflicts == 0, L"independent three-way edits merge automatically");
    check(BuildTextMerge(L"a\nb\n", L"a\nLEFT\n", L"a\nRIGHT\n", true, {}, cancel, &model, &error) && RenderMerge(model, &conflicts).find(L"<<<<<<< LEFT") != std::wstring::npos && conflicts == 1, L"overlapping edits produce explicit conflicts");
    for (auto& block : model.blocks) if (block.conflict) block.choice = MergeChoice::Right;
    check(RenderMerge(model, &conflicts) == L"a\nRIGHT\n" && conflicts == 0, L"side choice resolves a conflict");
    check(BuildTextMerge(L"a\nb\n", L"insert\na\nb\n", L"A\nb\n", true, {}, cancel, &model, &error) && RenderMerge(model) == L"insert\nA\nb\n", L"boundary insertion commutes with adjacent replacement");
    check(BuildTextMerge(L"", L"x", L"y", true, {}, cancel, &model, &error) && (RenderMerge(model, &conflicts), conflicts == 1), L"different insertions in empty base conflict");
    check(BuildTextMerge(L"a\n", L"", L"A\n", true, {}, cancel, &model, &error) && (RenderMerge(model, &conflicts), conflicts == 1), L"delete versus modify conflicts");
    check(BuildTextMerge(L"a\n", L" a \n", L"A\n", true, {true,true,true}, cancel, &model, &error) && (RenderMerge(model, &conflicts), conflicts == 1), L"ignore rules do not discard real merge changes");
    std::mt19937 random(8931); bool roundtrips = true;
    for (int iteration = 0; iteration < 300; ++iteration) {
        std::wstring a, b;
        for (unsigned j = 0, count = random() % 24; j < count; ++j) a += std::wstring(1, static_cast<wchar_t>(L'a' + random() % 5)) + (random() % 2 ? L"\n" : L"\r\n");
        for (unsigned j = 0, count = random() % 24; j < count; ++j) b += std::wstring(1, static_cast<wchar_t>(L'a' + random() % 5)) + (random() % 2 ? L"\n" : L"\r\n");
        if (!BuildTextMerge(L"", a, b, false, {true,false,true}, cancel, &model, &error) || RenderMerge(model) != a) { roundtrips = false; break; }
        for (auto& block : model.blocks) block.choice = MergeChoice::Right;
        if (RenderMerge(model) != b || !BuildTextMerge(a, b, a, true, {}, cancel, &model, &error) || RenderMerge(model, &conflicts) != b || conflicts) { roundtrips = false; break; }
    }
    check(roundtrips, L"random repeated-line and newline round trips preserve both sources");
    bool triples = true;
    const auto random_text = [&]() { std::wstring value; for (unsigned i = 0, count = random() % 20; i < count; ++i) value += std::wstring(1,static_cast<wchar_t>(L'a' + random() % 4)) + (random() % 3 ? L"\n" : L"\r\n"); if (!value.empty() && random() % 2) value.pop_back(); return value; };
    for (int iteration = 0; iteration < 2000 && triples; ++iteration) {
        const auto base = random_text(), left_text = random_text(), right_text = random_text();
        if (!BuildTextMerge(base,left_text,right_text,true,{},cancel,&model,&error)) { triples = false; break; }
        for (auto& block : model.blocks) block.choice = MergeChoice::Base; triples = RenderMerge(model) == base;
        for (auto& block : model.blocks) block.choice = MergeChoice::Left; triples = triples && RenderMerge(model) == left_text;
        for (auto& block : model.blocks) block.choice = MergeChoice::Right; triples = triples && RenderMerge(model) == right_text;
    }
    check(triples,L"2000 random three-way alignments reconstruct every complete source exactly");
    BuildTextMerge(L"",L"left",L"right",true,{},cancel,&model,&error); for (auto& block : model.blocks) block.choice = MergeChoice::Both;
    check(RenderMerge(model) == L"left\r\nright",L"take-both separates final lines without existing newlines");
    BuildTextMerge(L"",L"\u00c4\n",L"\u00e4\n",false,{false,true,false},cancel,&model,&error);
    check(model.blocks.size() == 1 && !model.blocks.front().important,L"ignore-case handles Unicode without changing source text");
    std::wstring large_left,large_right;
    for (int i = 0; i < 5000; ++i) { large_left += std::to_wstring(i) + L"\n"; large_right += (i == 0 || i == 2500 || i == 4999 ? L"changed" : std::to_wstring(i)) + L"\n"; }
    check(BuildTextMerge(L"",large_left,large_right,false,{},cancel,&model,&error) && RenderMerge(model) == large_left,L"large line comparison preserves source outside bounded LCS");
    for (auto& block : model.blocks) block.choice = MergeChoice::Right;
    check(RenderMerge(model) == large_right,L"large line comparison reconstructs the other side");
    large_left.clear(); large_right.clear(); for (int i = 0; i < 4000; ++i) { large_left += L"a\n"; large_right += L"b\n"; }
    check(BuildTextMerge(L"",large_left,large_right,false,{},cancel,&model,&error) && RenderMerge(model) == large_left,L"large repeated-line fallback preserves all lines");
    for (auto& block : model.blocks) block.choice = MergeChoice::Right;
    check(RenderMerge(model) == large_right,L"coarse changed block retains the entire opposite source");
    std::wstring chained_base,chained_left,chained_right;
    for (int i = 0; i < 12000; ++i) { const auto line = std::to_wstring(i) + L"\n"; chained_base += line; chained_left += (i % 4 == 3 ? L"" : L"left ") + line; chained_right += (i % 4 == 1 ? L"" : L"right ") + line; }
    check(BuildTextMerge(chained_base,chained_left,chained_right,true,{},cancel,&model,&error) && model.blocks.size() == 1 && model.blocks[0].conflict,L"large interleaved edits form one conflict using a linear range sweep");
    for (auto& block : model.blocks) block.choice = MergeChoice::Left;
    check(RenderMerge(model) == chained_left,L"interleaved conflict retains every source line");
    const auto retained_blocks = model.blocks.size();
    check(!BuildTextMerge(L"",std::wstring(1000000,L'\n'),L"",false,{},cancel,&model,&error) && model.blocks.size() == retained_blocks,L"line cap rejects newline-heavy input before allocating per-line models");
    cancel = true; check(!BuildTextMerge(L"", L"a", L"b", false, {}, cancel, &model, &error), L"cancelled diff applies no model"); cancel = false;
    const auto left = root / L"sync-left", right = root / L"sync-right";
    Write(left / L"same.txt", "same"); Write(right / L"same.txt", "same"); Write(left / L"nested" / L"new.bin", "new"); Write(left / L"changed.txt", "left"); Write(right / L"changed.txt", "right"); Write(right / L"extra.txt", "extra");
    FolderScan scan; error.clear(); check(ScanFolders(left, right, {}, cancel, {}, &scan, &error), L"recursive scan compares actual file contents");
    std::vector<std::size_t> all(scan.rows.size()); std::iota(all.begin(), all.end(), 0); SyncPlan plan;
    check(BuildSyncPlan(scan, all, true, false, &plan, &error), L"build update preview plan"); auto result = ApplySyncPlan(plan, cancel, {});
    check(result.error.empty() && Read(right / L"nested" / L"new.bin") == "new" && Read(right / L"changed.txt") == "left" && Read(right / L"extra.txt") == "extra", L"update copies nested files while preserving extras");
    check(Read(result.backup / L"changed.txt") == "right", L"overwrites retain recoverable backups");
    check(ScanFolders(left, right, {}, cancel, {}, &scan, &error), L"rescan excludes recovery backup directories"); all.resize(scan.rows.size()); std::iota(all.begin(), all.end(), 0);
    check(BuildSyncPlan(scan, all, true, true, &plan, &error), L"mirror has an explicit separate plan"); result = ApplySyncPlan(plan, cancel, {});
    check(result.error.empty() && !std::filesystem::exists(right / L"extra.txt") && Read(result.backup / L"extra.txt") == "extra", L"mirror moves extras into recovery rather than deleting data");
    Write(left / L"changed.txt", "newer"); ScanFolders(left, right, {}, cancel, {}, &scan, &error); all.resize(scan.rows.size()); std::iota(all.begin(), all.end(), 0); BuildSyncPlan(scan, all, true, false, &plan, &error);
    Write(right / L"changed.txt", "external modification"); result = ApplySyncPlan(plan, cancel, {});
    check(!result.error.empty() && result.completed == 0 && Read(right / L"changed.txt") == "external modification", L"stale plans are rejected before copying");
    ScanFolders(left, right, {}, cancel, {}, &scan, &error); all.resize(scan.rows.size()); std::iota(all.begin(), all.end(), 0); BuildSyncPlan(scan, all, true, false, &plan, &error);
    cancel = true; result = ApplySyncPlan(plan, cancel, {}); check(result.cancelled && result.completed == 0, L"cancelled sync commits no file"); cancel = false;
    result = ApplySyncPlan(plan, cancel, {}, [](const auto&) { return false; }); check(result.completed == 0 && !result.error.empty(), L"unsaved editor guard rejects all operations");
    if (!plan.operations.empty()) plan.operations.front().relative = L"..\\escape.txt";
    check(ApplySyncPlan(plan, cancel, {}).completed == 0, L"sync rejects traversal paths");
    check(MatchFileFilter(L"main.CPP", L"*.h;*.cpp") && !MatchFileFilter(L"main.bin", L"*.h;*.cpp"), L"glob include/exclude filters are case insensitive");
    check(MatchFileFilter(L"\u00c4.txt",L"\u00e4.*"),L"file filters handle Unicode case");
    const auto selected_left = root / L"selected-left", selected_right = root / L"selected-right";
    Write(selected_left / L"dir" / L"child.txt","child"); Write(selected_left / L"outside.txt","outside"); std::filesystem::create_directories(selected_right);
    FolderScan selected_scan; ScanFolders(selected_left,selected_right,{},cancel,{},&selected_scan,&error);
    std::size_t directory_index = SIZE_MAX; for (std::size_t i = 0; i < selected_scan.rows.size(); ++i) if (selected_scan.rows[i].relative == L"dir") directory_index = i;
    check(BuildSyncPlan(selected_scan,{directory_index},true,false,&plan,&error) && plan.operations.size() == 2,L"selecting a directory expands its scanned descendants once");
    result = ApplySyncPlan(plan,cancel,{});
    check(result.error.empty() && Read(selected_right / L"dir" / L"child.txt") == "child" && !std::filesystem::exists(selected_right / L"outside.txt"),L"selected-directory sync preserves unselected paths");
    FolderScan nested_scan; const auto nested_root = left / L"nested";
    check(ScanFolders(left,nested_root,{},cancel,{},&nested_scan,&error) && !BuildSyncPlan(nested_scan,{},true,true,&plan,&error),L"synchronization rejects physically overlapping folder roots");
    const auto output = root / L"safe-output.txt"; error.clear(); check(WriteBytesSafely(output, {'a','b'}, &error) && Read(output) == "ab", L"merge result atomic save");
    FileStamp stamp; InspectFile(output, &stamp, &error); Write(output, "external"); check(!WriteBytesSafely(output, {'x'}, &error, {}, &stamp) && Read(output) == "external", L"saved merge output detects external changes");
    WorkspaceEntry entry; entry.comparison = true; entry.left = L"C:\\example\\\u6e2c\u8a66.cpp"; entry.right = L"C:\\right.cpp"; entry.mode = 3; entry.zoom[2] = 180; entry.options = 511; entry.include = L"*.cpp;*.h";
    const auto session = root / L"roundtrip.ini"; std::vector<WorkspaceEntry> entries;
    check(SaveWorkspaceFile(session,{entry},&error) && LoadWorkspaceFile(session,&entries,&error) && entries.size() == 1 && entries[0].left == entry.left && entries[0].mode == 3 && entries[0].zoom[2] == 180 && entries[0].options == 511,L"portable workspace preserves Unicode paths and comparison settings");
    Write(session,"[SimpleEditorToolWorkspace]\nVersion=1\nCount=1\n[Session0]\nLeft=x\nMode=999\n");
    check(!LoadWorkspaceFile(session,&entries,&error) && entries.size() == 1 && entries[0].left == entry.left,L"malformed workspace leaves existing parsed sessions intact");
    Write(session,"[SimpleEditorToolWorkspace]\nVersion=1\nVersion=1\nCount=0\n");
    check(!LoadWorkspaceFile(session,&entries,&error),L"duplicate workspace keys are rejected");
    Write(session,"[SimpleEditorToolWorkspace]\nVersion=1\nCount=+0\n");
    check(!LoadWorkspaceFile(session,&entries,&error),L"workspace counts require strict decimal digits");
    return failures;
}

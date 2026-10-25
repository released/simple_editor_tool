#include "../src/core/compare_region.h"
#include "../src/core/binary_merge.h"
#include "../src/core/binary_report.h"
#include "../src/core/workspace_session.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
namespace {
void Write(const std::filesystem::path& path,const std::string& value) { std::ofstream out(path,std::ios::binary); out.write(value.data(),value.size()); }
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
std::string Bytes(const BinaryDocument& source) { std::vector<std::uint8_t> data; source.Read(0,static_cast<std::size_t>(source.Size()),&data); return {data.begin(),data.end()}; }
}
int RunRegionTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; std::wstring error; CompareRegion region;
    check(ParseCompareRegionSpec(L"d:10,20,d:20,all;L:12+2,16+1;R:20+1",&region,&error) && region.left_begin == 10 && region.left_length == 32 && region.right_begin == 20 && region.ignore_left.size() == 2,L"range parser distinguishes decimal prefixes, hexadecimal lengths and per-side ignored spans");
    check(!ParseCompareRegionSpec(L"0,all,0,all;L:1+2;L:3+4",&region,&error) && !ParseCompareRegionSpec(L"0,all,0,all;R:FFFFFFFFFFFFFFFF+2",&region,&error),L"duplicate ignored-span sections and address overflow are rejected");
    const auto a = root / L"range-left.bin", b = root / L"range-right.bin"; const std::string first = "abcdefghijklmnop", second = "QRSTUVWX12345678", payload = first + second;
    const auto left_text = std::string(10,'L') + payload + "TAIL", right_text = std::string(20,'R') + "XY" + first + "Z" + second + "OTHER"; Write(a,left_text); Write(b,right_text); BinaryDocument left,right; left.Open(a.wstring(),&error); right.Open(b.wstring(),&error);
    auto slice = left.SnapshotRange(10,32,&error); check(slice && Bytes(*slice) == payload && left.Size() == left_text.size() && !left.IsModified(),L"piece-table range snapshot preserves its original document without copying rendered data");
    left.SetByte(11,'!'); auto draft = left.SnapshotRange(10,3,&error); check(draft && Bytes(*draft) == "a!c" && !draft->IsModified(),L"range snapshots include unsaved piece-table edits with read-only snapshot history"); std::uint64_t at = 0; left.Undo(&at);
    check(!left.SnapshotRange(left.Size(),1,&error) && left.SnapshotRange(left.Size(),0,&error)->Size() == 0,L"range snapshots reject overflow and accept an empty EOF range");
    ParseCompareRegionSpec(L"d:10,d:32,d:22,d:16",&region,&error); region.left_length = 16; std::atomic_bool cancelled{false}; std::vector<Difference> differences;
    const auto collect = [&](const DifferenceBatch& batch) { differences.insert(differences.end(),batch.begin(),batch.end()); return true; };
    check(CompareDocumentRegions(left,right,false,region,cancelled,collect,[](unsigned) {},&error) && differences.empty(),L"different start offsets compare the same selected bytes");
    ParseCompareRegionSpec(L"d:10,d:32,d:20,d:35;R:d:20+1",&region,&error);
    check(CompareDocumentRegions(left,right,true,region,cancelled,collect,[](unsigned) {},&error) && differences.size() == 2 && differences[0].right_offset == 21 && differences[0].left_anchor == 10 && differences[1].right_offset == 38 && differences[1].left_anchor == 26,L"regional comparison reports absolute addresses and retains insertion anchors across ignored data");
    std::vector<BinaryPatch> patches; SaveControl control; const auto merged = root / L"region-merged.bin";
    check(BuildBinaryPatches(differences,{0,1},true,left.Size(),right.Size(),&patches,&error) && MergeBinaryRanges(left,right,patches,merged,control,&error) && Read(merged) == std::string(10,'L') + "Y" + first + "Z" + second + "TAIL",L"selected regional differences merge correctly while an earlier insertion was ignored");
    BinaryReportStats stats; const auto report = root / L"region.csv";
    check(ExportBinaryReportRegion(left,right,true,region,report,BinaryReportFormat::Csv,control,&stats,&error) && stats.added == 2 && Read(report).find("0x0000000000000015") != std::string::npos && Read(report).find("0x0000000000000014") == std::string::npos,L"full report uses the same range/ignore rules and absolute offsets");
    region.ignore_left = {{10,1}}; differences.clear(); check(CompareDocumentRegions(left,right,true,region,cancelled,collect,[](unsigned) {},&error) && differences.size() == 1 && differences[0].right_offset == 38,L"left ignored spans also suppress insertions anchored inside their interval");
    region.left_length = 1000; differences.clear(); check(!CompareDocumentRegions(left,right,true,region,cancelled,collect,[](unsigned) {},&error) && differences.empty(),L"invalid regional input emits no comparison result");
    ParseCompareRegionSpec(L"0,0,0,0",&region,&error); check(CompareDocumentRegions(left,right,false,region,cancelled,collect,[](unsigned) {},&error) && differences.empty(),L"two empty selected ranges compare equal");
    WorkspaceEntry entry; entry.left = a.wstring(); entry.binary_region = L"d:10,d:32,d:20,d:35;R:d:20+1"; entry.options = 600; entry.hex_rows[0] = 32; entry.hex_groups[0] = 4; entry.hex_bases[0] = 0x80000000; std::vector<WorkspaceEntry> loaded;
    check(SaveWorkspaceFile(root / L"ranges.ini",{entry},&error) && LoadWorkspaceFile(root / L"ranges.ini",&loaded,&error) && loaded.size() == 1 && loaded[0].binary_region == entry.binary_region && loaded[0].hex_rows[0] == 32 && loaded[0].hex_groups[0] == 4 && loaded[0].hex_bases[0] == 0x80000000 && loaded[0].options == 600,L"workspace settings preserve ranges, Hex layout/base and the similar-line option");
    const auto old = root / L"old-settings.ini"; Write(old,"[SimpleEditorToolWorkspace]\nVersion=1\nCount=1\n[Session0]\nLeft=example.bin\n"); loaded.clear(); check(LoadWorkspaceFile(old,&loaded,&error) && loaded[0].binary_region.empty() && loaded[0].hex_rows[0] == 16 && loaded[0].hex_groups[0] == 8,L"older workspace settings retain compatible default range/layout values");
    return failures;
}

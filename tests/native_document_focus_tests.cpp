#include "../src/core/syntax_profiles.h"
#include "../src/core/function_outline.h"
#include "../src/core/text_page.h"
#include "../src/core/portable_preferences.h"
#include "../src/core/compare_presets.h"
#include "../src/core/folder_policy.h"
#include "../src/core/review_notes.h"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace simple_editor::core;
int RunDocumentFocusTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    const auto write = [](const std::filesystem::path& path,const std::string& text) { std::ofstream file(path,std::ios::binary); file.write(text.data(),text.size()); };
    const auto read = [](const std::filesystem::path& path) { std::ifstream file(path,std::ios::binary); return std::string{std::istreambuf_iterator<char>(file),{}}; };
    std::atomic_bool cancel{false}; std::wstring error;
    TextCodec codec; codec.encoding = TextEncoding::Utf16Le; codec.bom = true;
    check(DescribeTextCodec(codec,L"a\r\nb\nc\r") == L"UTF-16 LE; BOM; EOL=CRLF+LF+CR",L"decoded codec summary distinguishes BOM and mixed original line endings");
    WorkspaceEntry entry; entry.left = L"example.txt"; entry.advanced_encoding = 2; entry.advanced_encoding_right = 4; entry.advanced_encoding_base = 3; entry.syntax_mode = L"cpp"; std::vector<WorkspaceEntry> entries;
    const auto workspace = root / L"independent.ini";
    check(SaveWorkspaceFile(workspace,{entry},&error) && LoadWorkspaceFile(workspace,&entries,&error) && entries[0].advanced_encoding_right == 4 && entries[0].advanced_encoding_base == 3 && entries[0].syntax_mode == L"cpp",L"workspace preserves independent codecs and manual syntax without buffers");
    const auto legacy = root / L"legacy.ini"; write(legacy,"[SimpleEditorToolWorkspace]\nVersion=1\nCount=1\n[Session0]\nLeft=x.txt\nAdvancedEncoding=4\n");
    check(LoadWorkspaceFile(legacy,&entries,&error) && entries[0].advanced_encoding_right == 4 && entries[0].advanced_encoding_base == 4 && entries[0].syntax_mode.empty(),L"legacy shared-codec workspace restores all sides and automatic syntax");
    std::vector<ComparePreset> presets{{L"Mixed",entry}},restored;
    check(SaveComparePresets(root / L"presets.ini",presets,&error) && LoadComparePresets(root / L"presets.ini",&restored,&error) && restored[0].settings.advanced_encoding_right == 4 && restored[0].settings.advanced_encoding_base == 3,L"path-free presets retain all three input encoding choices");
    entry.advanced_encoding_right = 6; check(!SaveWorkspaceFile(workspace,{entry},&error),L"unsupported per-side codec cannot replace workspace settings");
    std::map<std::wstring,std::wstring> mappings;
    check(ParseSyntaxAssociations(L".CONF=JSON\r\n.inc=C/C++\n.raw=Plain text",&mappings,&error) && mappings[L".conf"] == L"json" && mappings[L".inc"] == L"cpp" && mappings[L".raw"] == L"text",L"syntax associations accept familiar language labels and normalize extension case");
    const auto prior = mappings; check(!ParseSyntaxAssociations(L".conf=JSON\n.CONF=Python",&mappings,&error) && mappings == prior && !ParseSyntaxAssociations(L"../x=JSON",&mappings,&error) && !ParseSyntaxAssociations(L".x=downloaded-plugin",&mappings,&error),L"duplicate, path-shaped and unsupported syntax rules reject atomically");
    check(ResolveSyntax(L"settings.CONF",L".conf=JSON") == L"json" && ResolveSyntax(L"unknown",L".conf=JSON").empty(),L"extension resolution leaves unmatched documents in automatic mode");
    AppPreferences preference,loaded; preference.syntax_associations = L".conf=JSON\n.inc=C/C++";
    const auto preferences = root / L"syntax.ini"; write(preferences,"[private]\nkeep=untouched\n");
    check(SavePreferences(preferences,preference,&error) && LoadPreferences(preferences,&loaded,&error) && loaded.syntax_associations == preference.syntax_associations && read(preferences).find("keep=untouched") != std::string::npos,L"syntax preference storage preserves multiline mappings and unrelated INI values");
    bool truncated = false; std::vector<MarkdownHeading> functions;
    const std::wstring code = L"#define FAKE(x) { x; }\n// int fake() {}\nconst char* s = R\"tag(int hidden() { return 0; })tag\";\nnamespace N {\nint first(int a) { if (a) { return a; } return 0; }\nclass C { public: int method() const { return 1; } };\n}\nint prototype();\nvoid final_fn(\n int x\n) { }\n";
    check(CppFunctionOutline(code,cancel,&functions,&truncated) && !truncated && functions.size() == 3 && functions[0].line == 5 && functions[1].line == 6 && functions[2].line == 9,L"C/C++ outline finds definitions across lines and skips directives/comments/raw strings/prototypes");
    check(functions.size() == 3 && functions[0].title.find(L"first") != std::wstring::npos && functions[1].title.find(L"method") != std::wstring::npos,L"outline titles retain original definition signatures and source lines");
    cancel = true; const auto old_functions = functions; check(!CppFunctionOutline(code,cancel,&functions,&truncated) && functions.size() == old_functions.size(),L"cancelled outline keeps the previous result vector"); cancel = false;
    std::wstring many; for (unsigned i = 0; i < 4100; ++i) many += L"int f" + std::to_wstring(i) + L"() {}\n";
    check(CppFunctionOutline(many,cancel,&functions,&truncated) && truncated && functions.size() == 4096,L"outline bounds retained definitions without claiming complete analysis");
    std::vector<std::uint8_t> needle; TextCodec utf8; check(EncodeText(L"中\U0001F680tail",utf8,false,&needle,&error),L"encode supplementary-character paged query");
    const std::size_t first = 1024 * 1024 - 2,second = first + 1024; std::string payload(second + needle.size() + 20,'a'); std::copy(needle.begin(),needle.end(),payload.begin() + first); std::copy(needle.begin(),needle.end(),payload.begin() + second);
    const auto paged = root / L"paged.txt"; write(paged,payload); BinaryDocument document; document.Open(paged.wstring(),&error); std::uint64_t hit = 0,after = 0;
    check(FindPagedText(document,utf8,L"中\U0001F680tail",0,{},&hit,&after,&error) && hit == first && after == first + needle.size(),L"streamed literal search finds UTF-8 query across a read-chunk boundary");
    check(FindPagedText(document,utf8,L"中\U0001F680tail",after,{},&hit,&after,&error) && hit == second,L"Find Next advances by physical encoded bytes without repeating a match");
    check(FindPagedText(document,utf8,L"中\U0001F680tail",after,{},&hit,&after,&error) && hit == UINT64_MAX && after == document.Size(),L"paged search reaches EOF with an explicit not-found result");
    hit = 123; after = 456; SaveControl stopped; cancel = true; stopped.cancelled = &cancel; check(!FindPagedText(document,utf8,L"tail",0,stopped,&hit,&after,&error) && hit == 123 && after == 456,L"cancelled paged search never adopts partial hit state"); cancel = false;
    TextCodec big5; big5.encoding = TextEncoding::Big5; big5.code_page = 950; check(!FindPagedText(document,big5,L"tail",0,{},&hit,&after,&error),L"paged search reports legacy-code-page boundary limitation explicitly");
    TextCodec utf16; utf16.encoding = TextEncoding::Utf16Le; utf16.bom = true; std::vector<std::uint8_t> wide; EncodeText(L"a中\U0001F680z",utf16,true,&wide,&error); const auto wide_path = root / L"wide.txt"; { std::ofstream file(wide_path,std::ios::binary); file.write(reinterpret_cast<const char*>(wide.data()),wide.size()); } BinaryDocument wide_document; wide_document.Open(wide_path.wstring(),&error);
    check(FindPagedText(wide_document,utf16,L"中\U0001F680",3,{},&hit,&after,&error) && hit == 4 && after == 10,L"UTF-16 search skips odd offsets and preserves surrogate-pair byte spans");
    { std::ofstream changed(paged,std::ios::binary | std::ios::app); changed << 'x'; } check(!FindPagedText(document,utf8,L"tail",0,{},&hit,&after,&error),L"paged search rejects an externally changed original source"); document.Close();
    const auto left = root / L"left",right = root / L"right"; std::filesystem::create_directories(left / L"sub"); std::filesystem::create_directories(right / L"sub");
    for (const auto& base : {left,right}) { write(base / L"one.txt","same"); write(base / L"two.txt","same"); write(base / L"sub" / L"old.txt","old"); }
    FolderOptions options; FolderScan scan,refreshed; check(ScanFolders(left,right,options,cancel,{},&scan,&error),L"capture initial complete folder model"); write(right / L"one.txt","changed");
    const auto held = CreateFileW((left / L"two.txt").c_str(),GENERIC_WRITE,FILE_SHARE_WRITE | FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);
    const bool selected_ok = RefreshFolderSelection(scan,options,{L"one.txt"},cancel,{},&refreshed,&error); if (held != INVALID_HANDLE_VALUE) CloseHandle(held);
    check(held != INVALID_HANDLE_VALUE && selected_ok && refreshed.partial_refresh,L"selected refresh succeeds without reading an unselected locked file");
    std::size_t one = SIZE_MAX,two = SIZE_MAX; for (std::size_t i = 0; i < refreshed.rows.size(); ++i) { if (refreshed.rows[i].relative == L"one.txt") one = i; if (refreshed.rows[i].relative == L"two.txt") two = i; }
    check(one < refreshed.rows.size() && two < refreshed.rows.size() && refreshed.rows[one].fresh && refreshed.rows[one].state == FolderState::Different && !refreshed.rows[two].fresh && refreshed.rows[two].state == FolderState::Equal,L"selected results update atomically while unselected rows are explicitly older");
    SyncPlan plan; check(two < refreshed.rows.size() && !BuildSyncPlan(refreshed,{two},true,false,&plan,&error),L"selected copy refuses rows that were not rescanned");
    SyncBundle bundle; check(one < refreshed.rows.size() && !BuildPolicySyncPlan(refreshed,{one},true,SyncPolicy::Newer,0,false,nullptr,cancel,{},&bundle,&error),L"policy synchronization requires a full comparison after partial refresh");
    FolderBaseline baseline; check(!CaptureFolderBaseline(refreshed,cancel,{},&baseline,&error),L"direct baseline capture also refuses a partially refreshed model");
    write(right / L"sub" / L"new.txt","new"); std::filesystem::remove(left / L"sub" / L"old.txt"); std::filesystem::remove(right / L"sub" / L"old.txt");
    check(RefreshFolderSelection(scan,options,{L"sub"},cancel,{},&refreshed,&error) && std::any_of(refreshed.rows.begin(),refreshed.rows.end(),[](const auto& row) { return row.relative == L"sub\\new.txt" && row.fresh; }) && std::none_of(refreshed.rows.begin(),refreshed.rows.end(),[](const auto& row) { return row.relative == L"sub\\old.txt"; }),L"selected subtree refresh incorporates new and removed descendants");
    const auto retained_rows = refreshed.rows.size(); check(!RefreshFolderSelection(scan,options,{L"..\\outside"},cancel,{},&refreshed,&error) && refreshed.rows.size() == retained_rows && !FolderScopeContains(L"sub",L"submarine\\x"),L"scope validation rejects traversal and component-prefix confusion");
    cancel = true; check(!RefreshFolderSelection(scan,options,{L"one.txt"},cancel,{},&refreshed,&error) && refreshed.rows.size() == retained_rows,L"cancelled selected refresh preserves the previous complete result"); cancel = false;
    TextMergeModel model; check(BuildTextMerge(L"base\n",L"<left>\n",L"right\n",true,{},cancel,&model,&error),L"build real three-way model for review metadata"); ComparisonNotes notes; check(FingerprintComparison(model,cancel,&notes.fingerprint,&error),L"fingerprint original text/endings and block mappings");
    const auto fingerprint = notes.fingerprint; std::size_t block = 0; while (block < model.blocks.size() && !model.blocks[block].changed) ++block; if (block < model.blocks.size()) { notes.notes[block] = {L"<script> & 中文\r\nreason",true}; model.blocks[block].choice = MergeChoice::Right; }
    std::wstring same; check(FingerprintComparison(model,cancel,&same,&error) && same == fingerprint,L"review identity is independent from merge-side choices");
    const auto metadata = root / L"notes.setreview"; ComparisonNotes loaded_notes; check(SaveReviewNotes(metadata,notes,{}, {},&error) && ReadReviewNotes(metadata,&loaded_notes,&error) && loaded_notes.fingerprint == notes.fingerprint && loaded_notes.notes.at(block).text == notes.notes.at(block).text,L"review metadata roundtrips Unicode and review state without source buffers");
    const auto report = root / L"review.html"; check(ExportReviewedHtml(model,notes,{},report,{},&error) && read(report).find("&lt;script&gt; &amp;") != std::string::npos && read(report).find("<th>Base</th>") != std::string::npos,L"annotated three-way HTML streams Base/source columns and escapes note content");
    auto wrong = notes; wrong.fingerprint[0] = wrong.fingerprint[0] == L'A' ? L'B' : L'A'; const auto old_report = read(report); check(!ExportReviewedHtml(model,wrong,{},report,{},&error) && read(report) == old_report,L"mismatched review fingerprint cannot replace an existing report");
    const auto old_notes = loaded_notes.fingerprint; check(!ParseReviewNotes(L"[SimpleEditorToolTextReview]\nVersion=99\n",&loaded_notes,&error) && loaded_notes.fingerprint == old_notes,L"invalid review metadata rejects atomically");
    const auto source = root / L"protected.txt"; write(source,"original"); FileStamp stamp; InspectFile(source,&stamp,&error); const std::vector<ProtectedInput> protected_input{{source,{stamp.volume,stamp.identity}}};
    check(!SaveReviewNotes(source,notes,protected_input,{},&error) && read(source) == "original",L"review metadata cannot overwrite a protected source path"); const auto alias = root / L"alias.txt"; const bool linked = CreateHardLinkW(alias.c_str(),source.c_str(),nullptr) != FALSE;
    check(linked && !ExportReviewedHtml(model,notes,protected_input,alias,{},&error) && read(source) == "original",L"annotated report refuses a hard link to a protected source");
    return failures;
}

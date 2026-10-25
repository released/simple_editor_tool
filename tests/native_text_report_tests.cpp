#include "../src/core/text_report.h"
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
namespace {
std::string Read(const std::filesystem::path& path) { std::ifstream in(path,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
void Write(const std::filesystem::path& path,const std::string& text) { std::ofstream out(path,std::ios::binary); out.write(text.data(),text.size()); }
}
int RunTextReportTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; }; std::atomic_bool cancelled{false}; std::wstring error,patch,output; TextReportStats stats;
    const std::wstring a = L"alpha\nbeta\ncharlie\ndelta\necho\nfoxtrot\ngolf\nhotel\nindia\njuliet\nkilo\nlima\n", b = L"alpha\nBETA\ncharlie\ndelta\necho\nfoxtrot\ngolf\nhotel\nindia\njuliet\nKILO\nlima\nextra\n";
    check(BuildUnifiedPatch(a,b,L"left.txt",L"right.txt",cancelled,&patch,&stats,&error) && stats.hunks == 2 && patch.find(L"--- a/left.txt\n+++ b/right.txt") != std::wstring::npos,L"unified patch groups three context lines into distinct hunks");
    check(ApplyUnifiedPatch(a,patch,cancelled,&output,&stats,&error) && output == b && stats.added == 3 && stats.removed == 2,L"generated multi-hunk patch validates and reproduces the complete right text");
    const auto previous = output; check(!ApplyUnifiedPatch(L"changed outside the hunks\n" + a,patch,cancelled,&output,&stats,&error) && output == previous && error.find(L"SHA-256") != std::wstring::npos,L"generated patch rejects a different normalized source and retains old output");
    BuildUnifiedPatch(L"a",L"b",L"a",L"b",cancelled,&patch,&stats,&error); check(patch.find(L"\\ No newline at end of file") != std::wstring::npos && ApplyUnifiedPatch(L"a",patch,cancelled,&output,&stats,&error) && output == L"b",L"patch preserves unterminated final lines");
    check(!ApplyUnifiedPatch(L"a\n",patch,cancelled,&output,&stats,&error),L"generated patch distinguishes an existing final newline");
    BuildUnifiedPatch(L"",L"new\n",L"empty",L"new",cancelled,&patch,&stats,&error); check(patch.find(L"@@ -0,0 +1,1 @@") != std::wstring::npos && ApplyUnifiedPatch(L"",patch,cancelled,&output,&stats,&error) && output == L"new\n",L"empty-source patch uses valid zero-line hunk positions");
    BuildUnifiedPatch(L"old\n",L"",L"old",L"empty",cancelled,&patch,&stats,&error); check(ApplyUnifiedPatch(L"old\n",patch,cancelled,&output,&stats,&error) && output.empty(),L"patch can produce an empty output buffer");
    const std::wstring generic = L"--- a/file\n+++ b/file\n@@ -1,2 +1,2 @@\n a\n-b\n+c\n";
    check(ApplyUnifiedPatch(L"a\r\nb\r\n",generic,cancelled,&output,&stats,&error) && output == L"a\r\nc\r\n",L"external unified patch checks exact content while retaining source CRLF style");
    check(ApplyUnifiedPatch(L"a\nb\r\n",generic,cancelled,&output,&stats,&error) && output == L"a\nc\r\n",L"patch replacements retain the local ending of a mixed-EOL source line");
    check(!ApplyUnifiedPatch(L"a\r\nx\r\n",generic,cancelled,&output,&stats,&error),L"patch without a source hash still rejects mismatched context/deletions");
    check(!ApplyUnifiedPatch(L"a\nb\n",L"--- x\n+++ y\n@@ -1,1 +1,1 @@\n-a\n+b\n+c\n",cancelled,&output,&stats,&error),L"malformed hunk counts are rejected");
    check(!ApplyUnifiedPatch(L"a\nb\n",L"--- x\n+++ y\n@@ -2,1 +1,1 @@\n-b\n+c\n",cancelled,&output,&stats,&error),L"wrong new-side positions are rejected");
    check(!ApplyUnifiedPatch(L"a\n",L"--- /dev/null\n+++ file\n@@ -0,0 +1,1 @@\n+new\n",cancelled,&output,&stats,&error),L"creation patch cannot append to a nonempty source");
    check(!ApplyUnifiedPatch(L"a\n",generic + L"--- other\n+++ other\n",cancelled,&output,&stats,&error),L"multiple-file patches cannot write through implicit header paths");
    BuildUnifiedPatch(L"甲\n",L"乙\n",L"a",L"b",cancelled,&patch,&stats,&error); check(ApplyUnifiedPatch(L"甲\r",patch,cancelled,&output,&stats,&error) && output == L"乙\r",L"Unicode patch source hash normalizes line style and output preserves CR");
    BuildUnifiedPatch(L"a\r\n",L"a\n",L"a",L"b",cancelled,&patch,&stats,&error); check(stats.hunks == 0 && ApplyUnifiedPatch(L"a\r\n",patch,cancelled,&output,&stats,&error) && output == L"a\r\n",L"content patches leave EOL-only conversion to the explicit encoding/EOL tool");
    cancelled = true; const auto kept = output; check(!ApplyUnifiedPatch(a,generic,cancelled,&output,&stats,&error) && output == kept,L"cancelled patch leaves the current output intact"); cancelled = false;
    const auto source = root / L"report-source.txt"; Write(source,"unchanged"); BinaryDocument document; document.Open(source.wstring(),&error); const std::vector<ProtectedInput> inputs{{source,document.SourceIdentity()}}; SaveControl control; control.cancelled = &cancelled;
    const auto html = root / L"text.html"; check(ExportTextReport(L"<script>&\n",L"changed\n",inputs,html,TextReportFormat::Html,control,&stats,&error) && Read(html).find("&lt;script&gt;&amp;") != std::string::npos && Read(html).find("<script>") == std::string::npos,L"HTML report escapes source content and contains full line pairs");
    const auto summary = root / L"summary.txt"; check(ExportTextReport(L"a\nb\n",L"a\nc\n",inputs,summary,TextReportFormat::Summary,control,&stats,&error) && stats.blocks == 1 && stats.equal == 1 && stats.added == 1 && stats.removed == 1,L"summary report returns exact block and line counts");
    check(!ExportTextReport(a,b,inputs,source,TextReportFormat::Html,control,&stats,&error) && Read(source) == "unchanged",L"text reports cannot overwrite their inputs");
    Write(summary,"keep"); cancelled = true; check(!ExportTextReport(a,b,inputs,summary,TextReportFormat::UnifiedPatch,control,&stats,&error) && Read(summary) == "keep",L"cancelled patch export preserves an existing destination"); cancelled = false;
    check(!ApplyUnifiedPatch(L"a",std::wstring(L"x\0y",3),cancelled,&output,&stats,&error),L"patch parser rejects embedded NUL text");
    return failures;
}

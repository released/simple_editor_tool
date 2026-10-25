#include "binary_report.h"
#include "stream_output.h"
#include <iomanip>
#include <sstream>

namespace simple_editor::core {
namespace {
std::wstring Hex(std::uint64_t value, unsigned digits) { std::wostringstream text; text << std::uppercase << std::hex << std::setfill(L'0') << std::setw(digits) << value; return text.str(); }
std::wstring Escape(const std::wstring& value) { std::wstring result; for (auto c : value) { if (c == L'&') result += L"&amp;"; else if (c == L'<') result += L"&lt;"; else if (c == L'>') result += L"&gt;"; else if (c == L'\"') result += L"&quot;"; else result += c; } return result; }
}
bool ExportBinaryReportRegion(const BinaryDocument& left, const BinaryDocument& right, bool insertion_aware,const CompareRegion& region,
    const std::filesystem::path& output, BinaryReportFormat format, const SaveControl& control,
    BinaryReportStats* stats, std::wstring* error,bool require_new) {
    if (error) error->clear(); FileStamp target;
    std::error_code path_error; const auto absolute_output = std::filesystem::absolute(output,path_error).lexically_normal().wstring();
    const auto same_path = [&](const std::wstring& source) { std::error_code code; const auto absolute_source = std::filesystem::absolute(source,code).lexically_normal().wstring(); return !code && !path_error && CompareStringOrdinal(absolute_output.c_str(),-1,absolute_source.c_str(),-1,TRUE) == CSTR_EQUAL; };
    if (path_error || same_path(left.Path()) || same_path(right.Path())) { if (error) *error = L"A report needs a different output path from its inputs."; return false; }
    if (!InspectFile(output,&target,error)) return false;
    if (target.exists && (std::make_pair(target.volume,target.identity) == left.SourceIdentity() || std::make_pair(target.volume,target.identity) == right.SourceIdentity())) { if (error) *error = L"A report cannot overwrite either comparison input."; return false; }
    StreamOutput stream; if (!stream.Open(output,error,require_new)) return false;
    const bool html = format == BinaryReportFormat::Html; const wchar_t delimiter = format == BinaryReportFormat::Tsv ? L'\t' : L',';
    if (!stream.Text(html ? L"<!doctype html><meta charset=\"utf-8\"><title>Binary comparison</title><style>body{font:14px Consolas,monospace}table{border-collapse:collapse}td,th{padding:4px 12px;border:1px solid #bbb}</style><h1>Binary comparison</h1><p>Left: " + Escape(left.Path()) + L"<br>Right: " + Escape(right.Path()) + L"</p><table><tr><th>Kind</th><th>Left address</th><th>Left byte</th><th>Right address</th><th>Right byte</th></tr>" : std::wstring(L"kind") + delimiter + L"left_address" + delimiter + L"left_byte" + delimiter + L"right_address" + delimiter + L"right_byte\r\n",error)) return false;
    std::atomic_bool never_cancel{false}; const auto& cancel = control.cancelled ? *control.cancelled : never_cancel;
    BinaryReportStats result; Difference previous{}; bool first = true, written = true;
    const bool compared = CompareDocumentRegions(left,right,insertion_aware,region,cancel,[&](const DifferenceBatch& batch) {
        std::wstring buffer; buffer.reserve(batch.size() * 90);
        for (const auto& d : batch) {
            if (cancel) return false;
            const bool added = d.left_byte < 0, removed = d.right_byte < 0;
            if (added) ++result.added; else if (removed) ++result.removed; else ++result.changed;
            const bool contiguous = !first && ((d.left_offset == kMissingOffset && previous.left_offset == kMissingOffset) || (d.left_offset != kMissingOffset && previous.left_offset != kMissingOffset && d.left_offset == previous.left_offset + 1)) && ((d.right_offset == kMissingOffset && previous.right_offset == kMissingOffset) || (d.right_offset != kMissingOffset && previous.right_offset != kMissingOffset && d.right_offset == previous.right_offset + 1));
            if (!contiguous) ++result.ranges; previous = d; first = false;
            const std::wstring cells[] = {added ? L"added" : removed ? L"removed" : L"changed",d.left_offset == kMissingOffset ? L"" : L"0x" + Hex(d.left_offset,16),added ? L"" : Hex(d.left_byte,2),d.right_offset == kMissingOffset ? L"" : L"0x" + Hex(d.right_offset,16),removed ? L"" : Hex(d.right_byte,2)};
            if (html) buffer += L"<tr>";
            for (int column = 0; column < 5; ++column) { if (html) buffer += L"<td>" + cells[column] + L"</td>"; else { if (column) buffer += delimiter; buffer += cells[column]; } }
            buffer += html ? L"</tr>\n" : L"\r\n";
        }
        written = stream.Text(buffer,error); return written;
    },[&](unsigned p) { if (control.progress) control.progress(p * 80 / 100); },error);
    if (!compared || !written || cancel) { if (error && error->empty()) *error = cancel ? L"Report cancelled; output retained." : L"Report generation failed."; return false; }
    if (html && !stream.Text(L"</table><p>Changed: " + std::to_wstring(result.changed) + L"; added: " + std::to_wstring(result.added) + L"; removed: " + std::to_wstring(result.removed) + L"; ranges: " + std::to_wstring(result.ranges) + L"</p>",error)) return false;
    SaveControl commit = control; commit.progress = [&](unsigned p) { if (control.progress) control.progress(80 + p / 5); };
    if (!stream.Commit(commit,error)) return false;
    if (stats) *stats = result; return true;
}
bool ExportBinaryReport(const BinaryDocument& left,const BinaryDocument& right,bool insertion_aware,const std::filesystem::path& output,BinaryReportFormat format,const SaveControl& control,BinaryReportStats* stats,std::wstring* error) { return ExportBinaryReportRegion(left,right,insertion_aware,{},output,format,control,stats,error); }
}

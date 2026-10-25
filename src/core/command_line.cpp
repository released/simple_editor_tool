#include "command_line.h"
#include "binary_report.h"
#include "text_report.h"
#include "folder_sync.h"
#include <algorithm>
#include <map>
#include <set>
namespace simple_editor::core {
namespace {
bool Outside(const std::filesystem::path& path,const FileStamp& root,std::wstring* error) {
    std::error_code code; auto part = std::filesystem::absolute(path,code).lexically_normal(); if (code) return false;
    for (;;) { FileStamp stamp; if (!InspectFile(part,&stamp,error)) return false; if (stamp.exists && stamp.volume == root.volume && stamp.identity == root.identity) { if (error) *error = L"Folder reports must be outside both comparison roots."; return false; } const auto parent = part.parent_path(); if (parent.empty() || parent == part) break; part = parent; } return true;
}
std::wstring Csv(const std::wstring& text) { std::wstring value = L"\""; for (auto c : text) { if (c == L'\"') value += L'\"'; value += c; } value += L'\"'; return value; }
bool FolderReport(const FolderScan& scan,const std::filesystem::path& path,bool overwrite,const SaveControl& control,std::wstring* error) {
    FileStamp output; if (!InspectFile(path,&output,error) || !Outside(path,scan.left_identity,error) || !Outside(path,scan.right_identity,error)) return false;
    for (const auto& row : scan.rows) for (const auto& stamp : {row.left,row.right}) if (stamp.exists && !stamp.directory && output.exists && stamp.volume == output.volume && stamp.identity == output.identity) { if (error) *error = L"Report output aliases a comparison input."; return false; }
    StreamOutput stream; if (!stream.Open(path,error,!overwrite) || !stream.Text(L"relative_path,state,left_bytes,right_bytes,left_modified,right_modified,newer,error\r\n",error)) return false;
    const wchar_t* const states[] = {L"equal",L"different",L"left-only",L"right-only",L"type-conflict",L"error"};
    for (const auto& row : scan.rows) { if (control.cancelled && *control.cancelled) return false; const auto text = Csv(row.relative.wstring()) + L"," + states[static_cast<unsigned>(row.state)] + L"," + std::to_wstring(row.left.size) + L"," + std::to_wstring(row.right.size) + L"," + std::to_wstring(row.left.modified) + L"," + std::to_wstring(row.right.modified) + L"," + (row.newness == FolderNewness::LeftNewer ? L"left" : row.newness == FolderNewness::RightNewer ? L"right" : L"same-time") + L"," + Csv(row.error) + L"\r\n"; if (!stream.Text(text,error)) return false; }
    return stream.Commit(control,error);
}
bool Decode(BinaryDocument& document,TextEncoding encoding,std::wstring* text,std::wstring* error) {
    if (document.Size() > 16 * 1024 * 1024) { if (error) *error = L"CLI text inputs are limited to 16 MiB; use binary mode for larger files."; return false; }
    std::vector<std::uint8_t> bytes; TextCodec codec; return document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) && DecodeText(bytes,encoding,text,&codec,error) && document.ValidateSource(true,error);
}
}
std::wstring CommandLineHelp() {
    return L"Simple Editor Tool native read-only comparison\r\n"
        L"--compare --left <path> --right <path> [--base <path>] [--mode <mode>]\r\n"
        L"  Modes: binary (default), binary-aligned, text, threeway, folders\r\n"
        L"  --report <path> [--format <format>] [--overwrite]\r\n"
        L"  Binary formats: csv (default), tsv, html. Text: html (default), summary, patch.\r\n"
        L"  Threeway: summary only. Folders: csv only. Existing reports require --overwrite.\r\n"
        L"  --encoding auto|utf8|utf16le|utf16be|big5|system (text/threeway)\r\n"
        L"  --region <left-start,left-length,right-start,right-length;L:...;R:...> (binary)\r\n"
        L"  --include <globs> --exclude <globs> --no-recursive (folders)\r\n"
        L"  --criterion content|size|sizetime|sha256 (folders)\r\n"
        L"Exit codes: 0 equal, 1 different, 2 error, 3 cancelled. Inputs are never written.\r\n";
}
CommandLineResult CompareCommandLine(const std::vector<std::wstring>& arguments,const SaveControl& control) {
    CommandLineResult result; std::atomic_bool local_cancel{false}; const auto& cancel = control.cancelled ? *control.cancelled : local_cancel;
    const auto error = [&](const std::wstring& text) { result.exit_code = cancel ? 3 : 2; result.message = text.empty() ? cancel ? L"Comparison cancelled." : L"Comparison or report failed." : text; return result; };
    try {
        if (arguments == std::vector<std::wstring>{L"--help"} || arguments == std::vector<std::wstring>{L"--compare",L"--help"}) { result.exit_code = 0; result.help = true; result.message = CommandLineHelp(); return result; }
        if (arguments.empty() || arguments[0] != L"--compare") return error(L"Use --compare for a noninteractive comparison, or --help.");
        std::map<std::wstring,std::wstring> options; std::set<std::wstring> flags; const std::set<std::wstring> values{L"--left",L"--right",L"--base",L"--mode",L"--report",L"--format",L"--encoding",L"--region",L"--include",L"--exclude",L"--criterion"};
        for (std::size_t i = 1; i < arguments.size(); ++i) { const auto& name = arguments[i]; if (name == L"--overwrite" || name == L"--no-recursive") { if (!flags.insert(name).second) return error(L"Duplicate command-line flag: " + name); } else { if (!values.count(name) || i + 1 >= arguments.size() || arguments[i + 1].empty()) return error(L"Unknown option or missing value: " + name); if (!options.emplace(name,arguments[++i]).second) return error(L"Duplicate command-line option: " + name); } }
        if (!options.count(L"--left") || !options.count(L"--right")) return error(L"Both --left and --right are required.");
        const auto mode = options.count(L"--mode") ? options[L"--mode"] : L"binary"; const bool binary = mode == L"binary" || mode == L"binary-aligned",threeway = mode == L"threeway",folder = mode == L"folders"; if (!binary && !threeway && !folder && mode != L"text") return error(L"Unknown comparison mode.");
        if (threeway != static_cast<bool>(options.count(L"--base"))) return error(L"--base is required only for threeway mode.");
        if (!binary && options.count(L"--region")) return error(L"--region requires a binary mode.");
        if (!folder && (options.count(L"--include") || options.count(L"--exclude") || options.count(L"--criterion") || flags.count(L"--no-recursive"))) return error(L"Folder options require folders mode.");
        if (folder && options.count(L"--encoding")) return error(L"--encoding requires a text mode.");
        const auto format = options.count(L"--format") ? options[L"--format"] : binary || folder ? L"csv" : threeway ? L"summary" : L"html";
        if ((binary && format != L"csv" && format != L"tsv" && format != L"html") || (folder && format != L"csv") || (threeway && format != L"summary") || (!binary && !folder && !threeway && format != L"html" && format != L"summary" && format != L"patch")) return error(L"Report format is not supported for this mode.");
        const auto report = options[L"--report"]; if (report.empty() && (options.count(L"--format") || flags.count(L"--overwrite"))) return error(L"--format/--overwrite require --report.");
        const bool overwrite = flags.count(L"--overwrite") != 0; std::wstring failure; if (!report.empty()) { FileStamp target; if (!InspectFile(report,&target,&failure)) return error(failure); if (target.exists && !overwrite) return error(L"The report already exists. Use --overwrite explicitly or choose another path."); }
        if (cancel) return error(L"Comparison cancelled."); bool committed = false;
        if (folder) {
            FolderOptions rules; rules.recursive = !flags.count(L"--no-recursive"); if (options.count(L"--include")) rules.include = options[L"--include"]; rules.exclude = options[L"--exclude"];
            if (options.count(L"--criterion")) { const auto value = options[L"--criterion"]; if (value != L"content" && value != L"size" && value != L"sizetime" && value != L"sha256") return error(L"Unknown folder criterion."); rules.criterion = value == L"size" ? FolderCriterion::Size : value == L"sizetime" ? FolderCriterion::SizeTime : value == L"sha256" ? FolderCriterion::Sha256 : FolderCriterion::Content; }
            FolderScan scan; if (!ScanFolders(options[L"--left"],options[L"--right"],rules,cancel,control.progress,&scan,&failure)) return error(failure);
            bool unreadable = false; for (const auto& row : scan.rows) { result.differences += row.state != FolderState::Equal; unreadable = unreadable || row.state == FolderState::Error; }
            if (!report.empty()) { if (!FolderReport(scan,report,overwrite,control,&failure)) return error(failure); committed = true; }
            if (unreadable) return error(L"Folder scan contains unreadable entries; inspect the report.");
        } else {
            BinaryDocument left,right,base; if (!left.Open(options[L"--left"],&failure) || !right.Open(options[L"--right"],&failure) || (threeway && !base.Open(options[L"--base"],&failure))) return error(failure);
            if (binary) {
                if (options.count(L"--encoding")) return error(L"--encoding requires a text mode."); CompareRegion region; if (options.count(L"--region") && !ParseCompareRegionSpec(options[L"--region"],&region,&failure)) return error(failure);
                if (!report.empty()) { BinaryReportStats stats; if (!ExportBinaryReportRegion(left,right,mode == L"binary-aligned",region,report,format == L"tsv" ? BinaryReportFormat::Tsv : format == L"html" ? BinaryReportFormat::Html : BinaryReportFormat::Csv,control,&stats,&failure,!overwrite)) return error(failure); result.differences = stats.changed + stats.added + stats.removed; committed = true; }
                else if (!CompareDocumentRegions(left,right,mode == L"binary-aligned",region,cancel,[&](const DifferenceBatch& batch) { result.differences += batch.size(); return !cancel; },[&](unsigned p) { if (control.progress) control.progress(p); },&failure)) return error(failure);
            } else {
                const auto selected = options.count(L"--encoding") ? options[L"--encoding"] : L"auto"; const std::vector<std::wstring> names{L"auto",L"utf8",L"utf16le",L"utf16be",L"big5",L"system"}; const auto found = std::find(names.begin(),names.end(),selected); if (found == names.end()) return error(L"Unknown text encoding."); const auto encoding = static_cast<TextEncoding>(found - names.begin()); std::wstring a,b,c;
                if (!Decode(left,encoding,&a,&failure) || !Decode(right,encoding,&b,&failure) || (threeway && !Decode(base,encoding,&c,&failure))) return error(failure); TextMergeModel model; if (!BuildTextMerge(c,a,b,threeway,{},cancel,&model,&failure)) return error(failure);
                std::size_t conflicts = 0; for (const auto& block : model.blocks) { result.differences += block.changed; conflicts += block.conflict; } if (threeway && (a != b || a != c) && !result.differences) result.differences = 1;
                std::vector<ProtectedInput> inputs{{left.Path(),left.SourceIdentity()},{right.Path(),right.SourceIdentity()}}; if (threeway) inputs.push_back({base.Path(),base.SourceIdentity()});
                if (!report.empty()) { if (threeway) { const auto text = L"Three-way comparison\r\nBase: " + base.Path() + L"\r\nLeft: " + left.Path() + L"\r\nRight: " + right.Path() + L"\r\nChanged blocks: " + std::to_wstring(result.differences) + L"\r\nConflicts: " + std::to_wstring(conflicts) + L"\r\n"; if (!ExportTextOutput(text,{},inputs,report,control,&failure,!overwrite)) return error(failure); } else { TextReportStats stats; if (!ExportTextReport(a,b,inputs,report,format == L"patch" ? TextReportFormat::UnifiedPatch : format == L"summary" ? TextReportFormat::Summary : TextReportFormat::Html,control,&stats,&failure,!overwrite)) return error(failure); } committed = true; }
            }
        }
        if (cancel && !committed) return error(L"Comparison cancelled."); result.exit_code = result.differences ? 1 : 0; result.message = (result.differences ? L"Different: " : L"Equal: ") + std::to_wstring(result.differences) + L" differing byte(s), text block(s), or folder entries."; if (!report.empty()) result.message += L" Report: " + report; return result;
    } catch (const std::exception&) { return error(L"Comparison failed: memory, path or I/O error."); }
}
}

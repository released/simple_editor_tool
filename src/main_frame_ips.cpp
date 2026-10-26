#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "core/ips_patch.h"
#include "core/stream_output.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
using namespace simple_editor::core;
namespace {
std::wstring IpsDetails(const IpsSummary& result) { return std::wstring(result.ips32 ? L"IPS32" : L"IPS") + L" equal-length patch\r\nRecords: " + std::to_wstring(result.records) + L"\r\nRecord bytes: " + std::to_wstring(result.bytes) + L"\r\nBase SHA-256:\r\n" + result.base_sha256 + L"\r\nResult SHA-256:\r\n" + result.result_sha256; }
bool SeparatePatch(const BinaryDocument& document,const std::wstring& path,std::wstring* error) {
    FileStamp stamp; std::error_code code; const auto target = std::filesystem::absolute(path,code).lexically_normal(); if (code || !InspectFile(path,&stamp,error)) return false;
    const auto source = std::filesystem::absolute(document.Path(),code).lexically_normal(); if (code) return false;
    if (CompareStringOrdinal(target.c_str(),-1,source.c_str(),-1,TRUE) == CSTR_EQUAL || (stamp.exists && document.SourceIdentity() == std::make_pair(stamp.volume,stamp.identity))) { *error = L"A patch output must be separate from its source document, including hard links."; return false; } return true;
}
}
void CMainFrame::ReviewIps(UINT command) {
    const bool importing = command == ID_REVIEW_IPS_IMPORT,wide = command == ID_REVIEW_IPS32_EXPORT;
    if (IsBusy() || comparison_session_ || ActiveHex() != &hex_ || !document_.IsOpen() || (importing && (MutationBlocked() || hex_.ReadOnly()))) { status_.SetPaneText(0,L"Choose a file Hex page; import requires a writable document."); return; }
    CFileDialog file(importing ? TRUE : FALSE,wide ? L"ips32" : L"ips",nullptr,importing ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT,L"IPS / IPS32 patches (*.ips;*.ips32)|*.ips;*.ips32|All files (*.*)|*.*||",this);
    path_dialog_ = true; const auto answer = file.DoModal(); path_dialog_ = false; if (answer != IDOK) return;
    std::wstring hash;
    if (importing) { CValueDialog digest(this,L"Verify IPS base SHA-256",L"Paste the expected base SHA-256 supplied with this patch (64 hex digits). IPS itself contains no base identity. The current draft must match it; no automatic saving.",L""); digest.validate = [](const auto& value,auto* error) { if (value.size() == 64 && std::all_of(value.begin(),value.end(),[](auto ch) { return (ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'f') || (ch >= L'A' && ch <= L'F'); })) return true; *error = L"Enter exactly 64 hexadecimal digits from the patch's base hash."; return false; }; path_dialog_ = true; const auto accepted = digest.DoModal(); path_dialog_ = false; if (accepted != IDOK) return; hash = digest.value; }
    StartIps(file.GetPathName().GetString(),importing,wide,hash);
}
bool CMainFrame::StartIps(const std::wstring& path,bool importing,bool wide,const std::wstring& expected) {
    if (IsBusy() || comparison_session_ || ActiveHex() != &hex_ || !document_.IsOpen() || (importing && (MutationBlocked() || hex_.ReadOnly())) || !ApplyTextEdits()) return false;
    if (document_.Size() > 64 * 1024 * 1024) { status_.SetPaneText(0,L"This patch workflow supports documents up to 64 MiB (IPS up to 16 MiB)."); return false; }
    std::wstring error; if (!importing && !SeparatePatch(document_,path,&error)) { status_.SetPaneText(0,error.c_str()); return false; }
    auto snapshot = document_.Snapshot(&error,importing); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return false; }
    if (!importing) { job_locked_paths_ = {path}; if (workspace_ && !workspace_->LockPaths(job_locked_paths_,true,&job_)) { job_locked_paths_.clear(); return false; } job_locks_held_ = workspace_ != nullptr; }
    cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_revision_ = revision_; job_editor_generation_ = text_.Generation(); job_kind_ = importing ? EditorJobKind::IpsImport : EditorJobKind::IpsExport;
    try { job_ = std::async(std::launch::async,[this,path,importing,wide,expected,snapshot = std::move(snapshot)]() mutable {
        EditorJobResult result; result.kind = importing ? EditorJobKind::IpsImport : EditorJobKind::IpsExport; result.ips = std::make_shared<IpsSummary>(); SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; };
        BinaryDocument input; std::vector<std::uint8_t> bytes;
        if (!input.Open(importing ? path : snapshot->Path(),&result.error)) return result;
        if (input.Size() > (importing ? 65 : 64) * 1024 * 1024 || !input.Read(0,static_cast<std::size_t>(input.Size()),&bytes) || !input.ValidateSource(true,&result.error)) { if (result.error.empty()) result.error = L"Patch/base exceeds the input limit or could not be captured."; return result; }
        if (importing) {
            if (!ApplyIpsPatch(*snapshot,bytes,expected,control,result.ips.get(),&result.error) || !input.ValidateSource(true,&result.error)) return result;
            result.details = IpsDetails(*result.ips) + L"\r\n\r\nThe hash-matched private draft has been validated. Apply it as one Undo action? Save remains separate.";
            result.details += L"\r\n\r\nRecord previews: first 200 records, first 16 bytes per record (all records are validated).\r\n";
            for (const auto& row : result.ips->preview) { std::wostringstream text; text << L"0x" << std::hex << std::uppercase << row.offset << L" + " << std::dec << row.length << L" bytes\r\n  before: "; for (auto byte : row.before) text << std::hex << std::setw(2) << std::setfill(L'0') << static_cast<unsigned>(byte) << L' '; text << L"\r\n  after:  "; for (auto byte : row.after) text << std::hex << std::setw(2) << std::setfill(L'0') << static_cast<unsigned>(byte) << L' '; result.details += text.str() + L"\r\n"; }
            result.document = std::move(snapshot);
        } else {
            std::vector<std::uint8_t> updated,patch; if (!snapshot->Read(0,static_cast<std::size_t>(snapshot->Size()),&updated) || !BuildIpsPatch(bytes,updated,wide,cancelled_,&patch,result.ips.get(),&result.error) || !snapshot->ValidateSource(true,&result.error) || !input.ValidateSource(true,&result.error) || !SeparatePatch(*snapshot,path,&result.error)) { if (result.error.empty()) result.error = L"Could not generate the patch from stable equal-length inputs."; return result; }
            StreamOutput output; if (!output.Open(path,&result.error) || !output.Write(patch.data(),patch.size(),&result.error)) return result;
            std::atomic_bool stop{false}; SaveControl commit; commit.cancelled = &stop; commit.progress = [&](unsigned p) { if (cancelled_ || (p == 99 && (!snapshot->ValidateSource(true,&result.error) || !input.ValidateSource(true,&result.error)))) stop = true; };
            if (!output.Commit(commit,&result.error)) return result; result.saved = true; result.details = IpsDetails(*result.ips) + L"\r\n\r\nShare the base SHA-256 alongside the patch. IPS/IPS32 files do not store it."; result.message = L"Patch exported to " + path;
        }
        return result;
    }); } catch (...) { ReleaseJobLocks(); status_.SetPaneText(0,L"Could not start the patch operation; draft retained."); return false; }
    status_.SetPaneText(0,importing ? L"Validating base SHA-256 and patch records on a private draft..." : L"Exporting current bytes versus the saved disk baseline..."); return true;
}
void CMainFrame::FinishIps(EditorJobResult result) {
    ReleaseJobLocks();
    if (result.kind == EditorJobKind::IpsExport) { status_.SetPaneText(0,result.saved ? result.message.c_str() : result.error.empty() ? L"Patch export cancelled." : result.error.c_str()); if (result.saved) { CTextResultDialog dialog(this,L"Exported patch - base and result hashes",result.details); path_dialog_ = true; dialog.DoModal(); path_dialog_ = false; } return; }
    if (cancelled_ || revision_ != job_revision_ || text_.Generation() != job_editor_generation_ || MutationBlocked() || !result.document || !result.error.empty()) { status_.SetPaneText(0,result.error.empty() ? L"Patch cancelled or document changed; draft retained." : result.error.c_str()); return; }
    CTextPreviewDialog preview(this,result.details); path_dialog_ = true; const auto answer = preview.DoModal(); path_dialog_ = false;
    std::wstring error;
    if (answer != IDOK || cancelled_ || revision_ != job_revision_ || text_.Generation() != job_editor_generation_ || MutationBlocked() || hex_.ReadOnly() || !result.document->ValidateSource(true,&error) || !document_.ValidateSource(true,&error)) { status_.SetPaneText(0,L"Patch preview cancelled, read-only or stale; draft retained."); return; }
    if (result.ips->base_sha256 != result.ips->result_sha256) { document_.Swap(*result.document); DocumentChanged(); hex_.SetDocument(&document_); ShowPage(0); }
    status_.SetPaneText(0,L"Hash-verified patch applied. One Undo restores the previous draft. Save writes the file.");
}

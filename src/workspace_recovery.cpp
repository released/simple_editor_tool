#include "workspace_frame.h"
#include "ui/localization.h"
#include "main_frame.h"
#include "resource.h"
#include <Shlobj.h>
#include <Shellapi.h>
#include <algorithm>
#include <chrono>
using namespace simple_editor::core;
namespace {
class CRecoveryDialog : public CDialog {
public:
    CRecoveryDialog(CWnd* owner,const RecoveryIndex& index) : CDialog(IDD_RECOVERY,owner),index_(index) {}
    std::vector<std::size_t> selected;
protected:
    BOOL OnInitDialog() override {
        CDialog::OnInitDialog(); list_.SubclassDlgItem(2100,this); list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER); list_.InsertColumn(0,L"Draft",LVCFMT_LEFT,210); list_.InsertColumn(1,L"Original path (reference only)",LVCFMT_LEFT,450); list_.InsertColumn(2,L"Bytes",LVCFMT_RIGHT,110);
        for (std::size_t i = 0; i < index_.drafts.size(); ++i) { const auto& item = index_.drafts[i]; const int row = list_.InsertItem(static_cast<int>(i),item.label.c_str()); list_.SetItemText(row,1,item.original_path.empty() ? L"Untitled / merge result" : item.original_path.c_str()); list_.SetItemText(row,2,std::to_wstring(item.size).c_str()); list_.SetItemState(row,LVIS_SELECTED,LVIS_SELECTED); } return TRUE;
    }
    void OnOK() override { selected.clear(); for (int row = list_.GetNextItem(-1,LVNI_SELECTED); row >= 0; row = list_.GetNextItem(row,LVNI_SELECTED)) selected.push_back(row); if (selected.empty()) { UiMessageBox(L"Select one or more recovery copies.",MB_ICONINFORMATION); return; } CDialog::OnOK(); }
    BOOL OnCommand(WPARAM wparam,LPARAM lparam) override { if (LOWORD(wparam) == 2101) { selected.clear(); for (std::size_t i = 0; i < index_.drafts.size(); ++i) selected.push_back(i); EndDialog(IDOK); return TRUE; } if (LOWORD(wparam) == 2102) { EndDialog(2102); return TRUE; } return CDialog::OnCommand(wparam,lparam); }
private:
    const RecoveryIndex& index_; CListCtrl list_;
};
}
void CWorkspaceFrame::InitializeRecovery(bool load) {
    if (recovery_initialized_ || (!published_instance_ && recovery_root_.empty())) return;
    if (recovery_root_.empty()) { PWSTR local = nullptr; if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,KF_FLAG_DEFAULT,nullptr,&local))) return; DWORD session = 0; ProcessIdToSessionId(GetCurrentProcessId(),&session); recovery_root_ = std::filesystem::path(local) / L"SimpleEditorTool" / L"Recovery" / (L"Session-" + std::to_wstring(session)); CoTaskMemFree(local); }
    recovery_initialized_ = true; recovery_last_snapshot_ = GetTickCount64(); SetTimer(4,100,nullptr); if (load) StartRecoveryLoad(false);
}
void CWorkspaceFrame::StopRecovery() { KillTimer(4); recovery_cancel_ = true; if (recovery_io_.valid()) { try { recovery_io_.get(); } catch (...) {} } }
bool CWorkspaceFrame::StartRecoveryLoad(bool manual) {
    if (recovery_io_.valid() || recovery_root_.empty()) return false; const auto root = recovery_root_; recovery_cancel_ = false;
    try { recovery_io_ = std::async(std::launch::async,[root,manual]() { RecoveryJob result; result.kind = 0; result.manual = manual; if (!LoadRecovery(root,&result.index,&result.error)) return result; if (manual && result.index.drafts.empty() && !result.index.previous.empty()) { RecoveryIndex previous; if (LoadPreviousRecovery(result.index,&previous,&result.error)) result.index = std::move(previous); } return result; }); }
    catch (...) { return false; } SetTimer(4,100,nullptr); return true;
}
bool CWorkspaceFrame::StartRecoverySnapshot(bool clean_exit,bool manual) {
    if (recovery_io_.valid() || recovery_root_.empty() || recovery_modal_) return false;
    if (!clean_exit) for (auto* frame : sessions_) if (frame->IsBusy()) { recovery_last_snapshot_ = GetTickCount64(); if (manual) UiMessageBox(L"Wait for editor operations before taking a recovery snapshot.",MB_ICONINFORMATION); return false; }
    auto entries = CaptureWorkspace(); if (clean_exit) { entries.clear(); if (preferences_.auto_restore) for (auto id : exit_order_) { const auto found = exit_workspace_.find(id); if (found != exit_workspace_.end()) entries.push_back(found->second); } }
    std::vector<RecoveryDraft> drafts; std::wstring error; std::uint64_t total = 0;
    if (!clean_exit && (preferences_.recover_drafts || manual)) for (auto* frame : sessions_) {
        if (frame->IsBusy()) { if (manual) UiMessageBox(L"Wait for editor operations before taking a recovery snapshot.",MB_ICONINFORMATION); return false; }
        RecoveryDraft draft; draft.view.page = (std::min)(1,frame->active_tab_); draft.view.encoding = static_cast<int>(frame->requested_encoding_); for (int i = 0; i < 3; ++i) draft.view.zoom[i] = frame->zoom_percent_[i]; draft.view.caret = frame->hex_.CursorOffset(); draft.view.top_row = frame->hex_.TopRow(); draft.view.hex_rows[0] = frame->hex_.BytesPerRow(); draft.view.hex_groups[0] = frame->hex_.Grouping(); draft.view.hex_bases[0] = frame->hex_.BaseAddress();
        if (frame->comparison_session_) {
            if (!frame->advanced_.Dirty()) continue; const auto raw = frame->advanced_.RawOutputText(); draft.native_text = true; draft.original_text = raw; draft.utf8 = CCodeEditor::ToUtf8(TextForEditControl(raw)); draft.original_codec = frame->advanced_.OutputCodec(); draft.label = L"Merge result"; draft.view.page = 1;
        } else {
            if (!frame->document_.IsOpen() || (!frame->document_.IsModified() && !frame->text_dirty_ && !frame->text_.GetModify() && !frame->untitled_dirty_)) continue;
            draft.original_path = frame->untitled_ ? L"" : frame->document_.Path(); draft.label = frame->untitled_ ? frame->untitled_name_ : std::filesystem::path(frame->document_.Path()).filename().wstring();
            if (!frame->text_stale_ && frame->text_editable_ && (frame->text_dirty_ || frame->text_.GetModify())) { draft.native_text = true; draft.utf8 = frame->text_.Utf8(); draft.original_text = frame->text_original_; draft.original_codec = frame->text_codec_; }
            else { draft.document = frame->document_.Snapshot(&error); if (!draft.document) { if (manual) UiMessageBox(error.c_str(),MB_ICONWARNING); return false; } }
        }
        const auto size = draft.native_text ? draft.utf8.size() : draft.document->Size(); total += size; if (size > 64ULL * 1024 * 1024 || total > 256ULL * 1024 * 1024) { UiMessageBox(L"Recovery supports 64 MiB per draft and 256 MiB total. Oversized drafts still require explicit Save; the prior recovery snapshot is retained.",MB_ICONWARNING); recovery_paused_ = true; return false; } drafts.push_back(std::move(draft));
    }
    const auto root = recovery_root_; recovery_cancel_ = false; recovery_progress_ = 0;
    try { recovery_io_ = std::async(std::launch::async,[this,root,entries = std::move(entries),drafts = std::move(drafts),clean_exit,manual]() { RecoveryJob result; result.kind = clean_exit ? 4 : 1; result.manual = manual; SaveControl control; control.cancelled = &recovery_cancel_; control.progress = [this](unsigned p) { recovery_progress_ = p; }; result.saved = SaveRecovery(root,entries,drafts,clean_exit,control); result.error = result.saved.error; return result; }); }
    catch (...) { return false; } recovery_last_snapshot_ = GetTickCount64(); SetTimer(4,100,nullptr); return true;
}
bool CWorkspaceFrame::StartRecoveryRead(const RecoveryIndex& index,const std::vector<std::size_t>& items) {
    if (recovery_io_.valid()) return false; recovery_cancel_ = false;
    try { recovery_io_ = std::async(std::launch::async,[this,index,items]() { RecoveryJob result; result.kind = 2; result.index = index; SaveControl control; control.cancelled = &recovery_cancel_; for (auto i : items) { std::vector<std::uint8_t> bytes; if (!ReadRecoveryDraft(index,i,control,&bytes,&result.error)) { result.payloads.clear(); return result; } result.payloads.push_back({i,std::move(bytes)}); } return result; }); } catch (...) { return false; } SetTimer(4,100,nullptr); return true;
}
bool CWorkspaceFrame::StartRecoveryClear() {
    if (recovery_io_.valid() || recovery_root_.empty()) return false;
    const auto text = L"Clear the validated current and previous recovery snapshots?\r\nOriginal files are unchanged. Unknown or modified copies are retained.\r\n\r\n" + recovery_root_.wstring(); recovery_modal_ = true; const auto answer = UiMessageBox(text.c_str(),MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION); recovery_modal_ = false; if (answer != IDYES) return false;
    const auto root = recovery_root_; try { recovery_io_ = std::async(std::launch::async,[root]() { RecoveryJob result; result.kind = 3; ClearRecovery(root,&result.error); return result; }); } catch (...) { return false; } SetTimer(4,100,nullptr); return true;
}
void CWorkspaceFrame::OfferRecovery(const RecoveryIndex& index) {
    if (index.drafts.empty()) { UiMessageBox(L"No draft recovery copies are available.",MB_ICONINFORMATION); return; }
    recovery_paused_ = true; CRecoveryDialog dialog(this,index); recovery_modal_ = true; const auto answer = dialog.DoModal(); recovery_modal_ = false;
    if (answer == IDOK) StartRecoveryRead(index,dialog.selected); else if (answer == 2102) StartRecoveryClear();
}
void CWorkspaceFrame::TickRecovery() {
    if (!recovery_io_.valid()) { if (recovery_initialized_ && !recovery_paused_ && !recovery_modal_ && !closing_all_ && (preferences_.auto_restore || preferences_.recover_drafts) && GetTickCount64() - recovery_last_snapshot_ >= static_cast<ULONGLONG>(preferences_.snapshot_seconds) * 1000) StartRecoverySnapshot(false); return; }
    if (recovery_io_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
    RecoveryJob result; try { result = recovery_io_.get(); } catch (...) { result.error = L"Recovery operation failed: insufficient memory or I/O error."; }
    const bool close = recovery_close_pending_; recovery_close_pending_ = false;
    if (!result.error.empty()) { recovery_paused_ = true; UiMessageBox(result.error.c_str(),MB_ICONWARNING); if (close) { if (sessions_.empty()) CFrameWnd::OnClose(); else OnClose(); } return; }
    if (result.kind == 0) {
        if (!close) {
            const auto active = active_session_ >= 0 ? sessions_[active_session_]->workspace_session_id_ : 0;
            if (!result.manual && preferences_.auto_restore) { RestoreWorkspace(result.index.workspace); if (active) for (std::size_t i = 0; i < sessions_.size(); ++i) if (sessions_[i]->workspace_session_id_ == active) { ActivateSession(static_cast<int>(i)); break; } }
            if (result.manual || (!result.index.clean_exit && preferences_.recover_drafts && !result.index.drafts.empty())) OfferRecovery(result.index);
        }
    } else if (result.kind == 2) {
        for (auto& payload : result.payloads) {
            const auto& item = result.index.drafts[payload.first]; if (!NewDocument(payload.second,static_cast<TextEncoding>(item.view.encoding),item.view.page)) break; auto* frame = sessions_[active_session_]; frame->untitled_dirty_ = true; frame->untitled_name_ = L"Recovered " + (item.label.empty() ? std::to_wstring(frame->workspace_session_id_) : item.label); untitled_aliases_.push_back(frame->untitled_name_); frame->SetWindowTextW((frame->untitled_name_ + L" * - Simple Editor Tool").c_str()); frame->hex_.SetLayout(item.view.hex_rows[0],item.view.hex_groups[0],item.view.hex_bases[0]); frame->hex_.GoTo(item.view.caret); frame->hex_.SetTopRow(item.view.top_row); for (int i = 0; i < 3; ++i) frame->SetTabZoom(i,item.view.zoom[i]);
        } recovery_paused_ = false; RefreshLabels();
    } else if (result.kind == 3) { recovery_paused_ = false; recovery_last_snapshot_ = GetTickCount64(); }
    else if (result.kind == 1 || result.kind == 4) {
        if (!result.saved.committed && !result.saved.cancelled) UiMessageBox(L"The recovery snapshot was not committed. Existing copies were retained.",MB_ICONWARNING);
        if (!result.saved.warning.empty()) { recovery_paused_ = true; UiMessageBox(result.saved.warning.c_str(),MB_ICONWARNING); }
        if (result.kind == 4 && close) { CFrameWnd::OnClose(); return; }
        if (result.manual && result.saved.committed) UiMessageBox(L"Workspace/draft recovery copies saved. Original files were not modified.",MB_ICONINFORMATION);
    }
    if (close) OnClose();
}
void CWorkspaceFrame::OnRecoveryCommand(UINT id) {
    if (recovery_io_.valid() || recovery_modal_) { UiMessageBox(L"Wait for the current recovery operation.",MB_ICONINFORMATION); return; }
    if (!recovery_initialized_) { if (recovery_root_.empty() && !published_instance_) return; InitializeRecovery(false); }
    if (id == ID_WORKSPACE_RECOVER_DRAFTS) StartRecoveryLoad(true);
    else if (id == ID_WORKSPACE_SNAPSHOT) { recovery_paused_ = false; StartRecoverySnapshot(false,true); }
    else if (id == ID_WORKSPACE_CLEAR_RECOVERY) StartRecoveryClear();
    else if (id == ID_WORKSPACE_BROWSE_RECOVERY) { FileStamp stamp; std::wstring error; if (InspectFile(recovery_root_,&stamp,&error) && stamp.exists && stamp.directory) ShellExecuteW(GetSafeHwnd(),L"open",recovery_root_.c_str(),nullptr,nullptr,SW_SHOWNORMAL); }
}

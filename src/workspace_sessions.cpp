#include "workspace_frame.h"
#include "ui/localization.h"
#include "main_frame.h"
#include <chrono>
#include <filesystem>

using namespace simple_editor::core;
namespace {
std::wstring Absolute(const std::wstring& path) { if (path.empty()) return path; std::error_code error; const auto full = std::filesystem::absolute(path,error).lexically_normal(); return error ? path : full.wstring(); }
bool Same(const std::wstring& a, const std::wstring& b) { return CompareStringOrdinal(Absolute(a).c_str(),-1,Absolute(b).c_str(),-1,TRUE) == CSTR_EQUAL; }
}
bool CWorkspaceFrame::CaptureSession(CMainFrame* session,WorkspaceEntry* output) const {
        WorkspaceEntry entry; entry.comparison = session->comparison_session_; entry.active = active_session_ >= 0 && static_cast<std::size_t>(active_session_) < sessions_.size() && sessions_[active_session_] == session;
        if (session->untitled_) return false;
        if (session->displayed_mode_ == 8 || session->advanced_.HasPastedInputs()) return false;
        if (entry.comparison) { CString left,right,base; session->compare_left_path_.GetWindowTextW(left); session->compare_right_path_.GetWindowTextW(right); session->compare_base_path_.GetWindowTextW(base);
            bool transient = false; for (const auto& name : untitled_aliases_) for (const auto* value : {&left,&right,&base}) if (*value == name.c_str()) transient = true; if (transient) return false;
            entry.left = Absolute(left.GetString()); entry.right = Absolute(right.GetString()); entry.base = Absolute(base.GetString()); entry.mode = session->displayed_mode_; }
        else entry.left = session->document_.Path();
        entry.page = session->active_tab_; entry.encoding = static_cast<int>(session->requested_encoding_);
        entry.dual_view = session->dual_view_; entry.syntax_mode = session->syntax_mode_;
        entry.crc_profile = session->crc_profile_;
        entry.hex_bookmarks = session->bookmarks_.Save(!session->document_.IsModified() && !session->text_dirty_ && !session->text_.GetModify() && session->bookmarks_.Matches(session->document_));
        entry.binary_region = session->compare_region_spec_; const CHexView* views[] = {&session->hex_,&session->compare_left_,&session->compare_right_}; for (int j = 0; j < 3; ++j) { entry.hex_decoding[j] = static_cast<unsigned>(views[j]->Decoding()); entry.hex_rows[j] = views[j]->BytesPerRow(); entry.hex_groups[j] = views[j]->Grouping(); entry.hex_bases[j] = views[j]->BaseAddress(); }
        for (int j = 0; j < 3; ++j) entry.zoom[j] = session->zoom_percent_[j]; entry.caret = session->hex_.CursorOffset(); entry.top_row = session->hex_.TopRow();
        session->advanced_.CapturePreferences(entry); *output = std::move(entry); return true;
}
std::vector<WorkspaceEntry> CWorkspaceFrame::CaptureWorkspace() const {
    std::vector<WorkspaceEntry> entries; for (auto* frame : sessions_) { WorkspaceEntry entry; if (CaptureSession(frame,&entry)) entries.push_back(std::move(entry)); }
    return entries;
}
void CWorkspaceFrame::RestoreWorkspace(const std::vector<WorkspaceEntry>& entries) {
    int active = -1; std::size_t missing = 0;
    for (const auto& entry : entries) {
        bool existing = false;
        if (entry.comparison) {
            for (std::size_t i = 0; i < sessions_.size(); ++i) if (sessions_[i]->comparison_session_ && sessions_[i]->displayed_mode_ == entry.mode) {
                CString left,right,base; sessions_[i]->compare_left_path_.GetWindowTextW(left); sessions_[i]->compare_right_path_.GetWindowTextW(right); sessions_[i]->compare_base_path_.GetWindowTextW(base);
                if (Same(left.GetString(),entry.left) && Same(right.GetString(),entry.right) && Same(base.GetString(),entry.base)) { ActivateSession(static_cast<int>(i)); existing = true; break; }
            }
            if (!existing) NewComparisonInputs(entry.left,entry.right,entry.base,entry.mode,false);
        } else {
            for (auto* session : sessions_) if (!session->comparison_session_ && Same(session->document_.Path(),entry.left)) existing = true;
            if (!OpenPath(entry.left,true)) { ++missing; continue; }
        }
        if (active_session_ < 0) continue;
        if (entry.active) active = active_session_;
        if (existing) continue; // Existing drafts, undo and view settings take precedence.
        auto* session = sessions_[active_session_]; session->requested_encoding_ = static_cast<TextEncoding>(entry.encoding); session->text_stale_ = true;
        session->advanced_.RestorePreferences(entry); session->syntax_mode_ = entry.syntax_mode; if (!session->comparison_session_) session->ConfigureFileSyntax();
        session->compare_region_spec_ = entry.binary_region; if (!entry.binary_region.empty()) { std::wstring error; ParseCompareRegionSpec(entry.binary_region,&session->compare_region_,&error); } CHexView* views[] = {&session->hex_,&session->compare_left_,&session->compare_right_}; for (int j = 0; j < 3; ++j) { views[j]->SetDecoding(static_cast<HexDecoding>(entry.hex_decoding[j])); if (!views[j]->SetLayout(entry.hex_rows[j],entry.hex_groups[j],entry.hex_bases[j])) views[j]->SetLayout(entry.hex_rows[j],entry.hex_groups[j],0); }
        for (int j = 0; j < 3; ++j) session->SetTabZoom(j,entry.zoom[j]);
        std::wstring bookmark_error; session->bookmarks_.Load(entry.hex_bookmarks,session->document_.Size(),&bookmark_error); if (!session->bookmarks_.Matches(session->document_)) session->bookmarks_.stale = true;
        session->crc_profile_ = entry.crc_profile;
        if (entry.dual_view && !session->dual_view_) session->ToggleTextClone();
        session->hex_.GoTo(entry.caret); session->hex_.SetTopRow(entry.top_row); session->ShowPage(entry.page);
    }
    if (active >= 0 && active < static_cast<int>(sessions_.size())) ActivateSession(active);
    if (missing) UiMessageBox((std::to_wstring(missing) + L" workspace file(s) could not be opened. Existing tabs and edits were retained.").c_str(),MB_ICONWARNING);
}
void CWorkspaceFrame::OnSaveWorkspace() {
    if (workspace_io_.valid() || workspace_dialog_) return;
    bool dirty = false; for (auto* session : sessions_) dirty = dirty || session->untitled_ || session->document_.IsModified() || session->text_dirty_ || session->advanced_.Dirty();
    if (dirty) { workspace_dialog_ = true; const int answer = UiMessageBox(L"Workspace files store paths and view settings only. Unsaved file/merge edits are not included. Save paths/settings only?",MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING); workspace_dialog_ = false; if (answer != IDYES) return; }
    CFileDialog dialog(FALSE,L"ini",L"workspace.ini",OFN_OVERWRITEPROMPT,L"Workspace settings (*.ini)|*.ini||",this);
    workspace_dialog_ = true; const INT_PTR selected = dialog.DoModal(); workspace_dialog_ = false; if (selected != IDOK) return;
    const std::wstring path = dialog.GetPathName().GetString(); auto entries = CaptureWorkspace();
    if (!LockPaths({path},true,this)) { UiMessageBox(L"The workspace output file has unsaved edits or is busy.",MB_ICONWARNING); return; }
    workspace_io_path_ = path; workspace_io_writes_ = true;
    try { workspace_io_ = std::async(std::launch::async,[path,entries = std::move(entries)]() { WorkspaceIO result; result.path = path; result.saved = SaveWorkspaceFile(path,entries,&result.error); if (!result.saved && result.error.empty()) result.error = L"Could not save workspace settings."; return result; }); }
    catch (const std::exception&) { LockPaths({path},false,this); workspace_io_writes_ = false; UiMessageBox(L"Could not start workspace save.",MB_ICONERROR); return; }
    SetTimer(2,50,nullptr);
}
void CWorkspaceFrame::OnLoadWorkspace() {
    if (workspace_io_.valid() || workspace_dialog_) return;
    CFileDialog dialog(TRUE,L"ini",nullptr,OFN_FILEMUSTEXIST,L"Workspace settings (*.ini)|*.ini||",this);
    workspace_dialog_ = true; const INT_PTR selected = dialog.DoModal(); workspace_dialog_ = false; if (selected != IDOK) return;
    const std::wstring path = dialog.GetPathName().GetString();
    workspace_io_path_ = path; workspace_io_writes_ = false;
    try { workspace_io_ = std::async(std::launch::async,[path]() { WorkspaceIO result; result.loading = true; result.path = path; if (!LoadWorkspaceFile(path,&result.entries,&result.error) && result.error.empty()) result.error = L"Could not load workspace settings."; return result; }); }
    catch (const std::exception&) { UiMessageBox(L"Could not start workspace load.",MB_ICONERROR); return; } SetTimer(2,50,nullptr);
}
void CWorkspaceFrame::OnTimer(UINT_PTR id) {
    if (id == 5) { TickOpenSearch(); return; }
    if (id == 3) { TickPreferences(); return; }
    if (id == 4) { TickRecovery(); return; }
    if (id != 2 || !workspace_io_.valid()) { CFrameWnd::OnTimer(id); return; }
    if (workspace_io_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
    KillTimer(2); WorkspaceIO result; result.loading = !workspace_io_writes_;
    try { result = workspace_io_.get(); } catch (const std::exception&) { result.error = L"Workspace operation failed."; }
    if (workspace_io_writes_) LockPaths({workspace_io_path_},false,this); workspace_io_writes_ = false; workspace_io_path_.clear();
    if (!result.error.empty()) { workspace_close_pending_ = false; UiMessageBox(result.error.c_str(),MB_ICONERROR); return; }
    if (result.loading) RestoreWorkspace(result.entries);
    if (active_session_ >= 0) sessions_[active_session_]->status_.SetPaneText(0,result.loading ? L"Workspace paths/settings loaded; existing tabs retained. Compare sessions are ready to run." : L"Workspace paths/settings saved. File edits must be saved separately.");
    if (workspace_close_pending_) { workspace_close_pending_ = false; PostMessage(WM_CLOSE); }
}

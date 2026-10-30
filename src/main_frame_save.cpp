#include "main_frame.h"
#include "resource.h"
#include "workspace_frame.h"

bool CMainFrame::IsSaving() const { return job_.valid() && job_kind_ == EditorJobKind::Save; }

void CMainFrame::SetSaveReadOnly(bool saving) {
    hex_.SetReadOnly(saving || comparison_session_ || user_read_only_);
    if (text_.GetSafeHwnd()) text_.SetReadOnly(saving || comparison_session_ || user_read_only_ || !text_editable_);
    if (text_clone_.GetSafeHwnd()) text_clone_.SetReadOnly(text_.Call(SCI_GETREADONLY) != 0);
    DrawMenuBar();
    SetComparisonControlsEnabled(!saving);
}

bool CMainFrame::StartSave(const std::wstring& destination, AfterSave next, const std::wstring& next_path) {
    if (MutationBlocked()) return false;
    if (IsBusy()) { status_.SetPaneText(0, L"An operation is running. Cancel it or wait before saving."); return false; }
    if (!document_.IsOpen() || !ApplyTextEdits()) return false;
    if (workspace_ && workspace_->IsTemporaryPath(destination)) { status_.SetPaneText(0,L"Choose a user file path; internal document backing files cannot be saved over."); return false; }
    std::wstring error;
    try {
        auto snapshot = document_.Snapshot(&error);
        if (!snapshot) { status_.SetPaneText(0, error.c_str()); return false; }
        job_locked_paths_ = {destination}; if (workspace_ && !workspace_->LockPaths(job_locked_paths_,true,&job_)) { job_locked_paths_.clear(); status_.SetPaneText(0,L"The destination is busy or has unsaved edits in another tab."); return false; } job_locks_held_ = workspace_ != nullptr;
        job_revision_ = revision_; job_kind_ = EditorJobKind::Save; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX;
        after_save_ = next; after_save_path_ = next_path;
        ClearComparison(); SetSaveReadOnly(true);
        if (search_dialog_.GetSafeHwnd() && search_dialog_.IsWindowVisible()) search_dialog_.ShowWindow(SW_HIDE);
        job_ = std::async(std::launch::async, [this, snapshot = std::move(snapshot), destination]() mutable {
            EditorJobResult result; result.kind = EditorJobKind::Save;
            simple_editor::core::SaveControl control;
            control.cancelled = &cancelled_; control.progress = [this](unsigned int value) { progress_ = value; };
            result.saved = snapshot->Save(destination, &result.error, control);
            if (result.saved) result.document = std::move(snapshot);
            return result;
        });
    } catch (const std::exception&) {
        after_save_ = AfterSave::None; after_save_path_.clear(); ReleaseJobLocks(); SetSaveReadOnly(false);
        status_.SetPaneText(0, L"Could not start saving. The document and destination are unchanged."); return false;
    }
    status_.SetPaneText(0, L"Saving... 0%. Esc cancels before file replacement."); return true;
}

bool CMainFrame::SaveCurrent(AfterSave next,const std::wstring& next_path,bool choose) {
    if (IsBusy() || !document_.IsOpen()) return false;
    if (!choose && !untitled_) return StartSave(document_.Path(),next,next_path);
    const auto initial = untitled_ ? std::filesystem::path(untitled_name_).has_extension() ? untitled_name_ : untitled_name_ + L".txt" : document_.Path();
    CFileDialog dialog(FALSE,untitled_ ? L"txt" : nullptr,initial.c_str(),OFN_OVERWRITEPROMPT,L"All files (*.*)|*.*||",this);
    path_dialog_ = true; const auto accepted = dialog.DoModal(); path_dialog_ = false;
    return accepted == IDOK && StartSave(dialog.GetPathName().GetString(),next,next_path);
}
void CMainFrame::OnSave() { if (AdvancedActive()) { if (!advanced_.SaveFocusedSide()) advanced_.SaveResult(); return; } SaveCurrent(); }
void CMainFrame::OnSaveAs() {
    if (AdvancedActive()) { advanced_.SaveResult(); return; }
    if (IsBusy() || !document_.IsOpen()) { status_.SetPaneText(0, L"Wait for the current operation before Save As."); return; }
    SaveCurrent(AfterSave::None,L"",true);
}

void CMainFrame::OnUpdateSaveCommand(CCmdUI* command) {
    const bool file_save = command->m_nID == ID_FILE_SAVE || command->m_nID == ID_FILE_SAVE_AS;
    command->Enable(AdvancedActive() ? !IsBusy() && (displayed_mode_ == 2 || displayed_mode_ == 3 || displayed_mode_ == 8) : !MutationBlocked() && document_.IsOpen() && (file_save ? !IsBusy() : !IsSaving()));
}

void CMainFrame::FinishSave(EditorJobResult result) {
    ReleaseJobLocks();
    const auto next = after_save_; const auto next_path = std::move(after_save_path_);
    after_save_ = AfterSave::None; after_save_path_.clear();
    SetSaveReadOnly(false);
    if (!result.saved || !result.document) {
        if (workspace_) workspace_->CancelCloseAll();
        const std::wstring message = result.error.empty() ? L"Save failed. Edits are retained." : result.error;
        status_.SetPaneText(0, message.c_str());
        return;
    }
    if (revision_ != job_revision_) {
        if (workspace_) workspace_->CancelCloseAll();
        status_.SetPaneText(0, L"The snapshot was saved. Newer edits are retained; use Save As or Reload before another save.");
        return;
    }
    document_.Swap(*result.document); ++revision_; modified_ranges_.clear(); modified_revision_ = UINT64_MAX; hex_.Invalidate(); text_dirty_ = false; text_.SetModify(FALSE); untitled_ = false; untitled_dirty_ = false; untitled_name_.clear(); ConfigureFileSyntax();
    SetWindowTextW((document_.Path() + L" - Simple Editor Tool").c_str()); hex_.Invalidate();
    status_.SetPaneText(0, cancelled_ ? L"Saved. Cancellation arrived after file replacement." : L"Saved.");
    if (workspace_) { workspace_->RememberRecent(document_.Path()); workspace_->RefreshLabels(); }
    switch (next) {
    case AfterSave::Open: OpenPath(next_path); return;
    case AfterSave::OpenDialog: OnFileOpen(); return;
    case AfterSave::Reload: OnReload(); return;
    case AfterSave::Close: saved_close_pending_ = true; PostMessage(WM_EDITOR_SAVED_CLOSE); return;
    case AfterSave::CompareInsertion: OnCompare(); return;
    case AfterSave::CompareEqual: OnCompareEqual(); return;
    default: return;
    }
}

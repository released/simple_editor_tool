#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"

bool CMainFrame::MutationBlocked() const { return user_read_only_ || IsSaving() || comparison_session_ || (workspace_ && workspace_->DocumentLocked(document_)); }
bool CMainFrame::EnsureAdvanced() {
    if (advanced_.GetSafeHwnd()) return true;
    const auto name = AfxRegisterWndClass(CS_DBLCLKS,::LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),nullptr);
    if (!advanced_.Create(name,nullptr,WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,CRect(),this,526)) return false;
    advanced_.MoveWindow(compare_view_rect_);
    advanced_.SetWindowPos(&wndTop,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    advanced_.SetAppearance(text_face_,content_font_tenths_,content_theme_);
    advanced_.SetZoom(zoom_percent_[2]); return true;
}
void CMainFrame::InitializeAdvanced() {
    advanced_.apply_side_copy = [this](const simple_editor::core::TextSideCopy& copy,std::wstring* error) { return workspace_ && workspace_->ApplyComparisonCopy(copy,error); };
    advanced_.undo_side = [this](const std::wstring& path,bool redo) { return workspace_ && workspace_->ComparisonSideAction(path,redo ? 2 : 1); };
    advanced_.save_side = [this](const std::wstring& path) { return workspace_ && workspace_->ComparisonSideAction(path,0); };
    advanced_.compare_again = [this]() { RunComparison(false); };
    advanced_.no_differences = [this]() { NotifyNoDifferences(); };
    advanced_.report = [this](const std::wstring& text) { if (status_.GetSafeHwnd()) status_.SetPaneText(0,text.c_str()); };
    advanced_.wheel_zoom = [this](short delta) { ZoomWheel(2,delta); };
    advanced_.changed = [this]() { if (workspace_) workspace_->RefreshLabels(); };
    advanced_.busy_changed = [this](bool busy) { SetComparisonControlsEnabled(!busy); };
    advanced_.lock_paths = [this](const std::vector<std::wstring>& paths, bool acquire, const std::vector<simple_editor::core::FileStamp>& stamps) { return !workspace_ || workspace_->LockPaths(paths,acquire,&advanced_,stamps); };
    advanced_.saved_close = [this](bool success) { if (success) { saved_close_pending_ = true; PostMessage(WM_EDITOR_SAVED_CLOSE); } else if (workspace_) workspace_->CancelCloseAll(); };
    advanced_.open_pair = [this](const std::wstring& left, const std::wstring& right, int mode) {
        if (!workspace_) return;
        workspace_->OpenFolderPair(this,left,right,mode);
    };
}
void CMainFrame::OnCompareModeChanged() {
    const int requested = compare_mode_.GetCurSel();
    if (requested < 0) return;
    if (IsBusy() || !advanced_.CanReset()) { compare_mode_.SetCurSel(displayed_mode_); return; }
    if (requested == 8 && workspace_ && !comparison_session_) {
        compare_mode_.SetCurSel(displayed_mode_);
        workspace_->NewComparisonInputs(L"",L"",L"",8,false); return;
    }
    if (requested >= 2 && !EnsureAdvanced()) { compare_mode_.SetCurSel(displayed_mode_); status_.SetPaneText(0,L"Could not create the comparison view."); return; }
    if (requested >= 2 && !advanced_.SetPastedInputs(requested == 8)) { compare_mode_.SetCurSel(displayed_mode_); return; }
    if (requested < 2 && advanced_.GetSafeHwnd()) advanced_.SetPastedInputs(false);
    displayed_mode_ = requested;
    if (requested == 8 && comparison_session_) {
        document_.Close(); ++revision_; hex_.SetDocument(&document_); text_stale_ = true;
        SetWindowTextW(L"Pasted text comparison - Simple Editor Tool");
    }
    CString left,right; compare_left_path_.GetWindowTextW(left); compare_right_path_.GetWindowTextW(right);
    SetComparisonPaths(left.GetString(),right.GetString(),requested);
    if (requested >= 2) advanced_.Configure(requested == 8 ? 2 : requested);
    ClearComparison();
    compare_left_label_.SetWindowTextW(requested == 4 ? L"Left folder" : L"Left file"); compare_right_label_.SetWindowTextW(requested == 4 ? L"Right folder" : L"Right file");
    Layout(); ShowPage(2);
    if (workspace_) workspace_->RefreshLabels();
}
void CMainFrame::OnCompareBrowseBase() {
    if (IsBusy()) return; CString initial; compare_base_path_.GetWindowTextW(initial);
    CFileDialog dialog(TRUE,nullptr,initial.IsEmpty() ? nullptr : initial.GetString(),OFN_FILEMUSTEXIST | OFN_EXPLORER,L"All files (*.*)|*.*||",this);
    path_dialog_ = true; const INT_PTR answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK) { SetPathComboText(compare_base_path_,dialog.GetPathName().GetString()); if (workspace_) workspace_->RememberRecent(dialog.GetPathName().GetString()); OnComparePathChanged(); RefreshComparisonPaths(); }
}

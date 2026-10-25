#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"

bool CMainFrame::MutationBlocked() const { return user_read_only_ || IsSaving() || comparison_session_ || (workspace_ && workspace_->DocumentLocked(document_)); }
bool CMainFrame::EnsureAdvanced() {
    if (advanced_.GetSafeHwnd()) return true;
    const auto name = AfxRegisterWndClass(CS_DBLCLKS,::LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),nullptr);
    if (!advanced_.Create(name,nullptr,WS_CHILD | WS_CLIPCHILDREN,CRect(),this,526)) return false;
    advanced_.MoveWindow(compare_view_rect_);
    advanced_.SetAppearance(text_face_,content_font_tenths_,content_theme_);
    advanced_.SetZoom(zoom_percent_[2]); return true;
}
void CMainFrame::InitializeAdvanced() {
    advanced_.report = [this](const std::wstring& text) { if (status_.GetSafeHwnd()) status_.SetPaneText(0,text.c_str()); };
    advanced_.wheel_zoom = [this](short delta) { ZoomWheel(2,delta); };
    advanced_.changed = [this]() { if (workspace_) workspace_->RefreshLabels(); };
    advanced_.busy_changed = [this](bool busy) { SetComparisonControlsEnabled(!busy); };
    advanced_.lock_paths = [this](const std::vector<std::wstring>& paths, bool acquire, const std::vector<simple_editor::core::FileStamp>& stamps) { return !workspace_ || workspace_->LockPaths(paths,acquire,&advanced_,stamps); };
    advanced_.saved_close = [this](bool success) { if (success) { saved_close_pending_ = true; PostMessage(WM_EDITOR_SAVED_CLOSE); } else if (workspace_) workspace_->CancelCloseAll(); };
    advanced_.open_pair = [this](const std::wstring& left, const std::wstring& right, int mode) {
        if (!workspace_) return;
        if (left.empty() || right.empty()) workspace_->OpenPath(left.empty() ? right : left);
        else workspace_->NewComparisonInputs(left,right,L"",mode,true);
    };
}
void CMainFrame::OnCompareModeChanged() {
    const int requested = compare_mode_.GetCurSel();
    if (requested < 0) return;
    if (IsBusy() || !advanced_.CanReset()) { compare_mode_.SetCurSel(displayed_mode_); return; }
    if (requested >= 2 && !EnsureAdvanced()) { compare_mode_.SetCurSel(displayed_mode_); status_.SetPaneText(0,L"Could not create the comparison view."); return; }
    displayed_mode_ = requested;
    CString left,right; compare_left_path_.GetWindowTextW(left); compare_right_path_.GetWindowTextW(right);
    SetComparisonPaths(left.GetString(),right.GetString(),requested != 1); compare_mode_.SetCurSel(requested);
    if (requested >= 2) advanced_.Configure(requested);
    ClearComparison();
    compare_left_label_.SetWindowTextW(requested == 4 ? L"Left folder" : L"Left file"); compare_right_label_.SetWindowTextW(requested == 4 ? L"Right folder" : L"Right file");
    Layout(); ShowPage(2);
}
void CMainFrame::OnCompareBrowseBase() {
    if (IsBusy()) return; CString initial; compare_base_path_.GetWindowTextW(initial);
    CFileDialog dialog(TRUE,nullptr,initial.IsEmpty() ? nullptr : initial.GetString(),OFN_FILEMUSTEXIST | OFN_EXPLORER,L"All files (*.*)|*.*||",this);
    path_dialog_ = true; const INT_PTR answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK) { SetPathComboText(compare_base_path_,dialog.GetPathName().GetString()); OnComparePathChanged(); }
}

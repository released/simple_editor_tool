#include "main_frame.h"
#include "workspace_frame.h"
#include "core/manual_alignment.h"
#include <filesystem>

void CMainFrame::SetComparisonPaths(const std::wstring& left, const std::wstring& right, bool insertion_aware) {
    suppress_compare_path_change_ = true;
    compare_left_path_.ResetContent(); compare_right_path_.ResetContent();
    if (workspace_) for (const auto& path : displayed_mode_ == 4 ? workspace_->FolderPaths() : workspace_->OpenPaths()) { compare_left_path_.AddString(path.c_str()); compare_right_path_.AddString(path.c_str()); }
    SetPathComboText(compare_left_path_,left); SetPathComboText(compare_right_path_,right);
    compare_mode_.SetCurSel(insertion_aware ? 0 : 1); suppress_compare_path_change_ = false;
}
void CMainFrame::BrowseComparisonPath(bool left) {
    if (IsBusy()) return;
    CComboBox& box = left ? compare_left_path_ : compare_right_path_; CString initial; box.GetWindowTextW(initial);
    if (displayed_mode_ == 4) {
        CFolderPickerDialog folder(initial.IsEmpty() ? nullptr : initial.GetString(),0,this); path_dialog_ = true; const INT_PTR answer = folder.DoModal(); path_dialog_ = false;
        if (answer == IDOK) { SetPathComboText(box,folder.GetPathName().GetString()); OnComparePathChanged(); } return;
    }
    CFileDialog dialog(TRUE, nullptr, initial.IsEmpty() ? nullptr : initial.GetString(),
                       OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER, L"All files (*.*)|*.*||", this);
    dialog.m_ofn.lpstrTitle = left ? L"Select left comparison file" : L"Select right comparison file";
    path_dialog_ = true; const INT_PTR answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK) { SetPathComboText(box,dialog.GetPathName().GetString()); OnComparePathChanged(); }
}
void CMainFrame::OnCompareBrowseLeft() { BrowseComparisonPath(true); }
void CMainFrame::OnCompareBrowseRight() { BrowseComparisonPath(false); }
void CMainFrame::OnCompareSwap() {
    if (IsBusy()) return;
    if (displayed_mode_ == 8) { advanced_.SwapPastedInputs(); return; }
    if (displayed_mode_ == 2 || displayed_mode_ == 3 || displayed_mode_ >= 5) {
        simple_editor::core::WorkspaceEntry settings; advanced_.CapturePreferences(settings);
        std::swap(settings.advanced_encoding,settings.advanced_encoding_right);
        if (displayed_mode_ == 2) {
            simple_editor::core::LineAnchors anchors; std::wstring error;
            if (simple_editor::core::ParseLineAnchors(settings.manual_anchors,&anchors,&error)) { settings.manual_anchors.clear(); for (const auto& pair : anchors) settings.manual_anchors += std::to_wstring(pair.second + 1) + L"," + std::to_wstring(pair.first + 1) + L"\r\n"; }
        }
        advanced_.RestorePreferences(settings);
    }
    CString left, right; compare_left_path_.GetWindowTextW(left); compare_right_path_.GetWindowTextW(right);
    suppress_compare_path_change_ = true; SetPathComboText(compare_left_path_,right.GetString()); SetPathComboText(compare_right_path_,left.GetString());
    suppress_compare_path_change_ = false; OnComparePathChanged();
}
void CMainFrame::OnComparePathChanged() {
    if (suppress_compare_path_change_ || IsBusy()) return;
    if (displayed_mode_ >= 2) advanced_.InvalidateInputs();
    ClearComparison(); status_.SetPaneText(0, L"Select the left and right files, then press Compare. Open editor tabs are retained.");
    if (comparison_session_) {
        document_.Close(); ++revision_; hex_.SetDocument(&document_); text_stale_ = true;
        SetWindowTextW(L"Comparison inputs changed - Simple Editor Tool");
    }
    if (workspace_) workspace_->RefreshLabels();
}
void CMainFrame::SetComparisonControlsEnabled(bool enabled) {
    for (CWnd* control : {static_cast<CWnd*>(&compare_left_path_), static_cast<CWnd*>(&compare_right_path_), static_cast<CWnd*>(&compare_browse_left_), static_cast<CWnd*>(&compare_browse_right_), static_cast<CWnd*>(&compare_mode_), static_cast<CWnd*>(&compare_run_), static_cast<CWnd*>(&compare_swap_)}) {
        if (control->GetSafeHwnd()) control->EnableWindow(enabled);
    }
    if (compare_base_path_.GetSafeHwnd()) compare_base_path_.EnableWindow(enabled);
    if (compare_browse_base_.GetSafeHwnd()) compare_browse_base_.EnableWindow(enabled);
}
void CMainFrame::OnCompareRun() {
    if (IsBusy()) return;
    if (displayed_mode_ == 8) {
        SetWindowTextW(L"Pasted text comparison - Simple Editor Tool");
        advanced_.StartPastedText(); if (workspace_) workspace_->RefreshLabels(); return;
    }
    CString left, right; compare_left_path_.GetWindowTextW(left); compare_right_path_.GetWindowTextW(right);
    left.Trim(); right.Trim();
    if (left.IsEmpty() || right.IsEmpty()) { status_.SetPaneText(0, L"Choose both a left file and a right file."); return; }
    if (workspace_ && !comparison_session_) { CString base; compare_base_path_.GetWindowTextW(base); workspace_->NewComparisonInputs(left.GetString(),right.GetString(),base.GetString(),displayed_mode_,true); return; }
    if (displayed_mode_ >= 2) {
        const auto left_name = std::filesystem::path(left.GetString()).filename().wstring(), right_name = std::filesystem::path(right.GetString()).filename().wstring();
        SetWindowTextW((left_name + L" vs " + right_name + L" - Simple Editor Tool").c_str()); if (workspace_) workspace_->RefreshLabels();
        if (displayed_mode_ == 4) { advanced_.StartFolders(left.GetString(),right.GetString()); return; }
        CString base; compare_base_path_.GetWindowTextW(base); if (displayed_mode_ == 3 && base.IsEmpty()) { status_.SetPaneText(0,L"Choose the common Base file for a three-way merge."); return; }
        std::wstring detail;
        const auto snapshot = [&](const std::wstring& path) { if (workspace_) return workspace_->SnapshotForPath(path,&detail); auto doc = std::make_unique<simple_editor::core::BinaryDocument>(); if (!doc->Open(path,&detail)) doc.reset(); return doc; };
        auto a = snapshot(left.GetString()), b = snapshot(right.GetString()); auto c = displayed_mode_ == 3 ? snapshot(base.GetString()) : nullptr;
        if (!a || !b || (displayed_mode_ == 3 && !c)) { status_.SetPaneText(0,detail.c_str()); return; }
        auto preview = a->Snapshot(&detail); if (preview) { document_.Swap(*preview); hex_.SetDocument(&document_); text_stale_ = true; }
        advanced_.StartText(std::move(c),std::move(a),std::move(b)); return;
    }
    std::wstring error;
    std::unique_ptr<simple_editor::core::BinaryDocument> source;
    if (workspace_) source = workspace_->SnapshotForPath(left.GetString(), &error);
    else {
        source = std::make_unique<simple_editor::core::BinaryDocument>();
        if (!source->Open(left.GetString(), &error)) source.reset();
    }
    if (!source) { status_.SetPaneText(0, (L"Cannot read left file: " + error).c_str()); return; }
    // A standalone editor must not silently replace its own edited file.
    if (!workspace_ && document_.IsOpen() && (document_.IsModified() || text_dirty_ || text_.GetModify())) {
        status_.SetPaneText(0, L"Save the current editor before choosing another left input."); return;
    }
    document_.Swap(*source); ++revision_; hex_.SetDocument(&document_); text_stale_ = true;
    comparison_session_ = true;
    SetSaveReadOnly(false);
    StartCompare(right.GetString(), compare_mode_.GetCurSel() != 1);
    SetWindowTextW((std::filesystem::path(left.GetString()).filename().wstring() + L" vs " +
                    std::filesystem::path(right.GetString()).filename().wstring() + L" - Simple Editor Tool").c_str());
    if (workspace_) workspace_->RefreshLabels();
}

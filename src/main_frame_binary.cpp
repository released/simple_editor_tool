#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "core/binary_report.h"

CHexView* CMainFrame::ActiveHex() {
    if (active_tab_ == 0) return &hex_;
    if (active_tab_ == 2 && !AdvancedActive()) { if (GetFocus() == &compare_right_) active_compare_right_ = true; else if (GetFocus() == &compare_left_) active_compare_right_ = false; return active_compare_right_ ? static_cast<CHexView*>(&compare_right_) : static_cast<CHexView*>(&compare_left_); }
    return nullptr;
}
void CMainFrame::HexSelectionChanged() {
    if (workspace_) workspace_->TrackNavigation(this);
    if (!status_.GetSafeHwnd() || IsBusy()) return; auto* view = ActiveHex(); if (!view) return;
    CString text; text.Format(L"%s  %s  Offset 0x%llX  Selection 0x%llX + %llu bytes",view->ReadOnly() ? L"Read-only" : view->InsertMode() ? L"Insert" : L"Overwrite",view->AsciiMode() ? L"ASCII/UTF-8" : L"Hex",view->CursorOffset(),view->SelectionStart(),view->SelectionLength()); status_.SetPaneText(0,text);
}
void CMainFrame::OnHexCommand(UINT command) {
    auto* view = ActiveHex(); if (!view) { status_.SetPaneText(0,L"Choose a Hex view for byte operations."); return; }
    if (command == ID_HEX_READ_ONLY) {
        if (comparison_session_ || IsSaving() || (workspace_ && workspace_->DocumentLocked(document_))) return;
        if (!ApplyTextEdits()) return; user_read_only_ = !user_read_only_; SetSaveReadOnly(false); return;
    }
    if (command != ID_HEX_COPY && command != ID_HEX_COPY_TEXT && command != ID_HEX_SELECT_ALL && (IsSaving() || (view == &hex_ && MutationBlocked()))) return;
    try { view->Clipboard(command); } catch (const std::exception&) { status_.SetPaneText(0,L"Clipboard allocation failed; document retained."); }
}
void CMainFrame::ReleaseJobLocks() {
    if (job_locks_held_ && workspace_) workspace_->LockPaths(job_locked_paths_,false,&job_);
    job_locks_held_ = false; job_locked_paths_.clear();
}
bool CMainFrame::StartBinaryReport(const std::wstring& path, int format) {
    if (IsBusy() || AdvancedActive() || !compare_left_document_.IsOpen() || !compare_right_document_.IsOpen()) { status_.SetPaneText(0,L"Run a binary comparison first, then export its complete input snapshots."); return false; }
    std::wstring error; auto left = compare_left_document_.Snapshot(&error), right = compare_right_document_.Snapshot(&error);
    if (!left || !right) { status_.SetPaneText(0,error.c_str()); return false; }
    job_locked_paths_ = {path};
    if (workspace_ && !workspace_->LockPaths(job_locked_paths_,true,&job_)) { job_locked_paths_.clear(); status_.SetPaneText(0,L"The report output is busy or has unsaved editor changes."); return false; }
    job_locks_held_ = workspace_ != nullptr; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = EditorJobKind::Export; job_revision_ = revision_;
    const bool aligned = compare_mode_.GetCurSel() != 1;
    const auto region = compare_region_;
    try {
        job_ = std::async(std::launch::async,[this,path,format,aligned,region,left = std::move(left),right = std::move(right)]() {
            EditorJobResult result; result.kind = EditorJobKind::Export; simple_editor::core::BinaryReportStats stats;
            simple_editor::core::SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned value) { progress_ = value; };
            if (simple_editor::core::ExportBinaryReportRegion(*left,*right,aligned,region,path,static_cast<simple_editor::core::BinaryReportFormat>(format),control,&stats,&result.error))
                result.message = L"Full binary report saved: " + path + L". Changed " + std::to_wstring(stats.changed) + L", added " + std::to_wstring(stats.added) + L", removed " + std::to_wstring(stats.removed) + L", ranges " + std::to_wstring(stats.ranges);
            return result;
        });
    } catch (const std::exception&) { ReleaseJobLocks(); status_.SetPaneText(0,L"Could not start report generation."); return false; }
    SetComparisonControlsEnabled(false); status_.SetPaneText(0,L"Exporting all differences in the background; Esc cancels before replacement."); return true;
}
void CMainFrame::OnExportBinaryReport() {
    if (IsBusy()) return;
    if (!compare_left_document_.IsOpen() || !compare_right_document_.IsOpen()) { status_.SetPaneText(0,L"Run a binary comparison first."); return; }
    CFileDialog dialog(FALSE,L"csv",L"binary-differences.csv",OFN_OVERWRITEPROMPT,L"CSV (*.csv)|*.csv|TSV (*.tsv)|*.tsv|HTML (*.html)|*.html||",this);
    path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK) StartBinaryReport(dialog.GetPathName().GetString(),static_cast<int>(dialog.m_ofn.nFilterIndex) - 1);
}

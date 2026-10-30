#include "main_frame.h"
#include "ui/localization.h"
#include "resource.h"
#include "workspace_frame.h"
#include <algorithm>
#include <chrono>

bool CMainFrame::IsBusy() const { return job_.valid() || advanced_.Busy() || path_dialog_; }

void CMainFrame::ClearComparison() {
    if (difference_mailbox_) { difference_mailbox_->Stop(); difference_mailbox_.reset(); }
    compare_origin_left_ = compare_region_.left_begin; compare_origin_right_ = compare_region_.right_begin;
    if (differences_.GetSafeHwnd()) differences_.SetItemCountEx(0);
    pending_differences_.clear(); difference_index_ = -1;
    compare_left_document_.Close(); compare_right_document_.Close();
    if (compare_left_.GetSafeHwnd()) compare_left_.SetDocument(&compare_left_document_);
    if (compare_right_.GetSafeHwnd()) compare_right_.SetDocument(&compare_right_document_);
    if (compare_left_label_.GetSafeHwnd()) compare_left_label_.SetWindowTextW(L"Left");
    if (compare_right_label_.GetSafeHwnd()) compare_right_label_.SetWindowTextW(L"Right");
}
bool CMainFrame::PrepareComparison(bool insertion_aware) {
    if (IsBusy()) { status_.SetPaneText(0, L"An operation is running. Cancel it or wait for completion."); return false; }
    if (!document_.IsOpen()) { UiMessageBox(L"Open the left file first."); return false; }
    std::wstring source_error;
    if (!document_.ValidateSource(true, &source_error)) { status_.SetPaneText(0, source_error.c_str()); return false; }
    if (!document_.IsModified() && !text_dirty_ && !text_.GetModify()) return true;
    if (UiMessageBox(L"The left file has unsaved changes. Save it before comparing?", MB_YESNO | MB_ICONQUESTION) != IDYES) return false;
    StartSave(document_.Path(), insertion_aware ? AfterSave::CompareInsertion : AfterSave::CompareEqual); return false;
}
void CMainFrame::OnCompare() {
    if (workspace_) { workspace_->NewComparison(document_.Path()); return; }
    if (!PrepareComparison()) return;
    CFileDialog dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST, L"All files (*.*)|*.*||", this);
    if (dialog.DoModal() == IDOK) StartCompare(dialog.GetPathName().GetString(), true);
}
void CMainFrame::OnCompareEqual() {
    if (workspace_) { workspace_->NewComparison(document_.Path(), false); return; }
    if (!PrepareComparison(false)) return;
    CFileDialog dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST, L"All files (*.*)|*.*||", this);
    if (dialog.DoModal() == IDOK) StartCompare(dialog.GetPathName().GetString(), false);
}
void CMainFrame::OnCompareCancel() {
    if (advanced_.Busy()) { advanced_.Cancel(); return; }
    if (IsBusy()) {
        const bool saving = IsSaving();
        cancelled_ = true;
        if (saving) { after_save_ = AfterSave::None; after_save_path_.clear(); if (workspace_) workspace_->CancelCloseAll(); }
        status_.SetPaneText(0, saving ? L"Cancelling save before file replacement..." : L"Cancelling operation...");
    }
}
void CMainFrame::OnUpdateJobCommand(CCmdUI* command) { command->Enable(command->m_nID == ID_FILE_COMPARE_CANCEL ? IsBusy() : !IsBusy() && document_.IsOpen()); }
void CMainFrame::StartCompare(const std::wstring& right, bool insertion_aware) {
    if (IsBusy()) return;
    ClearComparison(); std::wstring error;
    if (!document_.ValidateSource(true, &error)) { status_.SetPaneText(0, error.c_str()); return; }
    auto opened_left = document_.Snapshot(&error);
    auto opened_right = workspace_ ? workspace_->SnapshotForPath(right, &error) : std::make_unique<simple_editor::core::BinaryDocument>();
    if (!opened_left || !opened_right || (!workspace_ && !opened_right->Open(right, &error))) {
        ClearComparison(); UiMessageBox(error.c_str(), MB_ICONERROR); return;
    }
    compare_left_document_.Swap(*opened_left);
    compare_right_document_.Swap(*opened_right);
    auto worker_left = compare_left_document_.Snapshot(&error);
    auto worker_right = compare_right_document_.Snapshot(&error);
    if (!worker_left || !worker_right) { ClearComparison(); status_.SetPaneText(0, error.c_str()); return; }
    compare_left_.SetDocument(&compare_left_document_); compare_right_.SetDocument(&compare_right_document_);
    displayed_mode_ = insertion_aware ? 0 : 1;
    SetComparisonPaths(document_.Path(), right, displayed_mode_);
    if (workspace_) { workspace_->RememberRecent(compare_left_document_.Path()); workspace_->RememberRecent(compare_right_document_.Path()); }
    compare_left_label_.SetWindowTextW(L"Left file"); compare_right_label_.SetWindowTextW(L"Right file");
    ShowPage(2); status_.SetPaneText(0, L"Comparing... Esc cancels.");
    job_revision_ = revision_; job_kind_ = EditorJobKind::Compare; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX;
    const auto region = compare_region_;
    difference_mailbox_ = std::make_shared<DifferenceMailbox>(); const auto mailbox = difference_mailbox_;
    job_ = std::async(std::launch::async, [this,mailbox, left = std::move(worker_left), right = std::move(worker_right), insertion_aware,region]() {
        EditorJobResult result;
        const bool ok = simple_editor::core::CompareDocumentRegions(*left, *right, insertion_aware,region, cancelled_,
            [&](const auto& batch) {
                constexpr std::size_t limit = 1000000;
                const auto available = limit - result.count; const auto count = (std::min)(available,batch.size());
                if (count && !mailbox->Push(simple_editor::core::DifferenceBatch(batch.begin(),batch.begin() + count),cancelled_)) return false;
                result.count += count;
                result.truncated = batch.size() > available;
                return !result.truncated && !cancelled_;
            }, [this](unsigned int percent) { progress_ = percent; }, &result.error);
        if (!ok && result.error.empty() && !cancelled_ && !result.truncated) result.error = L"Comparison failed.";
        return result;
    });
    SetComparisonControlsEnabled(false);
}

void CMainFrame::DrainDifferences(unsigned batches) {
    if (!difference_mailbox_ || cancelled_ || revision_ != job_revision_) return; simple_editor::core::DifferenceBatch batch; bool changed = false;
    while (batches-- && difference_mailbox_->Pop(&batch)) { pending_differences_.insert(pending_differences_.end(),batch.begin(),batch.end()); changed = true; }
    if (changed) differences_.SetItemCountEx(static_cast<int>(pending_differences_.size()),LVSICF_NOSCROLL);
}
void CMainFrame::OnTimer(UINT_PTR id) {
    if (id != 1) { CFrameWnd::OnTimer(id); return; }
    TickIndentation(); advanced_.Tick(); TickUnicode(); TickFunctionSidebar();
    const auto tick = GetTickCount64();
    if (!IsSaving() && document_.IsOpen() && tick - source_check_tick_ >= 1000) {
        source_check_tick_ = tick; std::wstring error,own_error;
        const bool own_valid = document_.ValidateSource(true,&own_error); error = own_error;
        if (!own_valid || (compare_right_document_.IsOpen() && !compare_right_document_.ValidateSource(true, &error))) {
            if (source_notice_key_.empty()) { ClearComparison(); hex_.Invalidate(); status_.SetPaneText(0, error.c_str()); }
        }
        CheckSourceNotice(own_error);
    }
    if (!job_.valid()) return;
    if (job_kind_ == EditorJobKind::Compare) DrainDifferences();
    if (job_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) FinishJob();
    else if (shown_progress_ != progress_.load()) {
        shown_progress_ = progress_; CString text; text.Format(job_kind_ == EditorJobKind::Save ? L"Saving... %u%%. Esc cancels before replacement." : L"Processing... %u%%. Esc cancels.", shown_progress_); status_.SetPaneText(0, text);
    }
}
void CMainFrame::FinishJob() {
    SetComparisonControlsEnabled(true);
    EditorJobResult result;
    try { result = job_.get(); }
    catch (const std::exception&) {
        ReleaseJobLocks(); if (job_kind_ == EditorJobKind::DiskReview) SetSaveReadOnly(workspace_ && workspace_->DocumentLocked(document_));
        if (job_kind_ == EditorJobKind::Compare) ClearComparison();
        if (job_kind_ == EditorJobKind::TextSearch || job_kind_ == EditorJobKind::Patch || job_kind_ == EditorJobKind::TextCommand) UnlockEditorSearch();
        if (job_kind_ == EditorJobKind::Save) { result.kind = EditorJobKind::Save; result.error = L"Save failed: insufficient memory or an I/O error. Edits are retained."; FinishSave(std::move(result)); }
        else status_.SetPaneText(0, L"Operation failed: insufficient memory or an I/O error.");
        return;
    }
    if (result.kind == EditorJobKind::Outline) { FinishOutline(std::move(result)); return; }
    if (result.kind == EditorJobKind::SelectionMap || result.kind == EditorJobKind::UnicodeScan || result.kind == EditorJobKind::Statistics) { FinishExpansion(std::move(result)); return; }
    if (result.kind == EditorJobKind::IpsImport || result.kind == EditorJobKind::IpsExport) { FinishIps(std::move(result)); return; }
    if (result.kind == EditorJobKind::DiskReview) { FinishDiskReview(std::move(result)); return; }
    if (result.kind == EditorJobKind::Save) { FinishSave(std::move(result)); return; }
    if (result.kind == EditorJobKind::TextSearch) { FinishEditorSearch(std::move(result)); return; }
    if (result.kind == EditorJobKind::ByteSearch) { FinishByteSearch(std::move(result)); return; }
    if (result.kind == EditorJobKind::Patch) { FinishPatch(std::move(result)); return; }
    if (result.kind == EditorJobKind::Template) { FinishTemplate(std::move(result)); return; }
    if (result.kind == EditorJobKind::TextCommand) { FinishTextCommand(std::move(result)); return; }
    if (result.kind == EditorJobKind::WorkflowChanges || result.kind == EditorJobKind::WorkflowStrings) { FinishWorkflowJob(std::move(result)); return; }
    if (result.kind == EditorJobKind::Export) { ReleaseJobLocks(); status_.SetPaneText(0,(result.error.empty() ? result.message : result.error).c_str()); if (result.error.empty() && !result.open_path.empty() && workspace_) workspace_->OpenPath(result.open_path); return; }
    if (revision_ != job_revision_) { if (result.kind == EditorJobKind::Compare) ClearComparison(); status_.SetPaneText(0, L"Document changed; the old operation results were discarded."); return; }
    if (cancelled_) { if (result.kind == EditorJobKind::Compare) ClearComparison(); status_.SetPaneText(0, L"Operation cancelled; document unchanged."); return; }
    if (!result.error.empty()) { if (result.kind == EditorJobKind::Compare) ClearComparison(); status_.SetPaneText(0, result.error.c_str()); return; }
    if (result.kind == EditorJobKind::Tool) { status_.SetPaneText(0,result.message.c_str()); if (!result.details.empty()) { CTextResultDialog dialog(this,result.message,result.details); path_dialog_ = true; dialog.DoModal(); path_dialog_ = false; } return; }
    std::wstring source_error;
    if (!document_.ValidateSource(true, &source_error) ||
        (result.kind == EditorJobKind::Compare && (!compare_left_document_.ValidateSource(true, &source_error) || !compare_right_document_.ValidateSource(true, &source_error)))) {
        ClearComparison(); status_.SetPaneText(0, source_error.c_str()); return;
    }
    CString message;
    if (result.kind == EditorJobKind::Compare) {
        DrainDifferences(16); if (difference_mailbox_) difference_mailbox_->Stop(); difference_mailbox_.reset();
        differences_.SetItemCountEx(static_cast<int>(pending_differences_.size()));
        message.Format(L"Compare complete: %llu differences%s. F7 / Shift+F7 navigate.", static_cast<unsigned long long>(pending_differences_.size()), result.truncated ? L" (first 1,000,000 shown)" : L"");
        if (!pending_differences_.empty()) NavigateDifference(0);
    } else if (result.kind == EditorJobKind::Find) {
        if (result.found == UINT64_MAX) message = L"No match found.";
        else {
            match_offset_ = result.found; ShowPage(0); hex_.SelectRange(result.found, match_pattern_.size());
            message.Format(L"Match at 0x%llX%s", result.found, result.wrapped ? L" (wrapped)" : L"");
        }
    } else {
        if (result.count && result.document) { document_.Swap(*result.document); DocumentChanged(); hex_.SetDocument(&document_); ShowPage(0); }
        if (!result.message.empty()) message = result.message.c_str(); else message.Format(L"Replaced %llu matches. Undo restores the entire operation.", static_cast<unsigned long long>(result.count));
    }
    status_.SetPaneText(0, message);
    if (result.kind == EditorJobKind::Compare && pending_differences_.empty() && !result.truncated) NotifyNoDifferences();
}
void CMainFrame::NotifyNoDifferences() {
    const bool previous = path_dialog_; path_dialog_ = true;
    UiMessageBox(L"Comparison complete. No differences were found using the current comparison settings.",MB_OK | MB_ICONINFORMATION);
    path_dialog_ = previous;
}

void CMainFrame::OnDifferenceDisplay(NMHDR* header, LRESULT* result) {
    auto* info = reinterpret_cast<NMLVDISPINFO*>(header); *result = 0;
    if (!(info->item.mask & LVIF_TEXT) || info->item.iItem < 0 || static_cast<std::size_t>(info->item.iItem) >= pending_differences_.size()) return;
    const auto& d = pending_differences_[info->item.iItem]; CString value;
    switch (info->item.iSubItem) {
    case 0: value.Format(L"%d", info->item.iItem + 1); break;
    case 1: if (d.left_offset == simple_editor::core::kMissingOffset) value = L"-"; else value.Format(L"0x%016llX", d.left_offset); break;
    case 2: if (d.left_byte < 0) value = L"-"; else value.Format(L"%02X", d.left_byte); break;
    case 3: if (d.right_offset == simple_editor::core::kMissingOffset) value = L"-"; else value.Format(L"0x%016llX", d.right_offset); break;
    case 4: if (d.right_byte < 0) value = L"-"; else value.Format(L"%02X", d.right_byte); break;
    }
    wcsncpy_s(info->item.pszText, info->item.cchTextMax, value, _TRUNCATE);
}
void CMainFrame::NavigateDifference(int index) {
    if (pending_differences_.empty()) return;
    index = (std::clamp)(index, 0, static_cast<int>(pending_differences_.size()) - 1);
    ShowPage(2);
    if (active_tab_ != 2 || static_cast<std::size_t>(index) >= pending_differences_.size()) return;
    difference_index_ = index;
    const auto& d = pending_differences_[index];
    compare_origin_left_ = d.left_anchor != simple_editor::core::kMissingOffset ? d.left_anchor : d.left_offset != simple_editor::core::kMissingOffset ? d.left_offset : compare_region_.left_begin;
    compare_origin_right_ = d.right_anchor != simple_editor::core::kMissingOffset ? d.right_anchor : d.right_offset != simple_editor::core::kMissingOffset ? d.right_offset : compare_region_.right_begin;
    if (d.left_offset != simple_editor::core::kMissingOffset) compare_left_.SelectRange(d.left_offset, 1); else { compare_left_.GoTo(compare_origin_left_); compare_left_.ClearSelection(); }
    if (d.right_offset != simple_editor::core::kMissingOffset) compare_right_.SelectRange(d.right_offset, 1); else { compare_right_.GoTo(compare_origin_right_); compare_right_.ClearSelection(); }
    differences_.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    differences_.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED); differences_.EnsureVisible(index, FALSE);
    CString text; text.Format(L"Difference %d / %llu (%s)", index + 1, static_cast<unsigned long long>(pending_differences_.size()), d.left_byte < 0 ? L"added" : d.right_byte < 0 ? L"removed" : L"changed"); status_.SetPaneText(0, text);
}
void CMainFrame::OnPreviousDifference() {
    if (AdvancedActive()) { advanced_.Navigate(true); return; }
    if (!pending_differences_.empty()) NavigateDifference(difference_index_ <= 0 ? static_cast<int>(pending_differences_.size()) - 1 : difference_index_ - 1);
}
void CMainFrame::OnNextDifference() {
    if (AdvancedActive()) { advanced_.Navigate(false); return; }
    if (!pending_differences_.empty()) NavigateDifference(difference_index_ + 1 >= static_cast<int>(pending_differences_.size()) ? 0 : difference_index_ + 1);
}
void CMainFrame::OnDifferenceActivate(NMHDR* header, LRESULT* result) { *result = 0; const int index = reinterpret_cast<NMITEMACTIVATE*>(header)->iItem; if (index >= 0) NavigateDifference(index); }

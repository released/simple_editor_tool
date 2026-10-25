#include "advanced_compare.h"
#include "../core/sync_journal.h"
#include <Shellapi.h>
#include <algorithm>
using namespace simple_editor::core;
bool CAdvancedCompareView::FolderRowVisible(const FolderRow& row) const {
    const auto filter = folder_filter_.GetCurSel(); if (filter == 9) return row.text_state == L"Important text differences"; if (filter == 10) return row.text_state == L"Equal under text rules"; if (filter == 11) return !row.text_error.empty();
    if (!show_equal_.GetCheck() && row.state == FolderState::Equal) return false;
    switch (folder_filter_.GetCurSel()) {
    case 1: return row.state == FolderState::Different;
    case 2: return row.state == FolderState::LeftOnly;
    case 3: return row.state == FolderState::RightOnly;
    case 4: return row.state == FolderState::Error || row.state == FolderState::TypeConflict;
    case 5: case 6: return row.left.exists && row.right.exists && !row.left.directory && !row.right.directory && row.newness == (folder_filter_.GetCurSel() == 5 ? FolderNewness::LeftNewer : FolderNewness::RightNewer);
    case 7: return row.state == FolderState::Equal;
    case 8: return row.state == FolderState::Different && row.newness == FolderNewness::SameTime;
    default: return true;
    }
}
void CAdvancedCompareView::OrderFolderRows() {
    if (!scan_) return;
    std::stable_sort(visible_.begin(),visible_.end(),[this](std::size_t first,std::size_t second) {
        const auto& a = scan_->rows[first]; const auto& b = scan_->rows[second]; int order = 0;
        const auto compare = [](auto x,auto y) { return x < y ? -1 : x > y ? 1 : 0; };
        switch (folder_sort_) {
        case 1: order = compare(a.state,b.state); break;
        case 2: order = compare(a.left.size,b.left.size); break;
        case 3: order = compare(a.right.size,b.right.size); break;
        case 4: order = compare(a.newness,b.newness); break;
        case 5: order = compare(a.left.modified,b.left.modified); break;
        case 6: order = compare(a.right.modified,b.right.modified); break;
        }
        if (!order) order = CompareStringOrdinal(a.relative.c_str(),-1,b.relative.c_str(),-1,TRUE) - CSTR_EQUAL;
        return folder_sort_descending_ ? order > 0 : order < 0;
    });
}
void CAdvancedCompareView::OnFolderColumn(NMHDR* header,LRESULT* result) {
    *result = 0; if (Busy() || mode_ != 4 || !scan_ || stale_) return;
    const auto column = reinterpret_cast<NMLISTVIEW*>(header)->iSubItem; if (column < 0 || column > 6) return;
    if (folder_sort_ == static_cast<unsigned>(column)) folder_sort_descending_ = !folder_sort_descending_; else { folder_sort_ = column; folder_sort_descending_ = false; }
    RebuildRows(); if (changed) changed();
}
void CAdvancedCompareView::OnFolderAction(UINT id) {
    if (Busy() || mode_ != 4) return;
    if (id == 575 || id == 576) ChooseBaseline(id == 575);
    else if (id == 579 || id == 580) ChooseJournal(id == 580);
    else if (id == 581) {
        const auto path = last_journal_.empty() ? std::filesystem::path(folder_right_) / L".SimpleEditorTool.backups" : std::filesystem::path(last_journal_).parent_path();
        std::wstring error; FileStamp stamp; if (!InspectFile(path,&stamp,&error) || !stamp.exists || !stamp.directory) { Status(L"No recovery backups are available here yet."); return; }
        if (reinterpret_cast<INT_PTR>(ShellExecuteW(GetSafeHwnd(),L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL)) <= 32) Status(L"Could not open the backup directory.");
    }
}
void CAdvancedCompareView::ChooseBaseline(bool save) {
    if (Busy() || !scan_ || stale_) { Status(L"Compare the folders before choosing or saving a baseline."); return; }
    CFileDialog dialog(save ? FALSE : TRUE,L"setb",baseline_path_.empty() ? L"folders.setb" : baseline_path_.c_str(),save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST,L"Folder baseline (*.setb)|*.setb|All files (*.*)|*.*||",this);
    modal_ = true; const auto accepted = dialog.DoModal(); modal_ = false; if (accepted != IDOK) return;
    if (save) StartBaselineSave(dialog.GetPathName().GetString()); else { baseline_path_ = dialog.GetPathName().GetString(); Status(L"Baseline selected: " + baseline_path_ + L". Its roots and fingerprints are checked before planning."); }
}
bool CAdvancedCompareView::StartBaselineSave(const std::wstring& path) {
    if (Busy() || !scan_ || stale_) return false;
    locked_paths_ = {path}; locked_stamps_.clear(); if (lock_paths && !lock_paths(locked_paths_,true,locked_stamps_)) { locked_paths_.clear(); Status(L"The baseline output has unsaved edits or is busy."); return false; } locks_held_ = static_cast<bool>(lock_paths);
    const auto scan = scan_; cancelled_ = false; job_kind_ = JobKind::Report; SetBusy(true);
    job_ = std::async(std::launch::async,[this,scan,path]() { Result result; result.kind = JobKind::Report; FolderBaseline baseline; if (CaptureFolderBaseline(*scan,cancelled_,[this](unsigned p) { progress_ = p; },&baseline,&result.error) && !cancelled_) { result.saved = SaveFolderBaseline(path,baseline,&result.error); if (result.saved) { result.output_path = path; result.text = L"Folder baseline saved: " + path; } } return result; }); return true;
}
void CAdvancedCompareView::ChooseJournal(bool recover) {
    if (Busy()) return; CFileDialog dialog(TRUE,L"setj",last_journal_.empty() ? L"operations.setj" : last_journal_.c_str(),OFN_FILEMUSTEXIST,L"Synchronization journal (operations.setj)|operations.setj|All files (*.*)|*.*||",this);
    modal_ = true; const auto accepted = dialog.DoModal(); modal_ = false; if (accepted == IDOK) StartJournalPlan(dialog.GetPathName().GetString(),recover);
}
bool CAdvancedCompareView::StartJournalPlan(const std::wstring& path,bool recover) {
    if (Busy() || mode_ != 4) return false; const auto left = folder_left_,right = folder_right_; cancelled_ = false; job_kind_ = JobKind::Plan; SetBusy(true);
    job_ = std::async(std::launch::async,[path,recover,left,right]() { Result result; result.kind = JobKind::Plan; SyncJournal journal; SyncPlan plan;
        if (!LoadSyncJournal(path,&journal,&result.error)) return result;
        if (CompareStringOrdinal(journal.plan.target_root.c_str(),-1,left.c_str(),-1,TRUE) != CSTR_EQUAL && CompareStringOrdinal(journal.plan.target_root.c_str(),-1,right.c_str(),-1,TRUE) != CSTR_EQUAL) { result.error = L"Choose a journal whose target is one of this comparison's folders."; return result; }
        std::size_t completed = 0,started = 0,errors = 0; std::wstring details;
        for (std::size_t i = 0; i < journal.entries.size(); ++i) { const auto& entry = journal.entries[i]; completed += entry.completed; started += entry.started; if (!entry.error.empty()) { if (++errors <= 20) details += journal.plan.operations[i].relative.wstring() + L": " + entry.error + L"\r\n"; } }
        result.bundle.note = L"Journal: " + path + L"\r\nCompleted: " + std::to_wstring(completed) + L", never started: " + std::to_wstring(journal.entries.size() - started) + L", errors: " + std::to_wstring(errors) + L"\r\n" + details;
        if (recover ? BuildSyncRecovery(journal,&plan,&result.error) : BuildSyncRetry(journal,&plan,&result.error)) result.bundle.plans.push_back(std::move(plan)); return result;
    }); return true;
}

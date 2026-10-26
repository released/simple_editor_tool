#include "advanced_compare.h"
#include "localization.h"
#include "sync_preview.h"
#include "../resource.h"
#include <algorithm>
#include <set>

using namespace simple_editor::core;
namespace {
struct ModalFlag { bool& flag; explicit ModalFlag(bool& value) : flag(value) { flag = true; } ~ModalFlag() { flag = false; } };
std::vector<MergeChoice> Choices(const TextMergeModel& model) {
    std::vector<MergeChoice> result; for (const auto& block : model.blocks) result.push_back(block.choice); return result;
}
}
bool CAdvancedCompareView::CanReset() {
    if (Busy()) { Status(L"Wait for the current operation or cancel it first."); return false; }
    if (!dirty_) return true;
    int answer; { ModalFlag guard(modal_); answer = UiMessageBox(L"The merge result has unsaved changes. Save the result before continuing?",MB_YESNOCANCEL | MB_ICONWARNING); }
    if (answer == IDYES) { SaveResult(); return false; }
    if (answer == IDCANCEL) return false;
    dirty_ = false; return true;
}
bool CAdvancedCompareView::CanClose() {
    if (modal_) { Status(L"Complete or cancel the active dialog before closing this tab."); return false; }
    if (Busy()) {
        if (job_kind_ == JobKind::Save || job_kind_ == JobKind::NotesSave) { close_after_save_ = true; Status(L"The tab will close after the merge result is saved."); }
        else Status(L"Wait or cancel the current comparison/synchronization before closing this tab.");
        return false;
    }
    if (!ConfirmNotesClose()) return false;
    if (HasPastedInputs()) {
        ModalFlag guard(modal_);
        if (UiMessageBox(L"Close this tab and discard its pasted input text? Pasted inputs are not saved in workspace files. Save result only saves the merged output.",MB_OKCANCEL | MB_ICONWARNING) != IDOK) return false;
    }
    if (!dirty_) return true;
    int answer; { ModalFlag guard(modal_); answer = UiMessageBox(L"The merge result has unsaved changes. Save it before closing this tab?",MB_YESNOCANCEL | MB_ICONWARNING); }
    if (answer == IDYES) { SaveResult(); close_after_save_ = Busy() && job_kind_ == JobKind::Save; return false; }
    return answer == IDNO;
}
void CAdvancedCompareView::Cancel() { if (Busy()) { cancelled_ = true; close_after_save_ = false; Status(L"Cancelling before the next file replacement. Already committed files remain committed."); if (saved_close) saved_close(false); } }
void CAdvancedCompareView::ReleaseLocks() {
    if (locks_held_ && lock_paths) lock_paths(locked_paths_,false,locked_stamps_); locks_held_ = false; locked_paths_.clear(); locked_stamps_.clear();
}
void CAdvancedCompareView::RenderAsync() {
    CString current; output_.GetWindowTextW(current); const auto previous = generated_, manual = TextFromEditControl(generated_,std::wstring(current.GetString(),current.GetLength()));
    cancelled_ = false; job_kind_ = JobKind::Render; SetBusy(true); auto model = model_;
    job_ = std::async(std::launch::async,[this,model,previous,manual]() { Result result; result.kind = JobKind::Render; result.text = RenderMerge(*model);
        if (manual != previous) { TextMergeModel rebase; if (BuildTextMerge(previous,manual,result.text,true,{},cancelled_,&rebase,&result.error)) result.text = RenderMerge(rebase,&result.extra_conflicts); }
        return result;
    });
}
void CAdvancedCompareView::RenderCheckpoint() { render_before_ = Choices(*model_); }
void CAdvancedCompareView::Take(MergeChoice choice) {
    if (manual_view_) { Status(L"Manual alignment is a read-only comparison. Clear anchors and compare again before merging."); return; }
    if (Busy() || !model_ || stale_) return;
    std::set<std::size_t> blocks;
    for (int row = rows_.GetNextItem(-1,LVNI_SELECTED); row >= 0; row = rows_.GetNextItem(row,LVNI_SELECTED)) if (static_cast<std::size_t>(row) < visible_.size() && !context_rows_.hidden.count(visible_[row])) blocks.insert(text_rows_[visible_[row]].block);
    if (blocks.empty()) { Status(L"Select changed rows first. Ctrl+A selects all visible rows."); return; }
    RenderCheckpoint(); bool altered = false;
    for (auto index : blocks) if (model_->blocks[index].changed && model_->blocks[index].choice != choice) { model_->blocks[index].choice = choice; altered = true; }
    if (!altered) { render_before_.clear(); return; }
    RenderAsync();
}
void CAdvancedCompareView::Undo(bool redo) {
    if (::GetFocus() == source_left_.GetSafeHwnd() || ::GetFocus() == source_right_.GetSafeHwnd() || ::GetFocus() == source_base_.GetSafeHwnd()) return;
    if (!Busy()) if (auto* input = FocusedInput()) { if (redo) input->Redo(); else input->Undo(); return; }
    if (manual_view_) return;
    if (Busy() || !model_) return;
    suppress_result_ = true; if (redo) output_.Redo(); else output_.Undo(); suppress_result_ = false; RefreshOutputState(); RebuildRows();
}
void CAdvancedCompareView::RestoreChoiceAction(int token, bool undo) {
    if (!model_ || token <= 0 || static_cast<std::size_t>(token) > choice_actions_.size()) return;
    const auto& action = choice_actions_[token - 1]; for (const auto& [index,before,after] : action.delta) if (index < model_->blocks.size()) model_->blocks[index].choice = undo ? before : after;
    extra_conflicts_ = undo ? action.before_extra : action.after_extra;
    generated_ = RenderMerge(*model_,&model_unresolved_); display_generated_ = TextForEditControl(generated_); manual_resolved_ = false;
}
void CAdvancedCompareView::RefreshOutputState() {
    CString text; output_.GetWindowTextW(text); manual_ = text.GetString() != display_generated_; dirty_ = text.GetString() != saved_text_;
    unresolved_ = manual_resolved_ ? 0 : model_unresolved_ + extra_conflicts_; if (changed) changed();
}
void CAdvancedCompareView::SaveResult() {
    if (manual_view_) { Status(L"The manually aligned view is read-only."); return; }
    if (Busy() || !model_ || mode_ == 4) return;
    if (unresolved_ && !allow_markers_.GetCheck()) { Status(L"Resolve all conflicts before saving, or explicitly enable Allow unresolved markers to save a conflict file."); return; }
    CFileDialog dialog(FALSE,L"txt",saved_path_.empty() ? L"merged.txt" : saved_path_.c_str(),OFN_OVERWRITEPROMPT,L"All files (*.*)|*.*||",this);
    INT_PTR selected; { ModalFlag guard(modal_); selected = dialog.DoModal(); } if (selected != IDOK) return;
    SaveTo(dialog.GetPathName().GetString());
}
bool CAdvancedCompareView::SaveTo(const std::wstring& path) {
    if (Busy() || !model_ || mode_ == 4 || (unresolved_ && !allow_markers_.GetCheck())) return false;
    if (read_only_snapshots_) {
        FileStamp destination; std::wstring error; if (!InspectFile(path,&destination,&error)) { Status(error); return false; }
        for (const auto& input : snapshot_protection_) if (_wcsicmp(std::filesystem::absolute(path).lexically_normal().c_str(),std::filesystem::absolute(input.path).lexically_normal().c_str()) == 0 || (destination.exists && input.identity == std::make_pair(destination.volume,destination.identity))) { Status(L"Save the reviewed result to a separate output file; the original disk input is protected."); return false; }
    }
    CString text; output_.GetWindowTextW(text);
    const auto value = TextFromEditControl(generated_,std::wstring(text.GetString(),text.GetLength())); std::vector<std::uint8_t> bytes; std::wstring error;
    if (!EncodeText(value,codec_,true,&bytes,&error)) { Status(error); return false; }
    locked_paths_ = {path}; locked_stamps_.clear();
    if (lock_paths && !lock_paths(locked_paths_,true,locked_stamps_)) { locked_paths_.clear(); Status(L"The output is busy or has unsaved editor changes. Save that editor or choose another output path."); return false; }
    locks_held_ = static_cast<bool>(lock_paths);
    const bool check_expected = !saved_path_.empty() && _wcsicmp(saved_path_.c_str(),path.c_str()) == 0;
    const auto expected = saved_stamp_; cancelled_ = false; job_kind_ = JobKind::Save; SetBusy(true);
    job_ = std::async(std::launch::async,[this,path,bytes = std::move(bytes),check_expected,expected]() {
        Result result; result.kind = JobKind::Save; result.output_path = path; SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; };
        result.saved = WriteBytesSafely(path,bytes,&result.error,control,check_expected ? &expected : nullptr,&result.written); return result;
    });
    return true;
}
void CAdvancedCompareView::PlanSync(bool left_to_right, bool all) {
    if (Busy() || !scan_ || stale_) return;
    if (all && scan_->partial_refresh) { Status(L"Run full Compare before Sync all after a selected refresh."); return; }
    std::vector<std::size_t> selected;
    if (all) selected = folder_tree_ ? eligible_folder_rows_ : visible_;
    else for (int row = rows_.GetNextItem(-1,LVNI_SELECTED); row >= 0; row = rows_.GetNextItem(row,LVNI_SELECTED)) if (static_cast<std::size_t>(row) < visible_.size()) selected.push_back(visible_[row]);
    if (selected.empty()) { Status(L"Select file/folder rows first."); return; }
    for (auto index : selected) if (scan_->partial_refresh && !scan_->rows[index].fresh) { Status(L"Refresh selected rows before synchronization."); return; }
    auto scan = scan_; const bool mirror = mirror_.GetCheck() == BST_CHECKED;
    const auto selection = sync_policy_.GetCurSel(); const auto policy = mirror ? SyncPolicy::Mirror : selection == 1 ? SyncPolicy::Newer : selection == 2 ? SyncPolicy::NewerBoth : selection == 3 ? SyncPolicy::BaselineBoth : SyncPolicy::Update;
    if (mirror && selection != 0) { Status(L"Mirror is a separate directed policy. Select directed update before enabling Mirror."); return; }
    const auto baseline_path = baseline_path_; const bool deletions = propagate_deletions_.GetCheck() != 0; const auto tolerance = static_cast<unsigned>((std::max)(0,folder_tolerance_.GetCurSel()));
    cancelled_ = false; job_kind_ = JobKind::Plan; SetBusy(true);
    job_ = std::async(std::launch::async,[this,scan,selected = std::move(selected),left_to_right,policy,baseline_path,deletions,tolerance]() { Result result; result.kind = JobKind::Plan; FolderBaseline baseline; if (policy == SyncPolicy::BaselineBoth && !LoadFolderBaseline(baseline_path,&baseline,&result.error)) return result; BuildPolicySyncPlan(*scan,selected,left_to_right,policy,tolerance,deletions,policy == SyncPolicy::BaselineBoth ? &baseline : nullptr,cancelled_,[this](unsigned p) { progress_ = p; },&result.bundle,&result.error); return result; });
}
void CAdvancedCompareView::ApplyPlan(SyncPlan plan) {
    SyncBundle bundle; bundle.plans.push_back(std::move(plan)); ApplyBundle(std::move(bundle));
}
void CAdvancedCompareView::ApplyBundle(SyncBundle bundle) {
    std::size_t count = bundle.conflicts.size(); for (const auto& plan : bundle.plans) count += plan.operations.size();
    if (!count) { Status(L"No operations are needed for this selection."); return; }
    CSyncPreview preview(std::move(bundle),this); INT_PTR accepted; { ModalFlag guard(modal_); accepted = preview.DoModal(); } if (accepted != IDOK) { Status(L"Synchronization preview cancelled. No files were changed."); return; }
    bundle = preview.Selected();
    locked_paths_.clear(); locked_stamps_.clear();
    for (const auto& plan : bundle.plans) for (const auto& item : plan.operations) {
        locked_paths_.push_back((plan.target_root / item.relative).wstring());
        locked_stamps_.push_back(item.target);
        if (item.source.exists && !item.source.directory) { locked_paths_.push_back((plan.source_root / item.relative).wstring()); locked_stamps_.push_back(item.source); }
    }
    if (lock_paths && !lock_paths(locked_paths_,true,locked_stamps_)) { locked_paths_.clear(); locked_stamps_.clear(); Status(L"A source/output has unsaved edits or another operation. Save it or deselect it before synchronization."); return; }
    locks_held_ = static_cast<bool>(lock_paths); cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = JobKind::Sync; SetBusy(true);
    job_ = std::async(std::launch::async,[this,bundle = std::move(bundle)]() {
        Result result; result.kind = JobKind::Sync;
        for (const auto& plan : bundle.plans) for (const auto& op : plan.operations) {
            FileStamp source,target; if (!InspectFile(plan.source_root / op.relative,&source,&result.sync.error) || !InspectFile(plan.target_root / op.relative,&target,&result.sync.error) || !SameFileState(source,op.source) || !SameFileState(target,op.target)) { if (result.sync.error.empty()) result.sync.error = L"A file in the preview changed; no operation group was started."; return result; }
        }
        for (const auto& plan : bundle.plans) { if (plan.operations.empty()) continue; auto current = ApplySyncPlan(plan,cancelled_,[this](unsigned p) { progress_ = p; }); result.sync.completed += current.completed; if (!result.text.empty()) result.text += L"; "; result.text += current.backup.wstring() + L" (journal: " + current.journal.wstring() + L")"; result.sync.backup = current.backup; if (!current.journal.empty()) result.output_path = current.journal.wstring(); if (current.cancelled || !current.error.empty()) { result.sync.cancelled = current.cancelled; result.sync.error = current.error; break; } } return result;
    });
}
void CAdvancedCompareView::OnAction(UINT id) {
    switch (id) {
    case ID_ADV_TAKE_LEFT: Take(MergeChoice::Left); break; case ID_ADV_TAKE_RIGHT: Take(MergeChoice::Right); break;
    case ID_ADV_TAKE_BASE: Take(MergeChoice::Base); break; case ID_ADV_TAKE_BOTH: Take(MergeChoice::Both); break;
    case ID_ADV_UNDO: Undo(); break; case ID_ADV_REDO: Undo(true); break; case ID_ADV_SAVE: SaveResult(); break;
    case ID_ADV_COPY_RIGHT: PlanSync(true,false); break; case ID_ADV_COPY_LEFT: PlanSync(false,false); break;
    case ID_ADV_SYNC_RIGHT: PlanSync(true,true); break; case ID_ADV_SYNC_LEFT: PlanSync(false,true); break;
    case ID_ADV_MANUAL_RESOLVED:
        if (!Busy() && model_ && manual_) {
            CString value; output_.GetWindowTextW(value); bool markers = false;
            for (const auto& line : SplitLines(value.GetString())) if (line.text == L"<<<<<<< LEFT" || line.text == L"||||||| BASE" || line.text == L"=======" || line.text == L">>>>>>> RIGHT") markers = true;
            if (markers) Status(L"Remove all conflict-marker lines before marking the manual result resolved.");
            else { manual_resolved_ = true; unresolved_ = 0; RebuildRows(); Status(L"Manual result marked resolved. Save result writes the reviewed output; source versions remain intact."); }
        }
        break;
    }
}
void CAdvancedCompareView::Navigate(bool previous, bool conflicts_only) {
    if (Busy() || stale_ || visible_.empty()) return;
    int row = rows_.GetNextItem(-1,LVNI_SELECTED); const int count = static_cast<int>(visible_.size());
    const std::size_t current_block = row >= 0 && model_ && mode_ != 4 ? text_rows_[visible_[row]].block : SIZE_MAX;
    for (int step = 0; step < count; ++step) {
        row = previous ? (row <= 0 ? count - 1 : row - 1) : (row + 1) % count;
        bool match = false;
        if (mode_ == 4 && scan_) match = scan_->rows[visible_[row]].state != FolderState::Equal;
        else if (mode_ >= 5 && structured_) match = true;
        else if (model_) { const auto block = text_rows_[visible_[row]].block; const auto& b = model_->blocks[block]; match = block != current_block && b.changed && b.important && (!conflicts_only || (b.conflict && b.choice == MergeChoice::Unresolved)); }
        if (match) { rows_.SetItemState(-1,0,LVIS_SELECTED); rows_.SetItemState(row,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED); rows_.EnsureVisible(row,FALSE); return; }
    }
}
void CAdvancedCompareView::Find(const std::wstring& text) {
    if (stale_ || text.empty() || visible_.empty()) return; const int count = static_cast<int>(visible_.size()); int row = rows_.GetNextItem(-1,LVNI_SELECTED);
    for (int step = 0; step < count; ++step) { row = (row + 1) % count; for (int column = 0; column < (mode_ == 4 ? 5 : 7); ++column) if (Cell(row,column).find(text) != std::wstring::npos) { rows_.SetItemState(-1,0,LVIS_SELECTED); rows_.SetItemState(row,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED); rows_.EnsureVisible(row,FALSE); return; } }
    Status(L"No matching visible line/path.");
}
void CAdvancedCompareView::OnDoubleClick(NMHDR* header, LRESULT* result) {
    *result = 0; if (Busy()) return;
    const int row = reinterpret_cast<NMITEMACTIVATE*>(header)->iItem; if (row < 0 || static_cast<std::size_t>(row) >= visible_.size()) return;
    if (model_ && context_rows_.hidden.count(visible_[row])) { context_expanded_.insert(visible_[row]); RebuildRows(); return; }
    if (mode_ != 4 || !scan_ || !open_pair) return;
    const auto& item = scan_->rows[visible_[row]];
    if (folder_tree_ && (item.left.directory || item.right.directory)) { const auto index = visible_[row]; if (!tree_expanded_.erase(index)) tree_expanded_.insert(index); RebuildRows(); return; }
    const auto left = item.left.exists ? (scan_->left_root / item.relative).wstring() : L"", right = item.right.exists ? (scan_->right_root / item.relative).wstring() : L"";
    if (item.left.directory || item.right.directory) { if (!left.empty() && !right.empty() && item.left.directory && item.right.directory) open_pair(left,right,4); else Status(L"Choose matching folders or synchronize the missing folder first."); return; }
    const auto extension = std::filesystem::path(item.relative).extension().wstring();
    const bool text = MatchFileFilter(extension,L".txt;.c;.cpp;.h;.hpp;.py;.js;.ts;.cs;.xml;.json;.ini;.md;.csv;.log;.ps1");
    open_pair(left,right,text ? 2 : 0);
}

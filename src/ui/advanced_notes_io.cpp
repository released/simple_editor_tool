#include "advanced_compare.h"
using namespace simple_editor::core;
bool CAdvancedCompareView::StartNotesIo(const std::wstring& path,bool save,bool report) {
    if (Busy()) return false;
    if ((!save || report) && (stale_ || !model_ || mode_ >= 4 || review_fingerprint_.empty())) { Status(L"Run a text comparison before loading notes or exporting an annotated report."); return false; }
    auto notes = notes_; if (notes.notes.empty()) notes.fingerprint = review_fingerprint_.empty() ? notes.fingerprint : review_fingerprint_;
    if (save && notes.fingerprint.empty()) { Status(L"There is no comparison fingerprint for these notes."); return false; }
    if (report && notes.fingerprint != review_fingerprint_) { Status(L"Notes belong to an older comparison; the report was not written."); return false; }
    locked_paths_ = {path}; locked_stamps_.clear(); if (lock_paths && !lock_paths(locked_paths_,true,locked_stamps_)) { locked_paths_.clear(); Status(L"The notes/report file has unsaved edits or is busy."); return false; } locks_held_ = static_cast<bool>(lock_paths);
    const auto model = model_; const auto inputs = report_inputs_; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = report ? JobKind::Report : save ? JobKind::NotesSave : JobKind::NotesLoad; SetBusy(true);
    try { job_ = std::async(std::launch::async,[this,path,save,report,notes,model,inputs] { Result result; result.kind = report ? JobKind::Report : save ? JobKind::NotesSave : JobKind::NotesLoad; result.output_path = path; SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; };
        if (report) { result.saved = ExportReviewedHtml(*model,notes,inputs,path,control,&result.error); if (result.saved) result.text = L"Annotated comparison report saved: " + path; }
        else if (save) result.saved = SaveReviewNotes(path,notes,inputs,control,&result.error);
        else { result.review_notes = std::make_shared<ComparisonNotes>(); if (!ReadReviewNotes(path,result.review_notes.get(),&result.error)) result.review_notes.reset(); }
        if (!result.saved && !result.review_notes && result.error.empty()) result.error = L"Review metadata/report operation failed."; return result;
    }); } catch (...) { SetBusy(false); ReleaseLocks(); Status(L"Could not start review metadata operation."); return false; } return true;
}
void CAdvancedCompareView::FinishNotesIo(Result& result) {
    if (result.kind == JobKind::NotesLoad) {
        bool valid = result.review_notes && result.review_notes->fingerprint == review_fingerprint_;
        if (valid) for (const auto& note : result.review_notes->notes) if (!model_ || note.first >= model_->blocks.size() || !model_->blocks[note.first].changed) valid = false;
        if (!valid) { Status(L"Notes do not match these source snapshots and blocks. Existing notes retained."); return; }
        notes_ = std::move(*result.review_notes); notes_path_ = result.output_path; notes_dirty_ = false; rows_.Invalidate(FALSE); Status(L"Review notes loaded. Review state does not resolve merge conflicts.");
    } else if (result.saved) { notes_path_ = result.output_path; notes_dirty_ = false; Status(L"Review metadata saved: " + notes_path_); }
    if (changed) changed(); if (close_after_save_) { close_after_save_ = false; if (saved_close) saved_close(result.saved); }
}

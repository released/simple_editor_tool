#include "advanced_compare.h"
#include "review_note_dialog.h"
#include "localization.h"
#include "../resource.h"
using namespace simple_editor::core;
using simple_editor::ui::Localize;
bool CAdvancedCompareView::SetReviewNote(std::size_t block,const std::wstring& text,bool reviewed,std::wstring* error) {
    const auto fail = [&](const wchar_t* message) { if (error) *error = message; return false; };
    if (Busy() || stale_ || !model_ || block >= model_->blocks.size() || !model_->blocks[block].changed || review_fingerprint_.empty()) return fail(L"Choose a changed text block in a completed comparison.");
    if (!notes_.notes.empty() && notes_.fingerprint != review_fingerprint_) return fail(L"Existing notes belong to an older comparison. Save or explicitly clear them before adding new notes.");
    if (text.size() > 4096 || !ValidateText(text,error) || (!notes_.notes.count(block) && notes_.notes.size() >= 1024)) return fail(L"Review notes allow 1024 blocks and 4096 Unicode characters per note.");
    notes_.fingerprint = review_fingerprint_; if (text.empty() && !reviewed) notes_.notes.erase(block); else notes_.notes[block] = {text,reviewed}; notes_dirty_ = true; rows_.Invalidate(FALSE); if (changed) changed(); return true;
}
std::wstring CAdvancedCompareView::NoteCell(std::size_t block) const {
    if (notes_.notes.empty()) return L""; if (notes_.fingerprint != review_fingerprint_) return Localize(L"Notes from an older comparison"); const auto found = notes_.notes.find(block); if (found == notes_.notes.end()) return L"";
    auto text = found->second.text.substr(0,160); for (auto& c : text) if (c == L'\r' || c == L'\n') c = L' '; return Localize(found->second.reviewed ? L"Reviewed" : L"Pending review") + L": " + text;
}
void CAdvancedCompareView::NotesCommand(UINT command) {
    if (Busy()) return;
    if (command == ID_DOCUMENT_NOTES_CLEAR) { if (notes_.notes.empty()) return; modal_ = true; const auto answer = UiMessageBox(L"Clear these review notes? Source documents and merge choices are unchanged.",MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION); modal_ = false; if (answer == IDYES) { notes_.notes.clear(); if (!review_fingerprint_.empty()) notes_.fingerprint = review_fingerprint_; notes_dirty_ = true; rows_.Invalidate(FALSE); if (changed) changed(); } return; }
    if (command == ID_DOCUMENT_NOTE) {
        if (stale_ || !model_ || mode_ >= 4) { Status(L"Run a text comparison before annotating its differences."); return; }
        const int row = rows_.GetNextItem(-1,LVNI_SELECTED); if (row < 0 || static_cast<std::size_t>(row) >= visible_.size() || context_rows_.hidden.count(visible_[row])) { Status(L"Select a changed source row first."); return; }
        const auto block = text_rows_[visible_[row]].block; if (!model_->blocks[block].changed) { Status(L"Choose a changed text block."); return; }
        if (!notes_.notes.empty() && notes_.fingerprint != review_fingerprint_) { Status(L"Existing notes belong to an older comparison. Save or clear them first."); return; }
        const auto found = notes_.notes.find(block); const auto& range = model_->blocks[block]; CReviewNoteDialog dialog(this,found == notes_.notes.end() ? ReviewNote{} : found->second,L"Block " + std::to_wstring(block + 1) + L" | Left line " + std::to_wstring(range.left_begin + 1) + L" | Right line " + std::to_wstring(range.right_begin + 1));
        modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; std::wstring error; if (answer == IDOK && !SetReviewNote(block,dialog.note.text,dialog.note.reviewed,&error)) Status(error); return;
    }
    const bool load = command == ID_DOCUMENT_NOTES_LOAD,report = command == ID_DOCUMENT_NOTES_REPORT;
    if (load && notes_dirty_) { modal_ = true; const auto answer = UiMessageBox(L"Replace unsaved review notes with a metadata file?",MB_OKCANCEL | MB_ICONWARNING); modal_ = false; if (answer != IDOK) return; }
    CFileDialog dialog(load ? TRUE : FALSE,report ? L"html" : L"setreview",report ? L"comparison-review.html" : notes_path_.empty() ? L"comparison.setreview" : notes_path_.c_str(),load ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT,report ? L"HTML review (*.html)|*.html||" : L"Review metadata (*.setreview)|*.setreview||",this);
    modal_ = true; const auto answer = dialog.DoModal(); modal_ = false; if (answer == IDOK) StartNotesIo(dialog.GetPathName().GetString(),!load,report);
}
bool CAdvancedCompareView::ConfirmNotesClose() {
    if (!notes_dirty_) return true; modal_ = true; const auto answer = UiMessageBox(L"Review notes have unsaved changes. Save their metadata before closing?",MB_YESNOCANCEL | MB_ICONWARNING); modal_ = false;
    if (answer == IDNO) return true; if (answer == IDCANCEL) return false; NotesCommand(ID_DOCUMENT_NOTES_SAVE); close_after_save_ = job_.valid() && job_kind_ == JobKind::NotesSave; return false;
}

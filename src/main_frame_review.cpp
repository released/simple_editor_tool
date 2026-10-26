#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "core/line_merge.h"

using namespace simple_editor::core;
void CMainFrame::AppendReviewMenu(CMenu& menu) {
    CMenu review; review.CreatePopupMenu();
    AppendDocumentCommands(review);
    review.AppendMenu(MF_STRING,ID_REVIEW_DISK,L"Compare current draft with disk...");
    review.AppendMenu(MF_STRING,ID_REVIEW_OUTLINE,L"Markdown outline...");
    CMenu marks; marks.CreatePopupMenu(); marks.AppendMenu(MF_STRING,ID_REVIEW_MARK1,L"Mark all - yellow..."); marks.AppendMenu(MF_STRING,ID_REVIEW_MARK2,L"Mark all - cyan..."); marks.AppendMenu(MF_STRING,ID_REVIEW_MARK3,L"Mark all - pink..."); marks.AppendMenu(MF_STRING,ID_REVIEW_MARK_CLEAR,L"Clear text marks"); review.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(marks.Detach()),L"Text marks");
    review.AppendMenu(MF_STRING,ID_REVIEW_MOVED,L"Moved blocks in text comparison...");
    review.AppendMenu(MF_STRING,ID_REVIEW_BITS,L"Named bitfields / flags...");
    CMenu patch; patch.CreatePopupMenu(); patch.AppendMenu(MF_STRING,ID_REVIEW_IPS_EXPORT,L"Export IPS patch..."); patch.AppendMenu(MF_STRING,ID_REVIEW_IPS32_EXPORT,L"Export IPS32 patch..."); patch.AppendMenu(MF_STRING,ID_REVIEW_IPS_IMPORT,L"Preview / import IPS or IPS32..."); review.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(patch.Detach()),L"Binary patches");
    review.AppendMenu(MF_STRING,ID_REVIEW_EDITORCONFIG,L"Reload EditorConfig indentation");
    AppendExpansionCommands(review); menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(review.Detach()),L"&Review");
}
void CMainFrame::OnReview(UINT command) {
    if (command == ID_REVIEW_DISK) { StartDiskReview(); return; }
    if (command == ID_REVIEW_OUTLINE) { StartOutline(); return; }
    if (command == ID_REVIEW_MOVED) { if (AdvancedActive()) advanced_.ReviewMovedBlocks(); else status_.SetPaneText(0,L"Choose a two-way Text comparison."); return; }
    if (command == ID_REVIEW_BITS) { ReviewBitFields(); return; }
    if (command >= ID_REVIEW_IPS_EXPORT && command <= ID_REVIEW_IPS_IMPORT) { ReviewIps(command); return; }
    if (command == ID_REVIEW_EDITORCONFIG) { RequestIndentation(true); status_.SetPaneText(0,L"Reading the supported EditorConfig indentation rules..."); return; }
    if (command == ID_REVIEW_MARK_CLEAR) { if (auto* editor = ActiveEditor()) editor->ClearMarks(); return; }
    if (command >= ID_REVIEW_MARK1 && command <= ID_REVIEW_MARK3) {
        auto* editor = ActiveEditor(); if (IsBusy() || !editor) return;
        CValueDialog dialog(this,L"Mark all occurrences",L"Literal text (case-sensitive). Marks do not change text or Undo; editing clears all marks.",last_search_.find.GetString());
        dialog.validate = [](const std::wstring& value,std::wstring* error) { if (value.empty() || value.find_first_of(L"\r\n") != std::wstring::npos) { *error = L"Enter a nonempty single-line literal."; return false; } return true; };
        path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return;
        SearchRequest request; request.find = dialog.value.c_str(); request.action = SearchAction::FindAll; request.mark_colour = static_cast<int>(command - ID_REVIEW_MARK1); ExecuteEditorSearch(request,editor); return;
    }
    status_.SetPaneText(0,L"Choose the appropriate Text or Hex document for this review command.");
}
bool CMainFrame::StartDiskReview() {
    if (!workspace_ || IsBusy() || comparison_session_ || untitled_ || !document_.IsOpen() || text_stale_ || !text_editable_) { status_.SetPaneText(0,L"Choose a loaded, saved Text document to compare its current draft with disk."); return false; }
    const auto input = text_.Utf8(); if (input.size() > 16 * 1024 * 1024) { status_.SetPaneText(0,L"Draft comparison supports up to 16 MiB per side. The draft is retained."); return false; }
    const auto draft = TextFromEditControl(text_original_,CCodeEditor::FromUtf8(input)); const auto path = document_.Path(); const auto encoding = requested_encoding_;
    job_revision_ = revision_; job_editor_generation_ = text_.Generation(); job_kind_ = EditorJobKind::DiskReview; cancelled_ = false; progress_ = 0; SetSaveReadOnly(true);
    try { job_ = std::async(std::launch::async,[this,path,draft,encoding]() {
        EditorJobResult result; result.kind = EditorJobKind::DiskReview; result.message = draft; result.open_path = path;
        BinaryDocument disk; std::vector<std::uint8_t> bytes; TextCodec codec;
        if (!disk.Open(path,&result.error)) return result;
        if (disk.Size() > 16 * 1024 * 1024) { result.error = L"Disk comparison supports up to 16 MiB per side; draft retained."; return result; }
        if (cancelled_ || !disk.Read(0,static_cast<std::size_t>(disk.Size()),&bytes) || !DecodeText(bytes,encoding,&result.details,&codec,&result.error) || !disk.ValidateSource(true,&result.error) || !InspectFile(path,&result.stamp,&result.error) || disk.SourceIdentity() != std::make_pair(result.stamp.volume,result.stamp.identity) || !disk.ValidateSource(true,&result.error)) { if (result.error.empty()) result.error = L"Disk snapshot could not be read; draft retained."; }
        return result;
    }); } catch (...) { SetSaveReadOnly(false); status_.SetPaneText(0,L"Could not start disk comparison; draft retained."); return false; }
    status_.SetPaneText(0,L"Reading an independent disk snapshot. Current draft remains intact."); return true;
}
void CMainFrame::FinishDiskReview(EditorJobResult result) {
    SetSaveReadOnly(workspace_ && workspace_->DocumentLocked(document_));
    if (cancelled_ || revision_ != job_revision_ || text_.Generation() != job_editor_generation_) { status_.SetPaneText(0,L"Disk comparison cancelled or draft changed; no comparison opened."); return; }
    if (!result.error.empty()) { status_.SetPaneText(0,result.error.c_str()); return; }
    FileStamp now; std::wstring error;
    if (!InspectFile(result.open_path,&now,&error) || !SameFileState(now,result.stamp)) { status_.SetPaneText(0,L"Disk version changed after reading; repeat the comparison. Draft retained."); return; }
    workspace_->ShowDiskComparison(result.message,result.details,result.open_path,text_codec_,result.stamp);
}
void CWorkspaceFrame::ShowDiskComparison(const std::wstring& draft,const std::wstring& disk,const std::wstring& path,const TextCodec& codec,const FileStamp& stamp) {
    NewComparisonInputs(L"",L"",L"",8,false); if (active_session_ < 0) return;
    auto* frame = sessions_[active_session_]; frame->advanced_.SetSnapshotInputs(draft,disk,path,codec,stamp);
    frame->SetWindowTextW((L"Draft versus disk - " + std::filesystem::path(path).filename().wstring()).c_str()); RefreshLabels();
}

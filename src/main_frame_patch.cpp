#include "main_frame.h"
#include "resource.h"
#include "core/text_report.h"
void CMainFrame::OnExportTextReport() { if (AdvancedActive()) advanced_.ExportReportDialog(); else status_.SetPaneText(0,L"Open a text/three-way comparison to export a text report or patch."); }
void CMainFrame::OnApplyPatch() {
    if (IsBusy()) return; CFileDialog dialog(TRUE,L"patch",nullptr,OFN_FILEMUSTEXIST,L"Unified patches (*.patch;*.diff)|*.patch;*.diff|All files (*.*)|*.*||",this); path_dialog_ = true; const auto selected = dialog.DoModal(); path_dialog_ = false; if (selected == IDOK) StartPatch(dialog.GetPathName().GetString());
}
bool CMainFrame::StartPatch(const std::wstring& path) {
    if (IsBusy()) return false; if (active_tab_ == 0) ShowPage(1); auto* editor = ActiveEditor(); if (!editor || editor->Call(SCI_GETREADONLY)) { status_.SetPaneText(0,L"Choose an editable Text or merged-output buffer."); return false; }
    const auto raw = editor == &text_ ? simple_editor::core::TextFromEditControl(text_original_,CCodeEditor::FromUtf8(editor->Utf8())) : advanced_.RawOutputText(); job_editor_ = editor; job_editor_generation_ = editor->Generation(); job_kind_ = EditorJobKind::Patch; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX;
    if (editor == &text_) editor->SetReadOnly(TRUE); else advanced_.SearchBusy(true);
    try { job_ = std::async(std::launch::async,[this,path,raw]() { using namespace simple_editor::core; EditorJobResult result; result.kind = EditorJobKind::Patch; BinaryDocument document; std::vector<std::uint8_t> bytes; std::wstring patch; TextCodec codec;
        if (!document.Open(path,&result.error)) return result;
        if (document.Size() > 32 * 1024 * 1024) { result.error = L"Patch input exceeds 32 MiB."; return result; }
        if (!document.Read(0,static_cast<std::size_t>(document.Size()),&bytes) || !DecodeText(bytes,TextEncoding::Auto,&patch,&codec,&result.error) || !document.ValidateSource(true,&result.error)) { if (result.error.empty()) result.error = L"Could not read the patch input."; return result; }
        std::wstring updated; TextReportStats stats; if (!ApplyUnifiedPatch(raw,patch,cancelled_,&updated,&stats,&result.error)) return result;
        result.search = std::make_shared<NativeSearchPlan>(); result.search->changed = updated != raw; result.search->output = CCodeEditor::ToUtf8(TextForEditControl(updated)); result.search->count = stats.hunks;
        result.details = L"Patch: " + path + L"\r\nValidated hunks: " + std::to_wstring(stats.hunks) + L"\r\nAdded lines: " + std::to_wstring(stats.added) + L"\r\nRemoved lines: " + std::to_wstring(stats.removed) + L"\r\n\r\nApply to the current editor as one Undo action. Save separately to write a file. Existing line styles are retained; new lines use the source's first style.\r\n\r\nPatch preview (first 100 lines, 200 characters per line):\r\n";
        const auto lines = SplitLines(patch); for (std::size_t i = 0; i < (std::min<std::size_t>)(100,lines.size()); ++i) result.details += lines[i].text.substr(0,200) + L"\r\n"; return result;
    }); } catch (...) { UnlockEditorSearch(); status_.SetPaneText(0,L"Could not start patch validation."); return false; } return true;
}
void CMainFrame::FinishPatch(EditorJobResult result) {
    auto* editor = job_editor_; const auto generation = job_editor_generation_; const bool valid = editor && editor->GetSafeHwnd() && editor->Generation() == generation; UnlockEditorSearch();
    if (!valid || cancelled_) { status_.SetPaneText(0,L"Patch cancelled or editor changed; no patch applied."); return; }
    if (!result.error.empty() || !result.search) { status_.SetPaneText(0,result.error.empty() ? L"Patch validation failed." : result.error.c_str()); return; }
    CTextPreviewDialog preview(this,result.details); path_dialog_ = true; const auto accepted = preview.DoModal(); path_dialog_ = false;
    if (accepted != IDOK || editor->Generation() != generation || editor->Call(SCI_GETREADONLY)) { status_.SetPaneText(0,L"Patch preview cancelled or editor changed; no patch applied."); return; }
    if (result.search->changed) { editor->ReplaceGenerated(CCodeEditor::FromUtf8(result.search->output),0); editor->GetParent()->SendMessage(WM_COMMAND,MAKEWPARAM(editor->GetDlgCtrlID(),EN_CHANGE),reinterpret_cast<LPARAM>(editor->GetSafeHwnd())); }
    status_.SetPaneText(0,L"Validated patch applied to the editor. Undo restores the previous buffer; Save writes the file.");
}

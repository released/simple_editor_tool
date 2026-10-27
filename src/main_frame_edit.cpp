#include "main_frame.h"
#include "ui/localization.h"
#include "resource.h"
#include "core/search.h"
#include "workspace_frame.h"
#include <algorithm>

using namespace simple_editor::core;

void CMainFrame::RefreshText() {
    if (!text_.GetSafeHwnd()) return;
    suppress_text_change_ = true; text_dirty_ = false; text_editable_ = false;
    text_.SetReadOnly(TRUE);
    std::wstring value, error; std::vector<std::uint8_t> bytes;
    if (!document_.IsOpen()) value.clear();
    else if (document_.Size() > 32ULL * 1024 * 1024) value = L"Text editing is limited to 32 MiB. Choose Tools > Large text pages for a bounded read-only view, or use Hex mode.";
    else if (!document_.Read(0, static_cast<std::size_t>(document_.Size()), &bytes)) value = L"Could not read the document. Use Hex mode.";
    else if (!DecodeText(bytes, requested_encoding_, &value, &text_codec_, &error)) value = error;
    else { text_editable_ = true; text_.SetReadOnly(MutationBlocked()); }
    text_original_ = text_editable_ ? value : L"";
    text_.SetWindowTextW(TextForEditControl(value).c_str()); text_.SetModify(FALSE);
    ConfigureFileSyntax();
    text_stale_ = false; suppress_text_change_ = false;
}

void CMainFrame::OnTextChanged() {
    if (!suppress_text_change_ && text_editable_) { text_dirty_ = true; DocumentChanged(); text_stale_ = false; function_sidebar_.InvalidateSource(); }
}

bool CMainFrame::ApplyTextEdits() {
    if (!text_dirty_ && !text_.GetModify()) return true;
    if (IsSaving()) return false;
    if (!text_editable_) return false;
    CString value; text_.GetWindowTextW(value); std::vector<std::uint8_t> bytes; std::wstring error;
    const auto restored = TextFromEditControl(text_original_,std::wstring(value.GetString(), value.GetLength()));
    if (!EncodeText(restored, text_codec_, true, &bytes, &error)) {
        UiMessageBox(error.c_str(), MB_ICONERROR); return false;
    }
    if (!document_.Replace(0, document_.Size(), bytes)) { UiMessageBox(L"Could not apply text changes.", MB_ICONERROR); return false; }
    text_original_ = restored;
    text_dirty_ = false; text_.SetModify(FALSE); DocumentChanged(); text_stale_ = false;
    hex_.GoTo((std::min)(hex_.CursorOffset(), document_.Size())); return true;
}

void CMainFrame::DocumentChanged() {
    bookmarks_.stale = true; modified_ranges_.clear(); modified_revision_ = UINT64_MAX; hex_.Invalidate();
    ++revision_; cancelled_ = true; text_stale_ = true; match_offset_ = UINT64_MAX;
    match_pattern_.clear(); ClearComparison();
    CString caption; caption.Format(L"%s * - Simple Editor Tool", (untitled_ ? untitled_name_ : document_.Path()).c_str()); SetWindowTextW(caption);
    if (workspace_) workspace_->RefreshLabels();
}

void CMainFrame::OnUndo() {
    if (AdvancedActive()) { advanced_.Undo(); return; }
    if (MutationBlocked()) return;
    if (active_tab_ == 1 && ActiveEditor() && ActiveEditor()->CanUndo()) { ActiveEditor()->Undo(); return; }
    if (!ApplyTextEdits()) return;
    std::uint64_t offset = 0; if (document_.Undo(&offset)) { DocumentChanged(); hex_.GoTo(offset); if (active_tab_ == 1) RefreshText(); }
}
void CMainFrame::OnRedo() {
    if (AdvancedActive()) { advanced_.Undo(true); return; }
    if (MutationBlocked()) return;
    if (active_tab_ == 1 && ActiveEditor() && ActiveEditor()->CanRedo()) { ActiveEditor()->Redo(); return; }
    if (!ApplyTextEdits()) return;
    std::uint64_t offset = 0; if (document_.Redo(&offset)) { DocumentChanged(); hex_.GoTo(offset); if (active_tab_ == 1) RefreshText(); }
}
void CMainFrame::OnInsert() {
    if (MutationBlocked()) return;
    if (!document_.IsOpen() || !ApplyTextEdits()) return;
    const auto at = hex_.CursorOffset(); if (document_.Insert(at, {0})) { DocumentChanged(); hex_.GoTo(at); ShowPage(0); }
}
void CMainFrame::OnDelete() {
    if (MutationBlocked()) return;
    if (!ApplyTextEdits()) return;
    if (hex_.DeleteSelection()) ShowPage(0);
}

void CMainFrame::OnEncoding(UINT id) {
    if (AdvancedActive()) { advanced_.SetEncoding(static_cast<TextEncoding>(id - ID_ENCODING_AUTO)); return; }
    if (MutationBlocked()) return;
    if (!ApplyTextEdits()) return;
    requested_encoding_ = static_cast<TextEncoding>(id - ID_ENCODING_AUTO);
    session_menu_.CheckMenuRadioItem(ID_ENCODING_AUTO, ID_ENCODING_SYSTEM, id, MF_BYCOMMAND);
    text_stale_ = true; ShowPage(1);
}

void CMainFrame::OnFind() { ActiveHex(); ActiveEditor(); if (workspace_) { workspace_->ShowFindReplace(false); return; } if (document_.IsOpen() || AdvancedActive()) search_dialog_.Show(false, !AdvancedActive() && active_tab_ != 1, this); }
void CMainFrame::OnReplace() { ActiveHex(); ActiveEditor(); if (workspace_) { workspace_->ShowFindReplace(true); return; } if (document_.IsOpen() || AdvancedActive()) search_dialog_.Show(true, !AdvancedActive() && active_tab_ != 1, this); }
void CMainFrame::OnFindNext() {
    if (last_search_.find.IsEmpty()) { OnFind(); return; }
    auto request = last_search_; request.action = SearchAction::FindNext; request.previous = (GetKeyState(VK_SHIFT) & 0x8000) != 0; ExecuteSearch(request);
}
void CMainFrame::OnGoTo() {
    if (ActiveEditor()) { OnEditorCommand(ID_VIEW_GOTO_LINE); return; }
    if (IsSaving()) { status_.SetPaneText(0, L"Wait for saving to finish before opening Go To."); return; }
    if (!document_.IsOpen() || !ApplyTextEdits()) return;
    CHexView* target = &hex_; BinaryDocument* document = &document_;
    if (active_tab_ == 2) {
        if (GetFocus() == &compare_right_) { target = &compare_right_; document = &compare_right_document_; }
        else { target = &compare_left_; document = &compare_left_document_; }
    }
    if (!document->IsOpen()) return;
    CAddressDialog dialog(document->Size(), this); dialog.address = target->CursorOffset();
    if (dialog.DoModal() == IDOK) { if (active_tab_ != 2) ShowPage(0); target->GoTo(dialog.address); target->SetFocus(); match_offset_ = UINT64_MAX; }
}

void CMainFrame::ExecuteSearch(const SearchRequest& request) {
    if (request.all_open && workspace_) { workspace_->ExecuteOpenSearch(request); return; }
    if (!request.hex && !ActiveEditor() && !AdvancedActive() && (request.mode >= 2 || !request.match_case || request.whole_word || request.previous || request.selection_only || !request.wrap || request.action == SearchAction::Count || request.action == SearchAction::FindAll)) { ShowPage(1); if (!text_editable_) { status_.SetPaneText(0,L"Choose a valid text encoding or use Hex search."); return; } }
    if (!request.hex && ActiveEditor()) { ExecuteEditorSearch(request,ActiveEditor()); return; }
    if (request.hex && (request.mode >= 4 || request.previous || request.selection_only || !request.wrap || request.action == SearchAction::Count || request.action == SearchAction::FindAll)) { ExecuteByteSearch(request); return; }
    if (AdvancedActive()) { if (request.action == SearchAction::FindNext) advanced_.Find(request.find.GetString()); else status_.SetPaneText(0,L"Choose Text mode to search the merged output."); return; }
    if (MutationBlocked() && request.action != SearchAction::FindNext) return;
    if (comparison_session_ && request.action != SearchAction::FindNext) { status_.SetPaneText(0, L"Comparison snapshots are read-only. Edit the original file tab."); return; }
    if (!document_.IsOpen()) return;
    if (IsBusy()) { status_.SetPaneText(0, L"An operation is running. Cancel it or wait for completion."); return; }
    if (!ApplyTextEdits()) return;
    std::vector<std::uint8_t> pattern, replacement; std::wstring error;
    if (request.hex) {
        if (!ParseHexBytes(request.find.GetString(), &pattern) || (request.action != SearchAction::FindNext && !ParseHexBytes(request.replacement.GetString(), &replacement))) error = L"Enter pairs of hexadecimal digits, for example: 00 FF 41.";
    } else {
        if (text_stale_) RefreshText();
        if (!text_editable_) { status_.SetPaneText(0, L"Text search needs a valid text encoding. Select an encoding or use Hex bytes."); return; }
        EncodeText(request.find.GetString(), text_codec_, false, &pattern, &error);
        if (error.empty() && request.action != SearchAction::FindNext) EncodeText(request.replacement.GetString(), text_codec_, false, &replacement, &error);
    }
    if (!error.empty() || pattern.empty()) { UiMessageBox(error.empty() ? L"Enter a non-empty search pattern." : error.c_str(), MB_ICONWARNING); return; }
    last_search_ = request;
    if (request.action == SearchAction::Replace && match_offset_ != UINT64_MAX && match_pattern_ == pattern && hex_.CursorOffset() == match_offset_) {
        std::vector<std::uint8_t> current; const auto at = match_offset_;
        if (document_.Read(at, pattern.size(), &current) && current == pattern && document_.Replace(at, pattern.size(), replacement)) {
            DocumentChanged(); hex_.GoTo(at + replacement.size()); ShowPage(0); hex_.SetFocus(); status_.SetPaneText(0, L"One match replaced. F3 finds the next match."); return;
        }
    }
    const bool all = request.action == SearchAction::ReplaceAll;
    auto snapshot = document_.Snapshot(&error, all);
    if (!snapshot) { UiMessageBox(error.c_str(), MB_ICONERROR); return; }
    std::uint64_t start = hex_.CursorOffset();
    if (match_offset_ != UINT64_MAX && match_pattern_ == pattern && start == match_offset_) start = (std::min<std::uint64_t>)(document_.Size(), start + pattern.size());
    const bool aligned = !request.hex && (text_codec_.encoding == TextEncoding::Utf16Le || text_codec_.encoding == TextEncoding::Utf16Be);
    CPINFO cp_info{}; const UINT code_page = text_codec_.code_page;
    const bool dbcs = !request.hex && !aligned && GetCPInfo(code_page, &cp_info) && cp_info.MaxCharSize == 2;
    job_revision_ = revision_; job_kind_ = all ? EditorJobKind::ReplaceAll : EditorJobKind::Find; cancelled_ = false; progress_ = 0; match_pattern_ = pattern;
    status_.SetPaneText(0, all ? L"Replacing all matches... Esc cancels." : L"Searching... Esc cancels.");
    job_ = std::async(std::launch::async, [this, snapshot = std::move(snapshot), pattern, replacement, start, all, aligned, dbcs, code_page]() mutable {
        EditorJobResult result; result.kind = all ? EditorJobKind::ReplaceAll : EditorJobKind::Find;
        std::vector<std::uint64_t> offsets;
        std::vector<bool> boundaries;
        if (dbcs) {
            std::vector<std::uint8_t> content;
            if (!snapshot->Read(0, static_cast<std::size_t>(snapshot->Size()), &content)) { result.error = L"Could not read text boundaries."; return result; }
            boundaries.assign(content.size() + 1, false);
            for (std::size_t i = 0; i < content.size();) { boundaries[i] = true; i += IsDBCSLeadByteEx(code_page, content[i]) ? 2 : 1; }
            boundaries.back() = true;
        }
        std::uint64_t next_allowed = 0;
        auto match = [&](std::uint64_t at) {
            if (!boundaries.empty() && (!boundaries[static_cast<std::size_t>(at)] || !boundaries[static_cast<std::size_t>(at + pattern.size())])) return true;
            if (!all) { result.found = at; return false; }
            if (at >= next_allowed) { offsets.push_back(at); next_allowed = at + pattern.size(); }
            return !cancelled_;
        };
        ScanMatches(*snapshot, pattern, all ? 0 : start, snapshot->Size(), cancelled_, match, &result.error, aligned ? 2 : 1);
        if (!all && result.found == UINT64_MAX && !cancelled_ && result.error.empty() && start) {
            result.wrapped = true;
            ScanMatches(*snapshot, pattern, 0, (std::min<std::uint64_t>)(snapshot->Size(), start + pattern.size() - 1), cancelled_, [&](std::uint64_t at) { return at < start ? match(at) : false; }, &result.error, aligned ? 2 : 1);
        }
        if (all && !cancelled_ && result.error.empty()) {
            result.count = offsets.size();
            if (!snapshot->ReplaceMatches(offsets, pattern, replacement)) result.error = L"Could not replace all matches.";
            else result.document = std::move(snapshot);
        }
        return result;
    });
}

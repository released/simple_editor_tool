#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "core/text_byte_map.h"
#include "ui/feature_dialogs.h"
#include <algorithm>
using namespace simple_editor::core;
void CMainFrame::CreateSourceNotice() {
    source_notice_.Create(L"Disk version changed. The current Text draft is retained.",WS_CHILD,CRect(),this,594);
    source_keep_.Create(L"Keep draft",WS_CHILD | WS_TABSTOP,CRect(),this,ID_WORKFLOW_SOURCE_KEEP);
    source_compare_.Create(L"Compare with disk",WS_CHILD | WS_TABSTOP,CRect(),this,ID_WORKFLOW_SOURCE_COMPARE);
    source_reload_.Create(L"Reload...",WS_CHILD | WS_TABSTOP,CRect(),this,ID_WORKFLOW_SOURCE_RELOAD);
    for (CWnd* control : {static_cast<CWnd*>(&source_notice_),static_cast<CWnd*>(&source_keep_),static_cast<CWnd*>(&source_compare_),static_cast<CWnd*>(&source_reload_)}) control->SendMessage(WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE);
}
void CMainFrame::LayoutSourceNotice(CRect& page) {
    if (!source_notice_.GetSafeHwnd()) return;
    const bool show = active_tab_ == 1 && source_notice_visible_ && !comparison_session_;
    for (CWnd* control : {static_cast<CWnd*>(&source_notice_),static_cast<CWnd*>(&source_keep_),static_cast<CWnd*>(&source_compare_),static_cast<CWnd*>(&source_reload_)}) control->ShowWindow(show ? SW_SHOW : SW_HIDE);
    if (!show) return;
    source_notice_.MoveWindow(page.left + 4,page.top + 2,(std::max)(0,page.Width() - 8),22);
    source_keep_.MoveWindow(page.left + 4,page.top + 26,105,25); source_compare_.MoveWindow(page.left + 115,page.top + 26,150,25); source_reload_.MoveWindow(page.left + 271,page.top + 26,90,25);
    page.top = (std::min)(page.bottom,page.top + 57);
}
void CMainFrame::CheckSourceNotice(const std::wstring& error) {
    std::wstring key;
    if (!error.empty() && !comparison_session_ && !untitled_) { FileStamp disk; std::wstring ignored; InspectFile(document_.Path(),&disk,&ignored); key = document_.Path() + L":" + std::to_wstring(disk.exists) + L":" + std::to_wstring(disk.volume) + L":" + std::to_wstring(disk.identity) + L":" + std::to_wstring(disk.size) + L":" + std::to_wstring(disk.modified); }
    const bool show = !key.empty() && key != source_dismissed_key_; const bool changed = show != source_notice_visible_;
    if (key.empty()) source_dismissed_key_.clear(); source_notice_key_ = std::move(key); source_notice_visible_ = show;
    if (changed) Layout();
    source_compare_.EnableWindow(!IsBusy() && text_editable_ && !text_stale_); source_reload_.EnableWindow(!IsBusy());
}
bool CMainFrame::StartSelectionMap(bool to_hex) {
    if (IsBusy() || comparison_session_ || !document_.IsOpen() || (to_hex && active_tab_ != 1) || (!to_hex && active_tab_ != 0)) return false;
    std::wstring error; if (!document_.ValidateSource(true,&error)) { status_.SetPaneText(0,error.c_str()); return false; }
    auto* editor = to_hex && ActiveEditor() ? ActiveEditor() : static_cast<CCodeEditor*>(&text_);
    std::uint64_t begin,end;
    if (to_hex) { int a,b; editor->GetSel(a,b); begin = a; end = b; if (!ApplyTextEdits()) return false; }
    else { begin = hex_.SelectionLength() ? hex_.SelectionStart() : hex_.CursorOffset(); end = begin + hex_.SelectionLength(); ShowPage(1); }
    if (!text_editable_ || text_stale_ || !document_.ValidateSource(true,&error)) { status_.SetPaneText(0,error.empty() ? L"A current, decoded Text buffer is required for position mapping." : error.c_str()); return false; }
    const auto raw = TextFromEditControl(text_original_,CCodeEditor::FromUtf8(text_.Utf8())); const auto codec = text_codec_;
    job_editor_ = editor; job_editor_generation_ = editor->Generation(); job_revision_ = revision_; job_kind_ = EditorJobKind::SelectionMap; cancelled_ = false;
    job_ = std::async(std::launch::async,[this,raw,codec,to_hex,begin,end]() { EditorJobResult result; result.kind = EditorJobKind::SelectionMap; result.wrapped = to_hex; result.selection_map = std::make_shared<TextByteRange>(); SaveControl control; control.cancelled = &cancelled_; MapTextByteRange(raw,codec,true == to_hex,begin,end,control,result.selection_map.get(),&result.error); return result; });
    return true;
}
bool CMainFrame::StartUnicodeScan(bool explicit_scan) {
    auto* editor = ActiveEditor(); if (IsBusy() || !editor) return false;
    const auto text = editor->Utf8(); if (text.size() > 32 * 1024 * 1024) { status_.SetPaneText(0,L"Unicode scan is limited to 32 MiB; the buffer is unchanged."); return false; }
    job_editor_ = editor; job_editor_generation_ = editor->Generation(); job_revision_ = revision_; job_kind_ = EditorJobKind::UnicodeScan; cancelled_ = false;
    const auto allowed = unicode_allowed_;
    job_ = std::async(std::launch::async,[this,text,allowed,explicit_scan]() { EditorJobResult result; result.kind = EditorJobKind::UnicodeScan; result.wrapped = explicit_scan; result.unicode = std::make_shared<UnicodeNotices>(); SaveControl control; control.cancelled = &cancelled_; ScanUnicodeNotices(text,allowed,control,result.unicode.get(),&result.error); return result; }); return true;
}
void CMainFrame::TickUnicode() {
    if (!unicode_live_ || comparison_session_ || active_tab_ != 1 || !text_editable_ || text_stale_) return;
    const auto generation = text_.Generation(),now = GetTickCount64();
    if (generation != unicode_generation_) { unicode_generation_ = generation; unicode_tick_ = now + 350; }
    if (unicode_tick_ && now >= unicode_tick_ && !IsBusy()) { unicode_tick_ = 0; StartUnicodeScan(false); }
}
void CMainFrame::FinishExpansion(EditorJobResult result) {
    if (result.kind == EditorJobKind::Statistics) { FinishStatistics(std::move(result)); return; }
    auto* editor = job_editor_;
    if (cancelled_ || revision_ != job_revision_ || !editor || !editor->GetSafeHwnd() || editor->Generation() != job_editor_generation_) { status_.SetPaneText(0,L"Operation cancelled or buffer changed; stale result discarded."); return; }
    if (!result.error.empty()) { status_.SetPaneText(0,result.error.c_str()); return; }
    if (result.kind == EditorJobKind::SelectionMap) {
        std::wstring error; if (!document_.ValidateSource(true,&error)) { status_.SetPaneText(0,error.c_str()); return; }
        const auto& range = *result.selection_map;
        if (result.wrapped) { ShowPage(0); hex_.SelectRange(range.byte_begin,range.byte_end - range.byte_begin); hex_.SetFocus(); }
        else { ShowPage(1); text_.SetSel(static_cast<int>(range.text_begin),static_cast<int>(range.text_end)); text_.Call(SCI_SCROLLCARET); text_.SetFocus(); }
        status_.SetPaneText(0,L"Selection mapped to complete characters and physical bytes, including original BOM/EOL."); return;
    }
    if (unicode_live_) { editor->ShowUnicodeNotices(*result.unicode,unicode_allowed_); if (IsFileEditor(editor) && text_clone_.GetSafeHwnd()) { text_.ShowUnicodeNotices(*result.unicode,unicode_allowed_); text_clone_.ShowUnicodeNotices(*result.unicode,unicode_allowed_); } }
    status_.SetPaneText(0,(std::to_wstring(result.unicode->total) + L" unusual Unicode character(s); first 100,000 locations available.").c_str());
    if (!result.wrapped) return;
    std::vector<std::vector<std::wstring>> rows;
    for (const auto& notice : result.unicode->rows) { wchar_t point[16]{}; swprintf_s(point,L"U+%04X",notice.point); rows.push_back({point,notice.name,std::to_wstring(notice.line),std::to_wstring(notice.column),std::to_wstring(notice.offset)}); }
    CReviewList dialog(this,L"Unicode notices - scalar columns / original UTF-8 offsets",{L"Point",L"Name",L"Line",L"Column",L"Byte offset"},std::move(rows)); dialog.accept_label = L"Go to character"; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
    if (answer == IDOK && !dialog.selected.empty() && editor->Generation() == job_editor_generation_) { const auto& notice = result.unicode->rows[dialog.selected[0]]; editor->Call(SCI_SETSEL,editor->NativePosition(notice.offset),editor->NativePosition(notice.offset + notice.length)); editor->Call(SCI_SCROLLCARET); editor->SetFocus(); }
}

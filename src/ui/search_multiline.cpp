#include "editor_dialogs.h"
#include "native_search.h"
#include "../resource.h"

void CSearchDialog::InitializeMultiline() {
    original_bounds_.clear(); GetWindowRect(original_window_); multiline_ = false;
    for (auto* control = GetWindow(GW_CHILD); control; control = control->GetNextWindow()) {
        CRect bounds; control->GetWindowRect(bounds); ScreenToClient(bounds); wchar_t name[64]{}; ::GetClassNameW(control->GetSafeHwnd(),name,64);
        if (wcscmp(name,L"ComboBox") == 0) { CRect dropped; static_cast<CComboBox*>(control)->GetDroppedControlRect(dropped); bounds.bottom = bounds.top + dropped.Height(); }
        original_bounds_.push_back({control->GetSafeHwnd(),bounds});
    }
    const DWORD style = WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;
    multiline_find_.Create(style,CRect(),this,1020); multiline_replacement_.Create(style,CRect(),this,1021);
    for (auto* edit : {&multiline_find_,&multiline_replacement_}) { edit->SetFont(GetFont()); edit->SetLimitText(32768); }
}
void CSearchDialog::OnMultiline() {
    const bool next = IsDlgButtonChecked(IDC_SEARCH_MULTILINE) != 0; if (next == multiline_) return;
    CString find,replacement;
    if (next) {
        GetDlgItemTextW(IDC_SEARCH_TEXT,find); GetDlgItemTextW(IDC_REPLACE_TEXT,replacement);
        if (static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_MODE))->GetCurSel() == 2) {
            std::wstring a,b,error; if (!DecodeSearchEscapes(find.GetString(),&a,&error) || !DecodeSearchEscapes(replacement.GetString(),&b,&error)) { CheckDlgButton(IDC_SEARCH_MULTILINE,BST_UNCHECKED); SetDlgItemTextW(IDC_SEARCH_STATUS,error.c_str()); return; } find = a.c_str(); replacement = b.c_str();
        }
        multiline_find_.SetWindowTextW(find); multiline_replacement_.SetWindowTextW(replacement); static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_MODE))->SetCurSel(0);
    } else {
        multiline_find_.GetWindowTextW(find); multiline_replacement_.GetWindowTextW(replacement);
        if (find.FindOneOf(L"\r\n") >= 0 || replacement.FindOneOf(L"\r\n") >= 0) { CheckDlgButton(IDC_SEARCH_MULTILINE,BST_CHECKED); SetDlgItemTextW(IDC_SEARCH_STATUS,L"Remove line breaks before switching to single-line input. Both multiline values are retained."); return; }
        SetDlgItemTextW(IDC_SEARCH_TEXT,find); SetDlgItemTextW(IDC_REPLACE_TEXT,replacement);
    }
    CRect full(0,0,0,52),half(0,0,0,26),threshold(0,58,0,58),replace_top(0,30,0,30),height(0,0,0,40); MapDialogRect(full); MapDialogRect(half); MapDialogRect(threshold); MapDialogRect(replace_top); MapDialogRect(height);
    for (const auto& original : original_bounds_) {
        auto bounds = original.second; const int shift = bounds.top >= threshold.top ? full.bottom : bounds.top >= replace_top.top && bounds.top < threshold.top ? half.bottom : 0;
        if (next) bounds.OffsetRect(0,shift); ::MoveWindow(original.first,bounds.left,bounds.top,bounds.Width(),bounds.Height(),TRUE);
        if (::GetDlgCtrlID(original.first) == IDC_SEARCH_TEXT) multiline_find_.MoveWindow(bounds.left,bounds.top,bounds.Width(),height.bottom);
        if (::GetDlgCtrlID(original.first) == IDC_REPLACE_TEXT) multiline_replacement_.MoveWindow(bounds.left,bounds.top,bounds.Width(),height.bottom);
    }
    multiline_ = next; GetDlgItem(IDC_SEARCH_TEXT)->ShowWindow(next ? SW_HIDE : SW_SHOW); GetDlgItem(IDC_REPLACE_TEXT)->ShowWindow(next ? SW_HIDE : SW_SHOW);
    multiline_find_.ShowWindow(next ? SW_SHOW : SW_HIDE); multiline_replacement_.ShowWindow(next ? SW_SHOW : SW_HIDE); GetDlgItem(IDC_SEARCH_MODE)->EnableWindow(!next);
    SetWindowPos(nullptr,0,0,original_window_.Width(),original_window_.Height() + (next ? full.bottom : 0),SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    SetDlgItemTextW(IDC_SEARCH_STATUS,next ? L"Paste multiline literal text directly. Enter inserts a line break; use Find Next / Replace buttons. Regex remains single-line." : L"Current document or all open Text drafts; replacements never save files.");
    if (next) multiline_find_.SetFocus(); else GetDlgItem(IDC_SEARCH_TEXT)->SetFocus();
}

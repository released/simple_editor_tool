#include "editor_dialogs.h"
#include "../resource.h"
#include "../core/search.h"
#include "../core/line_merge.h"
#include <afxcmn.h>

BEGIN_MESSAGE_MAP(CSearchDialog, CDialog)
    ON_BN_CLICKED(IDC_REPLACE_ONE, OnReplace)
    ON_BN_CLICKED(IDC_REPLACE_ALL, OnReplaceAll)
    ON_BN_CLICKED(IDC_SEARCH_COUNT, OnCount)
    ON_BN_CLICKED(IDC_SEARCH_ALL, OnFindAll)
    ON_BN_CLICKED(IDC_SEARCH_PREVIOUS, OnPrevious)
END_MESSAGE_MAP()

CSearchDialog::CSearchDialog() : CDialog(IDD_SEARCH) {}
CNativeNoticesDialog::CNativeNoticesDialog(CWnd* parent) : CDialog(IDD_NATIVE_NOTICES,parent) {}
BOOL CNativeNoticesDialog::OnInitDialog() {
    CDialog::OnInitDialog(); std::wstring notice = L"Simple Editor Tool\r\nNative MFC x64 application\r\n\r\nScintilla 5.6.7 and Lexilla 5.5.4 are statically linked.\r\n\r\n";
    const auto resource = FindResourceW(AfxGetResourceHandle(),MAKEINTRESOURCEW(IDR_NATIVE_EDITOR_LICENSE),RT_RCDATA);
    const auto memory = resource ? LoadResource(AfxGetResourceHandle(),resource) : nullptr; const auto* data = memory ? static_cast<const char*>(LockResource(memory)) : nullptr;
    if (data) { const auto count = static_cast<int>(SizeofResource(AfxGetResourceHandle(),resource)); const int length = MultiByteToWideChar(CP_UTF8,0,data,count,nullptr,0); std::wstring text(length,L'\0'); MultiByteToWideChar(CP_UTF8,0,data,count,text.data(),length); notice += simple_editor::core::TextForEditControl(text); }
    SetDlgItemTextW(IDC_NATIVE_NOTICES,notice.c_str()); return TRUE;
}
CTextResultDialog::CTextResultDialog(CWnd* parent, std::wstring title, std::wstring text) : CDialog(IDD_NATIVE_NOTICES,parent), title_(std::move(title)), text_(std::move(text)) {}
BOOL CTextResultDialog::OnInitDialog() { CDialog::OnInitDialog(); SetWindowTextW(title_.c_str()); SetDlgItemTextW(IDC_NATIVE_NOTICES,text_.c_str()); return TRUE; }
CTextPreviewDialog::CTextPreviewDialog(CWnd* parent,std::wstring text) : CDialog(IDD_TEXT_PREVIEW,parent), text_(std::move(text)) {}
BOOL CTextPreviewDialog::OnInitDialog() { CDialog::OnInitDialog(); static_cast<CEdit*>(GetDlgItem(IDC_TEXT_PREVIEW))->SetLimitText(65536); SetDlgItemTextW(IDC_TEXT_PREVIEW,text_.c_str()); return TRUE; }
BOOL CSearchDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    auto* mode = static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_MODE));
    for (auto label : {L"Text (current encoding)",L"Hex bytes (e.g. 00 FF 41)",L"Text escapes (\\n \\t \\xHH \\uHHHH)",L"Regex (Scintilla POSIX; one line)",L"Hex wildcards (AA ?F ??)",L"Typed number (e.g. u32le:1234)"}) mode->AddString(label); mode->SetCurSel(0);
    static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_TEXT))->LimitText(32768);
    static_cast<CEdit*>(GetDlgItem(IDC_REPLACE_TEXT))->SetLimitText(32768);
    CheckDlgButton(IDC_SEARCH_CASE,BST_CHECKED); CheckDlgButton(IDC_SEARCH_WRAP,BST_CHECKED);
    return TRUE;
}
void CSearchDialog::Show(bool replace, bool hex, CWnd* owner) {
    const bool first = !GetSafeHwnd();
    if (first && !Create(IDD_SEARCH, owner)) return;
    SetWindowTextW(replace ? L"Find / Replace" : L"Find");
    GetDlgItem(IDC_REPLACE_ONE)->EnableWindow(replace); GetDlgItem(IDC_REPLACE_ALL)->EnableWindow(replace);
    if (first) static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_MODE))->SetCurSel(hex ? 1 : 0);
    ShowWindow(SW_SHOW); SetForegroundWindow(); GetDlgItem(IDC_SEARCH_TEXT)->SetFocus();
}
void CSearchDialog::Dispatch(SearchAction action) {
    SearchRequest request; GetDlgItemTextW(IDC_SEARCH_TEXT, request.find); GetDlgItemTextW(IDC_REPLACE_TEXT, request.replacement);
    request.mode = static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_MODE))->GetCurSel(); request.hex = request.mode == 1 || request.mode >= 4; request.action = action;
    request.match_case = IsDlgButtonChecked(IDC_SEARCH_CASE) != 0; request.whole_word = IsDlgButtonChecked(IDC_SEARCH_WORD) != 0; request.selection_only = IsDlgButtonChecked(IDC_SEARCH_SELECTION) != 0; request.wrap = IsDlgButtonChecked(IDC_SEARCH_WRAP) != 0;
    auto* history = static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_TEXT)); if (!request.find.IsEmpty() && history->FindStringExact(-1,request.find) == CB_ERR) { history->InsertString(0,request.find); if (history->GetCount() > 20) history->DeleteString(20); }
    if (execute) execute(request);
}
void CSearchDialog::OnOK() { Dispatch(SearchAction::FindNext); }
void CSearchDialog::OnCancel() { ShowWindow(SW_HIDE); }
void CSearchDialog::OnReplace() { Dispatch(SearchAction::Replace); }
void CSearchDialog::OnReplaceAll() { Dispatch(SearchAction::ReplaceAll); }
void CSearchDialog::OnCount() { Dispatch(SearchAction::Count); }
void CSearchDialog::OnFindAll() { Dispatch(SearchAction::FindAll); }
void CSearchDialog::OnPrevious() { SearchRequest request; GetDlgItemTextW(IDC_SEARCH_TEXT,request.find); GetDlgItemTextW(IDC_REPLACE_TEXT,request.replacement); request.mode = static_cast<CComboBox*>(GetDlgItem(IDC_SEARCH_MODE))->GetCurSel(); request.hex = request.mode == 1 || request.mode >= 4; request.previous = true; request.match_case = IsDlgButtonChecked(IDC_SEARCH_CASE) != 0; request.whole_word = IsDlgButtonChecked(IDC_SEARCH_WORD) != 0; request.selection_only = IsDlgButtonChecked(IDC_SEARCH_SELECTION) != 0; request.wrap = IsDlgButtonChecked(IDC_SEARCH_WRAP) != 0; if (execute) execute(request); }

CAddressDialog::CAddressDialog(std::uint64_t limit, CWnd* owner) : CDialog(IDD_GOTO, owner), limit_(limit) {}
CValueDialog::CValueDialog(CWnd* owner, const std::wstring& title, const std::wstring& prompt, const std::wstring& initial) : CDialog(IDD_VALUE_INPUT,owner), value(initial), title_(title), prompt_(prompt) {}
BOOL CValueDialog::OnInitDialog() {
    CDialog::OnInitDialog(); SetWindowTextW(title_.c_str()); SetDlgItemTextW(IDC_VALUE_PROMPT,prompt_.c_str()); SetDlgItemTextW(IDC_VALUE_TEXT,value.c_str()); static_cast<CEdit*>(GetDlgItem(IDC_VALUE_TEXT))->SetLimitText(32767);
    auto* label = GetDlgItem(IDC_VALUE_PROMPT); if (!label || !label->GetSafeHwnd()) return TRUE;
    CRect bounds; label->GetWindowRect(bounds); ScreenToClient(bounds); CRect measured(0,0,bounds.Width(),0); CClientDC dc(label); auto* font = label->GetFont(); auto* previous = font ? dc.SelectObject(font) : nullptr;
    dc.DrawTextW(prompt_.c_str(),static_cast<int>(prompt_.size()),measured,DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX); if (previous) dc.SelectObject(previous);
    const int extra = (std::max)(0,measured.Height() + 4 - bounds.Height());
    if (extra) { label->MoveWindow(bounds.left,bounds.top,bounds.Width(),bounds.Height() + extra); for (int id : {IDC_VALUE_TEXT,IDOK,IDCANCEL}) { auto* control = GetDlgItem(id); if (!control || !control->GetSafeHwnd()) continue; CRect rect; control->GetWindowRect(rect); ScreenToClient(rect); control->MoveWindow(rect.left,rect.top + extra,rect.Width(),rect.Height()); } CRect window; GetWindowRect(window); SetWindowPos(nullptr,0,0,window.Width(),window.Height() + extra,SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE); CenterWindow(); }
    return TRUE;
}
void CValueDialog::OnOK() { CString text; GetDlgItemTextW(IDC_VALUE_TEXT,text); std::wstring error; if (validate && !validate(text.GetString(),&error)) { AfxMessageBox(error.empty() ? L"Enter a valid value." : error.c_str(),MB_ICONWARNING); return; } value = text.GetString(); CDialog::OnOK(); }
BOOL CAddressDialog::OnInitDialog() {
    CDialog::OnInitDialog(); CString initial; initial.Format(L"%llX", address); SetDlgItemTextW(IDC_ADDRESS, initial);
    static_cast<CEdit*>(GetDlgItem(IDC_ADDRESS))->SetSel(0, -1);
    return TRUE;
}
void CAddressDialog::OnOK() {
    CString input; GetDlgItemTextW(IDC_ADDRESS, input);
    if (!simple_editor::core::ParseAddress(input.GetString(), &address) || address > limit_) {
        CString message; message.Format(L"Enter an address from 0 to %llX (hex). Use d: for decimal.", limit_);
        AfxMessageBox(message, MB_ICONWARNING); return;
    }
    CDialog::OnOK();
}

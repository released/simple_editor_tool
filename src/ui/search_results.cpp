#include "search_results.h"
#include "code_editor.h"
#include "../resource.h"
BEGIN_MESSAGE_MAP(CSearchResultsDialog,CDialog)
    ON_NOTIFY(LVN_GETDISPINFO,IDC_SEARCH_RESULTS,OnDisplay)
    ON_NOTIFY(NM_DBLCLK,IDC_SEARCH_RESULTS,OnActivate)
    ON_WM_SIZE()
END_MESSAGE_MAP()
BOOL CSearchResultsDialog::OnInitDialog() { CDialog::OnInitDialog(); list_.SubclassDlgItem(IDC_SEARCH_RESULTS,this); list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER); list_.InsertColumn(0,L"Byte offset",LVCFMT_LEFT,155); list_.InsertColumn(1,L"Line",LVCFMT_LEFT,80); list_.InsertColumn(2,L"Preview",LVCFMT_LEFT,550); CRect bounds; GetClientRect(bounds); Layout(bounds.Width(),bounds.Height()); return TRUE; }
void CSearchResultsDialog::Show(CWnd* owner,std::vector<NativeSearchMatch> matches,const std::wstring& title) { matches_ = std::move(matches); if (!GetSafeHwnd() && !Create(IDD_SEARCH_RESULTS,owner)) return; SetWindowTextW(title.c_str()); list_.SetItemCountEx(static_cast<int>(matches_.size())); ShowWindow(SW_SHOW); }
void CSearchResultsDialog::OnDisplay(NMHDR* header,LRESULT* result) { *result = 0; auto* info = reinterpret_cast<NMLVDISPINFO*>(header); const auto index = info->item.iItem; if (!(info->item.mask & LVIF_TEXT) || index < 0 || static_cast<std::size_t>(index) >= matches_.size()) return; const auto& match = matches_[index]; std::wstring text; if (info->item.iSubItem == 0) { wchar_t value[32]{}; swprintf_s(value,L"0x%llX",match.offset); text = value; } else if (info->item.iSubItem == 1) text = std::to_wstring(match.line); else text = CCodeEditor::FromUtf8(match.preview); wcsncpy_s(info->item.pszText,info->item.cchTextMax,text.c_str(),_TRUNCATE); }
void CSearchResultsDialog::OnOK() { const int selected = list_.GetNextItem(-1,LVNI_SELECTED); if (selected >= 0 && static_cast<std::size_t>(selected) < matches_.size() && navigate) navigate(matches_[selected]); }
void CSearchResultsDialog::OnActivate(NMHDR*,LRESULT* result) { *result = 0; OnOK(); }
void CSearchResultsDialog::OnSize(UINT type,int cx,int cy) { CDialog::OnSize(type,cx,cy); Layout(cx,cy); }
void CSearchResultsDialog::Layout(int cx,int cy) { if (!list_.GetSafeHwnd()) return; list_.MoveWindow(8,8,(std::max)(0,cx - 16),(std::max)(0,cy - 50)); if (auto* close = GetDlgItem(IDCANCEL)) close->MoveWindow((std::max)(8,cx - 108),(std::max)(8,cy - 34),100,26); }

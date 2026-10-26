#include "live_inspector.h"
#include "../resource.h"
BEGIN_MESSAGE_MAP(CLiveInspector,CDialog)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_NOTIFY(LVN_ITEMCHANGED,2500,OnSelect)
END_MESSAGE_MAP()
void CLiveInspector::Show(CWnd* owner) { if (!GetSafeHwnd()) Create(IDD_LIVE_INSPECTOR,owner); ShowWindow(SW_SHOW); Tick(); }
BOOL CLiveInspector::OnInitDialog() { CDialog::OnInitDialog(); list_.SubclassDlgItem(2500,this); list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER); list_.InsertColumn(0,L"Type",LVCFMT_LEFT,110); list_.InsertColumn(1,L"Value",LVCFMT_LEFT,390); endian_.SubclassDlgItem(2501,this); endian_.AddString(L"Little endian"); endian_.AddString(L"Big endian"); endian_.SetCurSel(0); GetDlgItem(IDOK)->EnableWindow(FALSE); SetTimer(1,150,nullptr); return TRUE; }
void CLiveInspector::Tick() { if (!inspect) return; current_ = inspect(endian_.GetCurSel() != 1); const auto selected = list_.GetNextItem(-1,LVNI_SELECTED); list_.SetRedraw(FALSE); for (std::size_t i = 0; i < current_.values.size(); ++i) { if (static_cast<int>(i) >= list_.GetItemCount()) list_.InsertItem(static_cast<int>(i),current_.values[i].type.c_str()); list_.SetItemText(static_cast<int>(i),1,current_.values[i].value.c_str()); } while (list_.GetItemCount() > static_cast<int>(current_.values.size())) list_.DeleteItem(list_.GetItemCount() - 1); list_.SetRedraw(TRUE); wchar_t offset[64]{}; swprintf_s(offset,L"Physical offset: 0x%llX",current_.offset); SetDlgItemTextW(2503,current_.error.empty() ? offset : current_.error.c_str()); if (selected < 0 || current_.error.size()) GetDlgItem(IDOK)->EnableWindow(FALSE); }
void CLiveInspector::OnSelect(NMHDR* header,LRESULT* result) { *result = 0; auto* event = reinterpret_cast<NMLISTVIEW*>(header); if (!(event->uNewState & LVIS_SELECTED) || event->iItem < 0 || static_cast<std::size_t>(event->iItem) >= current_.values.size()) return; const auto& value = current_.values[event->iItem]; editing_ = current_; editing_little_ = endian_.GetCurSel() != 1; type_ = value.type; auto number = value.value.substr(0,value.value.find(L" (")); SetDlgItemTextW(2502,number.c_str()); GetDlgItem(IDOK)->EnableWindow(current_.writable && value.valid && value.editable); }
void CLiveInspector::OnOK() { CString text; GetDlgItemTextW(2502,text); std::wstring error; if (!write || !write(editing_,type_,text.GetString(),editing_little_,&error)) { SetDlgItemTextW(2503,error.empty() ? L"Select a current writable field first." : error.c_str()); return; } Tick(); GetDlgItem(IDOK)->EnableWindow(FALSE); }
void CLiveInspector::OnCancel() { ShowWindow(SW_HIDE); }
void CLiveInspector::OnTimer(UINT_PTR id) { if (id == 1 && IsWindowVisible()) Tick(); else CDialog::OnTimer(id); }
void CLiveInspector::OnDestroy() { KillTimer(1); CDialog::OnDestroy(); }

#include "log_tail_dialog.h"
#include "../resource.h"
using namespace simple_editor::core;
BEGIN_MESSAGE_MAP(CLogTailDialog,CDialog)
    ON_WM_TIMER()
    ON_WM_DESTROY()
END_MESSAGE_MAP()
CLogTailDialog::~CLogTailDialog() { Stop(); }
void CLogTailDialog::Show(CWnd* owner,const std::filesystem::path& path,TextEncoding encoding) { Stop(); path_ = path; stamp_ = {}; last_ = 0; paused_ = false; if (!GetSafeHwnd()) Create(IDD_LOG_TAIL,owner); encoding_.SetCurSel(encoding == TextEncoding::Utf16Le ? 2 : encoding == TextEncoding::Utf16Be ? 3 : encoding == TextEncoding::Utf8 ? 1 : 0); SetDlgItemTextW(IDOK,L"Pause"); ShowWindow(SW_SHOW); Tick(); }
BOOL CLogTailDialog::OnInitDialog() { CDialog::OnInitDialog(); CRect area; GetDlgItem(2600)->GetWindowRect(area); ScreenToClient(area); GetDlgItem(2600)->ShowWindow(SW_HIDE); editor_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL,area,this,2604); editor_.SetReadOnly(TRUE); encoding_.SubclassDlgItem(2601,this); for (auto label : {L"Auto BOM / UTF-8",L"UTF-8",L"UTF-16 LE",L"UTF-16 BE"}) encoding_.AddString(label); encoding_.SetCurSel(0); CheckDlgButton(2602,BST_CHECKED); SetTimer(1,50,nullptr); return TRUE; }
void CLogTailDialog::Tick() {
    if (job_.valid() && job_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) { std::pair<LogTailResult,std::wstring> result; try { result = job_.get(); } catch (...) { result.second = L"Log read failed; the previous view was retained."; }
        if (!result.second.empty()) SetDlgItemTextW(2603,result.second.c_str()); else { if (result.first.changed) { stamp_ = result.first.stamp; editor_.SetWindowTextW(result.first.page.text.c_str()); editor_.SetReadOnly(TRUE); editor_.Call(SCI_EMPTYUNDOBUFFER); if (IsDlgButtonChecked(2602)) editor_.Call(SCI_GOTOPOS,editor_.Call(SCI_GETLENGTH)); } SetDlgItemTextW(2603,(result.first.state + L" | Last 256 KiB | Read only | Page-relative line numbers | File editor drafts are retained").c_str()); } }
    if (job_.valid() || paused_ || !IsWindowVisible() || GetTickCount64() - last_ < 750) return; last_ = GetTickCount64(); cancel_ = false; const auto path = path_; const auto previous = stamp_; const auto encoding = static_cast<TextEncoding>((std::max)(0,encoding_.GetCurSel()));
    try { job_ = std::async(std::launch::async,[this,path,previous,encoding] { std::pair<LogTailResult,std::wstring> result; SaveControl control; control.cancelled = &cancel_; ReadLogTail(path,encoding,previous,control,&result.first,&result.second); return result; }); } catch (...) { SetDlgItemTextW(2603,L"Could not start the log reader."); }
}
void CLogTailDialog::Stop() { cancel_ = true; if (job_.valid()) { job_.wait(); try { job_.get(); } catch (...) {} } }
BOOL CLogTailDialog::OnCommand(WPARAM wp,LPARAM lp) { if (LOWORD(wp) == 2601 && HIWORD(wp) == CBN_SELCHANGE) { Stop(); stamp_ = {}; last_ = 0; Tick(); return TRUE; } return CDialog::OnCommand(wp,lp); }
void CLogTailDialog::OnOK() { paused_ = !paused_; SetDlgItemTextW(IDOK,paused_ ? L"Resume" : L"Pause"); if (!paused_) { stamp_ = {}; last_ = 0; Tick(); } }
void CLogTailDialog::OnCancel() { Stop(); ShowWindow(SW_HIDE); }
void CLogTailDialog::OnTimer(UINT_PTR id) { if (id == 1) { if (IsWindowVisible()) Tick(); } else CDialog::OnTimer(id); }
void CLogTailDialog::OnDestroy() { KillTimer(1); Stop(); CDialog::OnDestroy(); }

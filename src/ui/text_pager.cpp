#include "text_pager.h"
#include "../resource.h"
#include "editor_dialogs.h"
#include "localization.h"
#include "../core/search.h"
#include <algorithm>
using namespace simple_editor::core;
BEGIN_MESSAGE_MAP(CTextPager,CDialog)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()
CTextPager::CTextPager(CWnd* owner,std::unique_ptr<BinaryDocument> source,TextEncoding encoding) : CDialog(IDD_TEXT_PAGER,owner),source_(std::move(source)),encoding_(encoding) {}
CTextPager::~CTextPager() { cancelled_ = true; if (job_.valid()) job_.wait(); }
BOOL CTextPager::OnInitDialog() {
    CDialog::OnInitDialog(); CRect rect; GetDlgItem(2300)->GetWindowRect(rect); ScreenToClient(rect); GetDlgItem(2300)->ShowWindow(SW_HIDE); editor_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL,rect,this,2304); editor_.SetReadOnly(TRUE); editor_.SetWrap(false); editor_.ConfigureLanguage(source_->Path()); SetTimer(1,40,nullptr); Load(0,true,0);
    MONITORINFO monitor{sizeof(monitor)}; if (GetMonitorInfoW(MonitorFromWindow(GetSafeHwnd(),MONITOR_DEFAULTTONEAREST),&monitor)) { CRect window; GetWindowRect(window); const auto width = (std::min)(window.Width(),static_cast<int>(monitor.rcWork.right - monitor.rcWork.left) - 16),height = (std::min)(window.Height(),static_cast<int>(monitor.rcWork.bottom - monitor.rcWork.top) - 16); const auto x = (std::clamp)(window.left,monitor.rcWork.left,monitor.rcWork.right - width),y = (std::clamp)(window.top,monitor.rcWork.top,monitor.rcWork.bottom - height); SetWindowPos(nullptr,x,y,width,height,SWP_NOZORDER | SWP_NOACTIVATE); } Layout(); return TRUE;
}
void CTextPager::Layout() {
    if (!editor_.GetSafeHwnd()) return; CRect client,status,close,padding(0,0,8,8); GetClientRect(client); MapDialogRect(padding); GetDlgItem(2301)->GetWindowRect(status); GetDlgItem(IDCANCEL)->GetWindowRect(close);
    const int width = (std::max)(32L,client.Width() - 2 * padding.right),bottom = client.bottom - padding.bottom - status.Height(),height = close.Height(); int x = padding.right,y = padding.bottom;
    CClientDC dc(this); auto* previous = GetFont() ? dc.SelectObject(GetFont()) : nullptr;
    for (int id : {2302,2303,2305,2306,2307,2308,IDCANCEL}) { CString text; GetDlgItemTextW(id,text); const int button_width = (std::min)(width,(std::max)(70,static_cast<int>(dc.GetTextExtent(text).cx) + 24)); if (x > padding.right && x + button_width > client.right - padding.right) { x = padding.right; y += height + 5; } GetDlgItem(id)->MoveWindow(x,y,button_width,height); x += button_width + 5; }
    if (previous) dc.SelectObject(previous); const int top = y + height + padding.bottom;
    GetDlgItem(2301)->MoveWindow(padding.right,bottom,width,status.Height()); editor_.MoveWindow(padding.right,top,width,(std::max)(32,static_cast<int>(bottom - padding.bottom - top)));
}
void CTextPager::OnSize(UINT type,int cx,int cy) { CDialog::OnSize(type,cx,cy); Layout(); }
void CTextPager::OnGetMinMaxInfo(MINMAXINFO* info) { CDialog::OnGetMinMaxInfo(info); CRect minimum(0,0,480,200); MapDialogRect(minimum); info->ptMinTrackSize.x = minimum.Width(); info->ptMinTrackSize.y = minimum.Height(); }
bool CTextPager::Load(std::uint64_t offset,bool known,int direction) {
    if (job_.valid()) return false; if (!select_match_) next_match_ = offset; cancelled_ = false; old_begin_ = page_.begin; direction_ = direction;
    try { job_ = std::async(std::launch::async,[this,offset,known] { Result result; SaveControl control; control.cancelled = &cancelled_; ReadTextPage(*source_,offset,encoding_,known,control,&result.page,&result.error); return result; }); }
    catch (...) { SetDlgItemTextW(2301,L"Could not start the page reader."); return false; }
    for (int id : {2302,2303,2305,2306,2307,2308}) GetDlgItem(id)->EnableWindow(FALSE); SetDlgItemTextW(2301,L"Reading a bounded text page..."); return true;
}
void CTextPager::Poll() {
    if (!job_.valid() || job_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return; Result result;
    try { result = job_.get(); } catch (...) { result.error = L"The page could not be read; the previous page was retained."; }
    if (closing_) { CDialog::OnCancel(); return; }
    if (result.searching) {
        for (int id : {2302,2303,2305,2306,2307,2308}) GetDlgItem(id)->EnableWindow(TRUE);
        if (cancelled_ || !result.error.empty()) { SetDlgItemTextW(2301,cancelled_ ? L"Search cancelled; current page retained." : result.error.c_str()); return; }
        next_match_ = result.after;
        if (result.found == UINT64_MAX) { SetDlgItemTextW(2301,L"No further literal matches to end of file. Ctrl+F starts from the current page."); return; }
        select_match_ = true; if (!Load(result.found,false,0)) select_match_ = false; return;
    }
    if (result.error.empty()) { if (direction_ > 0) { if (previous_.size() == 4096) previous_.erase(previous_.begin()); previous_.push_back(old_begin_); } else if (direction_ < 0 && !previous_.empty()) previous_.pop_back(); else if (!direction_) previous_.clear(); page_ = std::move(result.page); editor_.SetReadOnly(FALSE); editor_.SetWindowTextW(page_.text.c_str()); editor_.SetReadOnly(TRUE); editor_.SetModify(FALSE); editor_.Call(SCI_EMPTYUNDOBUFFER); }
    wchar_t text[256]{}; swprintf_s(text,L"Bytes 0x%llX - 0x%llX of 0x%llX | %d%% | Read only | Line numbers restart on each page",page_.begin,page_.end,page_.total,zoom_); SetDlgItemTextW(2301,result.error.empty() ? text : result.error.c_str());
    GetDlgItem(2302)->EnableWindow(!previous_.empty()); GetDlgItem(2303)->EnableWindow(page_.end < source_->Size()); GetDlgItem(2305)->EnableWindow(TRUE); GetDlgItem(2306)->EnableWindow(TRUE);
    GetDlgItem(2307)->EnableWindow(TRUE); GetDlgItem(2308)->EnableWindow(TRUE);
    if (select_match_) { if (result.error.empty()) { editor_.Call(SCI_SETSEL,0,CCodeEditor::ToUtf8(query_).size()); editor_.Call(SCI_SCROLLCARET); wchar_t message[160]{}; swprintf_s(message,L"Literal match at byte 0x%llX. F3 finds the next match.",page_.begin); SetDlgItemTextW(2301,message); } select_match_ = false; }
}
void CTextPager::OnTimer(UINT_PTR id) { if (id == 1) { const auto language = simple_editor::ui::UiLanguage(); if (language != ui_language_) { ui_language_ = language; Layout(); } Poll(); } else CDialog::OnTimer(id); }
BOOL CTextPager::OnCommand(WPARAM wparam,LPARAM lparam) {
    switch (LOWORD(wparam)) {
    case 2302: if (!previous_.empty()) Load(previous_.back(),true,-1); return TRUE;
    case 2303: Load(page_.end,true,1); return TRUE;
    case 2305: { CAddressDialog address(source_->Size(),this); if (address.DoModal() == IDOK) Load(address.address,false,0); return TRUE; }
    case 2306: Load(0,true,0); return TRUE;
    case 2307: FindDialog(); return TRUE;
    case 2308: if (query_.empty()) FindDialog(); else FindNext(); return TRUE;
    } return CDialog::OnCommand(wparam,lparam);
}
void CTextPager::Zoom(int delta) { zoom_ = (std::clamp)(zoom_ + delta,50,300); for (unsigned style = 0; style <= STYLE_MAX; ++style) editor_.Call(SCI_STYLESETSIZEFRACTIONAL,style,zoom_ * 10); if (!job_.valid()) { wchar_t text[256]{}; swprintf_s(text,L"Bytes 0x%llX - 0x%llX of 0x%llX | %d%% | Read only | Line numbers restart on each page",page_.begin,page_.end,page_.total,zoom_); SetDlgItemTextW(2301,text); } }
BOOL CTextPager::PreTranslateMessage(MSG* message) { if (message->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000) && message->wParam == 'F') { FindDialog(); return TRUE; } if (message->message == WM_KEYDOWN && message->wParam == VK_F3) { if (query_.empty()) FindDialog(); else FindNext(); return TRUE; } if (message->message == WM_MOUSEWHEEL && (LOWORD(message->wParam) & MK_CONTROL)) { Zoom(GET_WHEEL_DELTA_WPARAM(message->wParam) > 0 ? 10 : -10); return TRUE; } if (message->message == WM_KEYDOWN && message->wParam == VK_ESCAPE && job_.valid()) { cancelled_ = true; return TRUE; } return CDialog::PreTranslateMessage(message); }
void CTextPager::OnCancel() { if (job_.valid()) { closing_ = true; cancelled_ = true; return; } CDialog::OnCancel(); }
void CTextPager::OnDestroy() { KillTimer(1); cancelled_ = true; if (job_.valid()) job_.wait(); CDialog::OnDestroy(); }

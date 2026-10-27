#include "function_sidebar.h"
#include "localization.h"
#include "../resource.h"
#include <algorithm>
using namespace simple_editor::core;
BEGIN_MESSAGE_MAP(CFunctionSidebar,CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_NOTIFY(LVN_GETDISPINFO,IDC_FUNCTION_LIST,OnDisplay)
    ON_NOTIFY(NM_CLICK,IDC_FUNCTION_LIST,OnActivate)
    ON_NOTIFY(NM_DBLCLK,IDC_FUNCTION_LIST,OnActivate)
    ON_NOTIFY(LVN_KEYDOWN,IDC_FUNCTION_LIST,OnKey)
END_MESSAGE_MAP()
bool CFunctionSidebar::Create(CWnd* owner) {
    const auto cls = AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1),nullptr);
    if (!CWnd::Create(cls,L"",WS_CHILD | WS_CLIPCHILDREN,CRect(),owner,IDC_FUNCTION_SIDEBAR)) return false;
    simple_editor::ui::RegisterLocalizationRoot(this); return true;
}
int CFunctionSidebar::OnCreate(LPCREATESTRUCT create) {
    if (CWnd::OnCreate(create) == -1) return -1;
    if (!caption_.Create(L"Functions",WS_CHILD | WS_VISIBLE,CRect(),this,IDC_FUNCTION_CAPTION) || !status_.Create(L"Updating function list...",WS_CHILD | WS_VISIBLE,CRect(),this,IDC_FUNCTION_STATUS) || !list_.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,CRect(),this,IDC_FUNCTION_LIST)) return -1;
    auto* font = CFont::FromHandle(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))); caption_.SetFont(font); status_.SetFont(font); list_.SetFont(font);
    list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
    list_.InsertColumn(0,L"Function",LVCFMT_LEFT,240); list_.InsertColumn(1,L"Line",LVCFMT_RIGHT,55); list_.EnableWindow(FALSE); return 0;
}
CFunctionSidebar::~CFunctionSidebar() { Stop(); }
void CFunctionSidebar::Stop() { cancelled_ = true; if (job_.valid()) { job_.wait(); try { job_.get(); } catch (...) {} } }
void CFunctionSidebar::OnDestroy() { Stop(); CWnd::OnDestroy(); }
void CFunctionSidebar::OnSize(UINT type,int width,int height) { CWnd::OnSize(type,width,height); Layout(); }
void CFunctionSidebar::Layout() {
    if (!list_.GetSafeHwnd()) return; CRect rect; GetClientRect(rect); const int width = (std::max)(0,rect.Width() - 8),height = (std::max)(0,rect.Height());
    caption_.MoveWindow(4,2,width,(std::min)(22,height)); const int top = (std::min)(26,height),bottom = (std::max)(top,height - 44);
    list_.MoveWindow(4,top,width,bottom - top); status_.MoveWindow(4,bottom + 3,width,(std::max)(0,height - bottom - 3)); list_.SetColumnWidth(0,(std::max)(180,width - 62)); list_.SetColumnWidth(1,55);
}
void CFunctionSidebar::Status(const std::wstring& text) { if (status_.GetSafeHwnd()) status_.SetWindowTextW(text.c_str()); }
void CFunctionSidebar::InvalidateSource() { ready_ = false; failed_ = false; cancelled_ = true; changed_at_ = GetTickCount64(); if (list_.GetSafeHwnd()) { list_.EnableWindow(FALSE); Status(L"Updating function list..."); } }
void CFunctionSidebar::RefreshNow() { InvalidateSource(); changed_at_ = GetTickCount64() - 300; }
bool CFunctionSidebar::Current(const CCodeEditor& editor,std::uint64_t revision,const std::wstring& path,OutlineLanguage language) const { return ready_ && generation_ == editor.Generation() && revision_ == revision && path_ == path && language_ == language; }
void CFunctionSidebar::Tick(CCodeEditor& editor,std::uint64_t revision,const std::wstring& path,OutlineLanguage language,bool start,unsigned theme) {
    if (!GetSafeHwnd()) return;
    if (theme_ != theme) { theme_ = theme; const auto back = theme == 2 ? GetSysColor(COLOR_WINDOW) : theme == 1 ? RGB(63,63,63) : RGB(255,255,255),fore = theme == 2 ? GetSysColor(COLOR_WINDOWTEXT) : theme == 1 ? RGB(225,228,235) : RGB(30,35,42); list_.SetBkColor(back); list_.SetTextBkColor(back); list_.SetTextColor(fore); }
    if (generation_ != editor.Generation() || revision_ != revision || path_ != path || language_ != language) { generation_ = editor.Generation(); revision_ = revision; path_ = path; language_ = language; InvalidateSource(); }
    if (job_.valid() && job_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        Result result; try { result = job_.get(); } catch (...) { failed_ = true; Status(L"Function analysis failed; edit or refresh to retry."); }
        if (!cancelled_ && result.ok && result.generation == generation_ && result.revision == revision_ && result.path == path_ && result.language == language_) {
            const auto top = list_.GetTopIndex(); entries_ = std::move(result.entries); ready_ = true; list_.SetItemCountEx(static_cast<int>(entries_.size()),LVSICF_NOSCROLL); list_.EnableWindow(TRUE); list_.Invalidate(FALSE); if (top >= 0 && static_cast<std::size_t>(top) < entries_.size()) list_.EnsureVisible(top,FALSE);
            Status(result.truncated ? L"Partial list: parsing or result limit reached." : entries_.empty() ? L"No supported functions or declarations found." : L"Definitions and declarations. Click or press Enter to jump.");
        } else if (!cancelled_ && !result.ok) { failed_ = true; Status(L"Function analysis failed; edit or refresh to retry."); }
    }
    if (!start || ready_ || failed_ || job_.valid() || GetTickCount64() - changed_at_ < 300) return;
    if (editor.Call(SCI_GETLENGTH) > 32 * 1024 * 1024) { failed_ = true; Status(L"Function sidebar supports up to 32 MiB of editor text."); return; }
    auto text = editor.Utf8(); const auto generation = generation_,captured_revision = revision_; const auto captured_path = path_; cancelled_ = false; Status(L"Reading functions and declarations...");
    try { job_ = std::async(std::launch::async,[this,text = std::move(text),generation,captured_revision,captured_path,language] { Result result; result.generation = generation; result.revision = captured_revision; result.path = captured_path; result.language = language; result.ok = ProgramFunctionOutline(CCodeEditor::FromUtf8(text),language,cancelled_,&result.entries,&result.truncated); return result; }); }
    catch (...) { failed_ = true; Status(L"Function analysis failed; edit or refresh to retry."); }
}
void CFunctionSidebar::OnDisplay(NMHDR* header,LRESULT* result) {
    *result = 0; auto* info = reinterpret_cast<NMLVDISPINFO*>(header); if (!(info->item.mask & LVIF_TEXT) || info->item.iItem < 0 || static_cast<std::size_t>(info->item.iItem) >= entries_.size() || info->item.cchTextMax <= 0) return;
    const auto& entry = entries_[info->item.iItem]; const auto text = info->item.iSubItem == 0 ? entry.title : std::to_wstring(entry.line); wcsncpy_s(info->item.pszText,info->item.cchTextMax,text.c_str(),_TRUNCATE);
}
void CFunctionSidebar::OnActivate(NMHDR*,LRESULT* result) { *result = 0; const auto row = list_.GetNextItem(-1,LVNI_SELECTED); if (ready_ && row >= 0 && static_cast<std::size_t>(row) < entries_.size() && navigate) navigate(entries_[row].line); }
void CFunctionSidebar::OnKey(NMHDR* header,LRESULT* result) { *result = 0; if (reinterpret_cast<NMLVKEYDOWN*>(header)->wVKey == VK_RETURN) OnActivate(header,result); }

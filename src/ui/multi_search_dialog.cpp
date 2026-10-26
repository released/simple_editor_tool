#include "multi_search_dialog.h"
#include "code_editor.h"
#include "editor_dialogs.h"
#include "../resource.h"
#include <chrono>
using namespace simple_editor::services;
using namespace simple_editor::core;
BEGIN_MESSAGE_MAP(CMultiSearchDialog,CDialog)
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_NOTIFY(LVN_GETDISPINFO,1909,OnDisplay)
    ON_NOTIFY(NM_DBLCLK,1909,OnActivate)
END_MESSAGE_MAP()
CMultiSearchDialog::~CMultiSearchDialog() { Stop(); }
void CMultiSearchDialog::Stop() { cancel_ = true; available_.notify_all(); if (job_.valid()) { try { job_.get(); } catch (...) {} } }
void CMultiSearchDialog::Show(CWnd* owner) { if (!GetSafeHwnd() && !Create(IDD_MULTI_SEARCH,owner)) return; ShowWindow(SW_SHOW); SetForegroundWindow(); }
bool CMultiSearchDialog::Present(CWnd* owner,std::vector<MultiSearchMatch> matches,const std::wstring& status) {
    if (Busy()) return false; Show(owner); if (!GetSafeHwnd()) return false;
    matches_ = std::move(matches); summary_ = {}; list_.SetItemCountEx(static_cast<int>(matches_.size())); EnableInputs(true); SetDlgItemTextW(1914,status.c_str()); return true;
}
BOOL CMultiSearchDialog::OnInitDialog() {
    CDialog::OnInitDialog(); mode_.SubclassDlgItem(1901,this); encoding_.SubclassDlgItem(1902,this); scope_.SubclassDlgItem(1903,this); list_.SubclassDlgItem(1909,this);
    for (auto value : {L"Text literal",L"Text escaped",L"Text regex (POSIX)",L"Hex wildcards",L"Typed numeric"}) mode_.AddString(value); mode_.SetCurSel(0);
    for (auto value : {L"Auto",L"UTF-8",L"UTF-16 LE",L"UTF-16 BE",L"Big5 / CP950",L"System"}) encoding_.AddString(value); encoding_.SetCurSel(0);
    scope_.AddString(L"All open file editors"); scope_.AddString(L"Folder on disk"); scope_.SetCurSel(0);
    CheckDlgButton(1910,BST_CHECKED); CheckDlgButton(1911,BST_CHECKED); SetDlgItemTextW(1906,L"*");
    list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER); list_.InsertColumn(0,L"Path",LVCFMT_LEFT,360); list_.InsertColumn(1,L"Line",LVCFMT_RIGHT,75); list_.InsertColumn(2,L"Hex address / UTF-8 position",LVCFMT_LEFT,220); list_.InsertColumn(3,L"Preview",LVCFMT_LEFT,600);
    SetTimer(1,50,nullptr); EnableInputs(true); return TRUE;
}
void CMultiSearchDialog::EnableInputs(bool enabled) {
    for (int id : {1900,1901,1902,1903,1904,1905,1906,1907,1910,1911,1912,IDOK}) if (auto* control = GetDlgItem(id)) control->EnableWindow(enabled);
    if (auto* control = GetDlgItem(1913)) control->EnableWindow(!enabled);
    if (auto* control = GetDlgItem(1908)) control->EnableWindow(enabled && !summary_.details.empty());
}
bool CMultiSearchDialog::Start(MultiSearchRequest request,const std::filesystem::path& folder,std::vector<MultiSearchSource> sources) {
    if (Busy() || request.text.find.empty()) return false;
    cancel_ = false; files_ = 0; matches_.clear(); summary_ = {}; { std::lock_guard<std::mutex> lock(mutex_); pending_.clear(); } list_.SetItemCountEx(0); EnableInputs(false); SetDlgItemTextW(1914,L"Searching snapshots; results arrive in bounded batches...");
    try { job_ = std::async(std::launch::async,[this,request,folder,sources = std::move(sources)]() mutable {
        return SearchManyFiles(std::move(sources),folder,request,cancel_,[this](std::vector<MultiSearchMatch> batch) {
            std::unique_lock<std::mutex> lock(mutex_); available_.wait(lock,[this]() { return cancel_ || pending_.size() < 16; }); if (cancel_) return false; pending_.push_back(std::move(batch)); return true;
        },[this](std::uint64_t count) { files_ = count; });
    }); } catch (...) { EnableInputs(true); SetDlgItemTextW(1914,L"Could not start the search worker."); return false; } return true;
}
void CMultiSearchDialog::Tick() {
    bool empty;
    { std::lock_guard<std::mutex> lock(mutex_); for (unsigned i = 0; i < 4 && !pending_.empty(); ++i) { auto batch = std::move(pending_.front()); pending_.pop_front(); matches_.insert(matches_.end(),std::make_move_iterator(batch.begin()),std::make_move_iterator(batch.end())); } empty = pending_.empty(); }
    available_.notify_all(); list_.SetItemCountEx(static_cast<int>(matches_.size()));
    if (!job_.valid()) return;
    if (empty && job_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
        { std::lock_guard<std::mutex> lock(mutex_); if (!pending_.empty()) return; }
        try { summary_ = job_.get(); } catch (...) { summary_.error = L"Search failed: memory or I/O error."; }
        EnableInputs(true); std::wstring text = std::to_wstring(summary_.matches) + L" matches in " + std::to_wstring(summary_.files) + L" scanned file(s); " + std::to_wstring(summary_.errors) + L" error(s).";
        if (summary_.truncated) text += L" First 100,000 results shown."; if (summary_.cancelled) text += L" Cancelled; partial results."; if (!summary_.error.empty()) text += L" " + summary_.error; SetDlgItemTextW(1914,text.c_str());
    } else SetDlgItemTextW(1914,(L"Scanned: " + std::to_wstring(files_.load()) + L" files. Received: " + std::to_wstring(matches_.size()) + L" results. Other editors remain available.").c_str());
}
void CMultiSearchDialog::OnTimer(UINT_PTR id) { if (id == 1) Tick(); CDialog::OnTimer(id); }
void CMultiSearchDialog::OnDestroy() { KillTimer(1); Stop(); CDialog::OnDestroy(); }
void CMultiSearchDialog::OnCancel() { cancel_ = true; available_.notify_all(); ShowWindow(SW_HIDE); }
void CMultiSearchDialog::OnOK() {
    if (Busy()) return; CString find,folder,include,exclude; GetDlgItemTextW(1900,find); GetDlgItemTextW(1904,folder); GetDlgItemTextW(1906,include); GetDlgItemTextW(1907,exclude);
    MultiSearchRequest request; request.text.find = find.GetString(); request.text.match_case = IsDlgButtonChecked(1911) != 0; request.text.whole_word = IsDlgButtonChecked(1912) != 0; request.mode = mode_.GetCurSel(); request.encoding = static_cast<TextEncoding>(encoding_.GetCurSel()); request.recursive = IsDlgButtonChecked(1910) != 0; request.include = include.GetString(); request.exclude = exclude.GetString();
    std::vector<MultiSearchSource> sources; std::wstring error;
    if (scope_.GetCurSel() == 0 && (!capture || !capture(request.mode,&sources,&error))) { SetDlgItemTextW(1914,error.empty() ? L"No open file editors are available." : error.c_str()); return; }
    if (scope_.GetCurSel() == 1 && folder.IsEmpty()) { SetDlgItemTextW(1914,L"Choose a folder to search."); return; }
    Start(std::move(request),scope_.GetCurSel() == 1 ? std::filesystem::path(folder.GetString()) : std::filesystem::path(),std::move(sources));
}
BOOL CMultiSearchDialog::OnCommand(WPARAM wparam,LPARAM lparam) {
    if (LOWORD(wparam) == 1905) { if (!Busy()) { CString current; GetDlgItemTextW(1904,current); CFolderPickerDialog dialog(current.IsEmpty() ? nullptr : current.GetString(),0,this); if (dialog.DoModal() == IDOK) SetDlgItemTextW(1904,dialog.GetPathName()); } return TRUE; }
    if (LOWORD(wparam) == 1913) { cancel_ = true; available_.notify_all(); return TRUE; }
    if (LOWORD(wparam) == 1908) { std::wstring text; for (const auto& error : summary_.details) text += error + L"\r\n"; CTextResultDialog dialog(this,L"Search errors (first 200)",text); dialog.DoModal(); return TRUE; }
    return CDialog::OnCommand(wparam,lparam);
}
void CMultiSearchDialog::OnDisplay(NMHDR* header,LRESULT* result) {
    *result = 0; auto* info = reinterpret_cast<NMLVDISPINFO*>(header); const auto index = info->item.iItem; if (!(info->item.mask & LVIF_TEXT) || index < 0 || static_cast<std::size_t>(index) >= matches_.size()) return;
    const auto& row = matches_[index]; std::wstring text;
    if (info->item.iSubItem == 0) text = row.path; else if (info->item.iSubItem == 1) text = row.binary ? L"-" : std::to_wstring(row.match.line); else if (info->item.iSubItem == 2) { wchar_t value[40]{}; swprintf_s(value,L"0x%llX",row.match.offset); text = value; } else text = CCodeEditor::FromUtf8(row.match.preview);
    wcsncpy_s(info->item.pszText,info->item.cchTextMax,text.c_str(),_TRUNCATE);
}
void CMultiSearchDialog::OnActivate(NMHDR*,LRESULT* result) { *result = 0; const auto row = list_.GetNextItem(-1,LVNI_SELECTED); if (row >= 0 && static_cast<std::size_t>(row) < matches_.size() && navigate) navigate(matches_[row]); }

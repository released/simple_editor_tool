#include "workspace_frame.h"
#include "ui/localization.h"
#include "main_frame.h"
#include "resource.h"
#include <algorithm>
using namespace simple_editor::core;
namespace {
bool SamePath(const std::wstring& first,const std::wstring& second) { return CompareStringOrdinal(first.c_str(),-1,second.c_str(),-1,TRUE) == CSTR_EQUAL; }
std::wstring MenuPath(std::wstring path) { if (path.size() > 160) path = path.substr(0,60) + L"..." + path.substr(path.size() - 96); for (std::size_t i = 0; (i = path.find(L'&',i)) != std::wstring::npos; i += 2) path.insert(i,1,L'&'); return path; }
}
void CWorkspaceFrame::AppendFileActions(CMenu& menu) {
    menu.AppendMenu(MF_SEPARATOR); menu.AppendMenu(MF_STRING,ID_FILE_NEW,L"&New text file\tCtrl+N"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_NEW_CLIPBOARD,L"New from Unicode clipboard"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_REOPEN,L"Reopen closed file\tCtrl+Shift+T");
    CMenu recent; recent.CreatePopupMenu(); if (recent_files_.empty()) recent.AppendMenu(MF_STRING | MF_GRAYED,0,L"(No recent files)"); else for (std::size_t i = 0; i < recent_files_.size(); ++i) recent.AppendMenu(MF_STRING,ID_WORKSPACE_RECENT_FIRST + i,MenuPath(recent_files_[i]).c_str()); menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(recent.Detach()),L"Recent files");
    menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLOSE_OTHERS,L"Close other tabs"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLOSE_LEFT,L"Close tabs to the left"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLOSE_RIGHT,L"Close tabs to the right");
    menu.AppendMenu(MF_SEPARATOR); menu.AppendMenu(MF_STRING,ID_WORKSPACE_PREFERENCES,L"Preferences...");
    menu.AppendMenu(MF_STRING,ID_WORKSPACE_RECOVER_DRAFTS,L"Recover draft snapshots..."); menu.AppendMenu(MF_STRING,ID_WORKSPACE_SNAPSHOT,L"Snapshot workspace / drafts now"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLEAR_RECOVERY,L"Clear recovery snapshots..."); menu.AppendMenu(MF_STRING,ID_WORKSPACE_BROWSE_RECOVERY,L"Browse recovery folder");
    UpdateMenuShortcuts(menu);
}
void CWorkspaceFrame::RememberRecent(const std::wstring& path) {
    if (path.empty() || IsTemporaryPath(path)) return;
    recent_files_.erase(std::remove_if(recent_files_.begin(),recent_files_.end(),[&](const auto& old) { return SamePath(old,path); }),recent_files_.end()); recent_files_.push_front(path); if (recent_files_.size() > 20) recent_files_.pop_back(); RefreshRecentMenus();
    if (!preferences_path_.empty()) preferences_dirty_ = true;
}
void CWorkspaceFrame::RefreshRecentMenus() {
    const auto refresh = [&](CMenu* file) { if (!file) return; for (int p = 0; p < file->GetMenuItemCount(); ++p) { CString title; file->GetMenuString(p,title,MF_BYPOSITION); if (title != L"Recent files") continue; auto* menu = file->GetSubMenu(p); if (!menu) continue; while (menu->GetMenuItemCount()) menu->DeleteMenu(0,MF_BYPOSITION); if (recent_files_.empty()) menu->AppendMenu(MF_STRING | MF_GRAYED,0,L"(No recent files)"); else for (std::size_t i = 0; i < recent_files_.size(); ++i) menu->AppendMenu(MF_STRING,ID_WORKSPACE_RECENT_FIRST + i,MenuPath(recent_files_[i]).c_str()); } };
    refresh(root_menu_.GetSubMenu(0)); UpdateMenuShortcuts(root_menu_); for (auto* frame : sessions_) { refresh(frame->session_menu_.GetSubMenu(0)); UpdateMenuShortcuts(frame->session_menu_); } DrawMenuBar();
}
bool CWorkspaceFrame::NewDocument(const std::vector<std::uint8_t>& bytes,TextEncoding encoding,int page) {
    CancelCloseAll(); std::wstring error; const auto path = temporary_documents_.Create(&error); if (path.empty()) { UiMessageBox(error.c_str(),MB_ICONERROR); return false; }
    BinaryDocument document; if (!document.Open(path.wstring(),&error) || (!bytes.empty() && !document.Replace(0,0,bytes))) { UiMessageBox(error.empty() ? L"Could not initialize the new document." : error.c_str(),MB_ICONERROR); return false; }
    auto* frame = CreateSession(); if (!frame) return false; frame->document_.Swap(document); frame->untitled_ = true; frame->untitled_name_ = L"Untitled " + std::to_wstring(++untitled_counter_); untitled_aliases_.push_back(frame->untitled_name_); frame->requested_encoding_ = encoding; frame->SetWindowTextW((frame->untitled_name_ + L" - Simple Editor Tool").c_str()); frame->hex_.SetDocument(&frame->document_); frame->text_stale_ = true; frame->ShowPage(page); RefreshLabels(); return true;
}
void CWorkspaceFrame::OnNewFile() { NewDocument(); }
void CWorkspaceFrame::OnNewClipboard() {
    std::wstring text;
    if (!::OpenClipboard(GetSafeHwnd())) { UiMessageBox(L"Could not open the clipboard.",MB_ICONWARNING); return; }
    const auto memory = GetClipboardData(CF_UNICODETEXT); const auto size = memory ? GlobalSize(memory) : 0; const auto* data = memory && size >= 2 && size <= 64 * 1024 * 1024 ? static_cast<const wchar_t*>(GlobalLock(memory)) : nullptr;
    bool valid = false; if (data) { const auto* end = std::find(data,data + size / 2,L'\0'); if (end != data + size / 2) { text.assign(data,end); valid = true; } GlobalUnlock(memory); } CloseClipboard();
    std::wstring error; if (!valid || !ValidateText(text,&error)) { UiMessageBox(error.empty() ? L"The clipboard needs valid Unicode text up to 64 MiB." : error.c_str(),MB_ICONWARNING); return; }
    const auto utf8 = CCodeEditor::ToUtf8(text); if (utf8.size() > 32 * 1024 * 1024) { UiMessageBox(L"Clipboard text exceeds the 32-MiB editable text limit.",MB_ICONWARNING); return; } NewDocument({utf8.begin(),utf8.end()});
}
void CWorkspaceFrame::OnReopenClosed() { if (closed_files_.empty()) return; const auto path = closed_files_.front(); closed_files_.pop_front(); OpenPath(path); }
void CWorkspaceFrame::OnRecentFile(UINT id) { const auto index = id - ID_WORKSPACE_RECENT_FIRST; if (index < recent_files_.size()) { const auto path = recent_files_[index]; OpenPath(path); } }
void CWorkspaceFrame::ReorderSessions(int from,int to) {
    if (from < 0 || to < 0 || static_cast<std::size_t>(from) >= sessions_.size() || static_cast<std::size_t>(to) >= sessions_.size() || from == to) return;
    auto* active = active_session_ >= 0 ? sessions_[active_session_] : nullptr; auto* moved = sessions_[from]; sessions_.erase(sessions_.begin() + from); sessions_.insert(sessions_.begin() + to,moved); sessions_tab_.DeleteAllItems(); for (std::size_t i = 0; i < sessions_.size(); ++i) sessions_tab_.InsertItem(static_cast<int>(i),L"");
    const auto found = std::find(sessions_.begin(),sessions_.end(),active); if (found != sessions_.end()) ActivateSession(static_cast<int>(found - sessions_.begin())); else RefreshLabels();
}
void CWorkspaceFrame::TabContext(int index,CPoint point) {
    if (index < 0 || static_cast<std::size_t>(index) >= sessions_.size()) return;
    const auto identity = sessions_[index]->workspace_session_id_;
    CMenu menu; menu.CreatePopupMenu(); menu.AppendMenu(MF_STRING,ID_FILE_CLOSE,L"Close this tab"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLOSE_OTHERS,L"Close other tabs"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLOSE_LEFT,L"Close tabs to the left"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_CLOSE_RIGHT,L"Close tabs to the right"); menu.AppendMenu(MF_SEPARATOR); menu.AppendMenu(MF_STRING,ID_WORKSPACE_MOVE_LEFT,L"Move left"); menu.AppendMenu(MF_STRING,ID_WORKSPACE_MOVE_RIGHT,L"Move right");
    const auto chosen = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON,point.x,point.y,this);
    const auto found = std::find_if(sessions_.begin(),sessions_.end(),[&](const auto* frame) { return frame->workspace_session_id_ == identity; }); if (found == sessions_.end()) return;
    context_session_ = static_cast<int>(found - sessions_.begin()); if (chosen == ID_FILE_CLOSE) { close_queue_.clear(); CloseSession(*found); } else if (chosen) OnTabAction(chosen); context_session_ = -1;
}
void CWorkspaceFrame::OnTabAction(UINT id) {
    const int index = context_session_ >= 0 ? context_session_ : active_session_; if (index < 0 || static_cast<std::size_t>(index) >= sessions_.size()) return;
    if (id == ID_WORKSPACE_MOVE_LEFT || id == ID_WORKSPACE_MOVE_RIGHT) { ReorderSessions(index,index + (id == ID_WORKSPACE_MOVE_LEFT ? -1 : 1)); return; }
    closing_all_ = false; close_queue_.clear();
    for (std::size_t i = 0; i < sessions_.size(); ++i) if ((id == ID_WORKSPACE_CLOSE_OTHERS && i != static_cast<std::size_t>(index)) || (id == ID_WORKSPACE_CLOSE_LEFT && i < static_cast<std::size_t>(index)) || (id == ID_WORKSPACE_CLOSE_RIGHT && i > static_cast<std::size_t>(index))) close_queue_.push_back(sessions_[i]->workspace_session_id_);
    PostMessage(WM_APP + 44);
}
LRESULT CWorkspaceFrame::OnContinueCloseMany(WPARAM,LPARAM) {
    if (closing_all_) { close_queue_.clear(); return 0; }
    while (!close_queue_.empty()) { const auto id = close_queue_.front(); const auto found = std::find_if(sessions_.begin(),sessions_.end(),[&](const auto* frame) { return frame->workspace_session_id_ == id; }); if (found == sessions_.end()) { close_queue_.pop_front(); continue; } auto* frame = *found; if (!CloseSession(frame) && !frame->IsSaving()) close_queue_.clear(); return 0; } return 0;
}

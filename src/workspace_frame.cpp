#include "workspace_frame.h"
#include "main_frame.h"
#include "resource.h"
#include "ui/localization.h"
#include <algorithm>
#include <filesystem>
#include <cwctype>

namespace {
constexpr UINT kQueuedOpen = WM_APP + 42;
constexpr UINT kContinueClose = WM_APP + 43;
std::wstring NormalPath(const std::wstring& path) {
    std::error_code error;
    auto full = std::filesystem::absolute(path, error);
    return error ? path : full.lexically_normal().wstring();
}
bool SamePath(const std::wstring& first, const std::wstring& second) {
    return CompareStringOrdinal(NormalPath(first).c_str(),-1,NormalPath(second).c_str(),-1,TRUE) == CSTR_EQUAL;
}
std::wstring LockKey(const std::wstring& path) { auto key = NormalPath(path); for (auto& c : key) c = static_cast<wchar_t>(towlower(c)); return key; }
}

BEGIN_MESSAGE_MAP(CWorkspaceFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_SETFOCUS()
    ON_WM_GETMINMAXINFO()
    ON_WM_DROPFILES()
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_WM_COPYDATA()
    ON_COMMAND(ID_FILE_OPEN, OnOpen)
    ON_COMMAND(ID_FILE_COMPARE, OnNewCompare)
    ON_COMMAND(ID_FILE_COMPARE_EQUAL, OnNewCompareEqual)
    ON_COMMAND(ID_FILE_CLOSE, OnCloseTab)
    ON_COMMAND(ID_WORKSPACE_OPEN, OnOpen)
    ON_COMMAND(ID_WORKSPACE_COMPARE, OnNewCompare)
    ON_COMMAND(ID_WORKSPACE_CLOSE, OnCloseTab)
    ON_COMMAND_RANGE(ID_WORKSPACE_TEXT_COMPARE, ID_WORKSPACE_FOLDERS, OnAdvancedComparison)
    ON_COMMAND_RANGE(ID_WORKSPACE_PASTED_TEXT, ID_WORKSPACE_PASTED_TEXT, OnAdvancedComparison)
    ON_COMMAND(ID_WORKSPACE_SAVE_SESSION, OnSaveWorkspace)
    ON_COMMAND(ID_WORKSPACE_LOAD_SESSION, OnLoadWorkspace)
    ON_COMMAND(ID_APP_ABOUT,OnAboutNative)
    ON_COMMAND(ID_WORKSPACE_SEARCH_FILES,OnSearchFiles)
    ON_COMMAND(ID_WORKSPACE_BATCH_REPLACE,OnBatchReplace)
    ON_COMMAND_RANGE(ID_WORKFLOW_PALETTE,ID_WORKFLOW_PALETTE,OnWorkflowRoot)
    ON_COMMAND_RANGE(ID_WORKFLOW_BACK,ID_WORKFLOW_FORWARD,OnWorkflowRoot)
    ON_COMMAND(ID_FILE_NEW,OnNewFile)
    ON_COMMAND(ID_WORKSPACE_NEW_CLIPBOARD,OnNewClipboard)
    ON_COMMAND(ID_WORKSPACE_REOPEN,OnReopenClosed)
    ON_COMMAND_RANGE(ID_WORKSPACE_RECENT_FIRST,ID_WORKSPACE_RECENT_LAST,OnRecentFile)
    ON_COMMAND_RANGE(ID_WORKSPACE_CLOSE_OTHERS,ID_WORKSPACE_MOVE_RIGHT,OnTabAction)
    ON_COMMAND(ID_WORKSPACE_PREFERENCES,OnPreferences)
    ON_COMMAND_RANGE(ID_LANGUAGE_ENGLISH,ID_LANGUAGE_TRADITIONAL,OnLanguage)
    ON_COMMAND_RANGE(ID_EXPLORER_ADD,ID_EXPLORER_REMOVE,OnExplorerMenu)
    ON_UPDATE_COMMAND_UI_RANGE(ID_EXPLORER_ADD,ID_EXPLORER_REMOVE,OnUpdateExplorerMenu)
    ON_COMMAND_RANGE(ID_WORKSPACE_RECOVER_DRAFTS,ID_WORKSPACE_BROWSE_RECOVERY,OnRecoveryCommand)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnUpdateClose)
    ON_UPDATE_COMMAND_UI(ID_FILE_COMPARE, OnUpdateCompare)
    ON_UPDATE_COMMAND_UI(ID_FILE_COMPARE_EQUAL, OnUpdateCompare)
    ON_NOTIFY(TCN_SELCHANGE, 600, OnTabChanged)
    ON_MESSAGE(kQueuedOpen, OnQueuedOpen)
    ON_MESSAGE(kContinueClose, OnContinueClose)
    ON_MESSAGE(WM_APP + 44,OnContinueCloseMany)
END_MESSAGE_MAP()

CWorkspaceFrame::CWorkspaceFrame(bool publish_instance) {
    published_instance_ = publish_instance;
    std::wstring preference_error;
    if (publish_instance) { std::vector<wchar_t> name(32768); const auto count = GetModuleFileNameW(nullptr,name.data(),static_cast<DWORD>(name.size())); if (count && count < name.size()) LoadConfiguration(std::filesystem::path(name.data()).parent_path() / L"simple_editor_tool.ini",&preference_error); }
    if (publish_instance) { wchar_t executable[32768]{}; GetModuleFileNameW(nullptr,executable,32768); std::wstring language_error; if (!simple_editor::ui::InitializeLocalization(std::filesystem::path(executable).parent_path(),preferences_.language,&language_error)) { if (!preference_error.empty()) preference_error += L"\r\n"; preference_error += L"Language files could not be loaded; English interface is active. " + language_error; } }
    Create(nullptr, L"Simple Editor Tool", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, rectDefault);
    SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE); simple_editor::ui::RegisterLocalizationRoot(this); simple_editor::ui::RegisterLocalizationMenu(root_menu_.GetSafeHmenu(),GetSafeHwnd());
    if (publish_instance) {
        ::SetPropW(m_hWnd, L"SimpleEditorTool.Native.WorkspaceVersion", reinterpret_cast<HANDLE>(2));
        ::SetPropW(m_hWnd, L"SimpleEditorTool.Native.MainWindow", reinterpret_cast<HANDLE>(1));
    }
    if (!preference_error.empty()) welcome_.SetWindowTextW((L"Portable settings could not be loaded; defaults are active.\r\n" + preference_error).c_str());
    if (preferences_.auto_restore || preferences_.recover_drafts) InitializeRecovery();
}
int CWorkspaceFrame::OnCreate(LPCREATESTRUCT create) {
    if (CFrameWnd::OnCreate(create) == -1) return -1;
    root_menu_.CreateMenu(); CMenu file; file.CreatePopupMenu();
    file.AppendMenu(MF_STRING, ID_FILE_OPEN, L"&Open files...\tCtrl+O");
    file.AppendMenu(MF_STRING, ID_FILE_COMPARE, L"&New comparison...");
    file.AppendMenu(MF_STRING, ID_WORKSPACE_PASTED_TEXT, L"Pasted text comparison...");
    file.AppendMenu(MF_STRING, ID_WORKSPACE_TEXT_COMPARE, L"Text/source comparison..."); file.AppendMenu(MF_STRING, ID_WORKSPACE_THREE_MERGE, L"Three-way text merge..."); file.AppendMenu(MF_STRING, ID_WORKSPACE_FOLDERS, L"Folder comparison / sync...");
    file.AppendMenu(MF_STRING, ID_FILE_CLOSE, L"&Close tab\tCtrl+W");
    file.AppendMenu(MF_STRING,ID_WORKFLOW_PALETTE,L"Search commands...\tCtrl+Shift+P"); file.AppendMenu(MF_STRING,ID_WORKSPACE_BATCH_REPLACE,L"Preview replacements in folder..."); file.AppendMenu(MF_STRING,ID_WORKSPACE_SEARCH_FILES,L"Find in open files / folder...\tCtrl+Shift+F");
    file.AppendMenu(MF_STRING,ID_WORKSPACE_SAVE_SESSION,L"Save workspace..."); file.AppendMenu(MF_STRING,ID_WORKSPACE_LOAD_SESSION,L"Open workspace...");
    AppendFileActions(file);
    file.AppendMenu(MF_SEPARATOR); file.AppendMenu(MF_STRING, ID_APP_EXIT, L"E&xit");
    root_menu_.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(file.Detach()), L"&File");
    CMenu tools; tools.CreatePopupMenu(); AppendExplorerMenu(tools); root_menu_.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(tools.Detach()),L"&Tools");
    simple_editor::ui::AppendLanguageMenu(root_menu_); CMenu help; help.CreatePopupMenu(); help.AppendMenu(MF_STRING,ID_APP_ABOUT,L"About / native component notices..."); root_menu_.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(help.Detach()),L"&Help"); SetMenu(&root_menu_);
    open_button_.Create(L"Open files...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_WORKSPACE_OPEN);
    compare_button_.Create(L"Compare files...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_WORKSPACE_COMPARE);
    close_button_.Create(L"Close tab", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, ID_WORKSPACE_CLOSE);
    sessions_tab_.Create(WS_CHILD | WS_VISIBLE | TCS_TABS | TCS_SINGLELINE, CRect(), this, 600);
    sessions_tab_.close = [this](int index) { if (index >= 0 && static_cast<std::size_t>(index) < sessions_.size()) { close_queue_.clear(); CloseSession(sessions_[index]); } };
    sessions_tab_.context = [this](int index,CPoint point) { TabContext(index,point); };
    sessions_tab_.reorder = [this](int from,int to) { ReorderSessions(from,to); };
    welcome_.Create(L"Drop files here or choose Open files.\r\nEach file stays open in its own tab.\r\nUse Compare files to select the left and right inputs.",
                    WS_CHILD | WS_VISIBLE | SS_CENTER, CRect(), this, 601);
    DragAcceptFiles(TRUE); close_button_.EnableWindow(FALSE); Layout(); return 0;
}
void CWorkspaceFrame::Layout() {
    if (!sessions_tab_.GetSafeHwnd() || !close_button_.GetSafeHwnd() || !welcome_.GetSafeHwnd()) return;
    CRect client; GetClientRect(client);
    open_button_.MoveWindow(8, 5, 110, 27); compare_button_.MoveWindow(126, 5, 130, 27);
    close_button_.MoveWindow((std::max)(264, client.Width() - 98), 5, 90, 27);
    CRect tabs = client; tabs.top = 37; sessions_tab_.MoveWindow(tabs);
    CRect page = tabs; sessions_tab_.AdjustRect(FALSE, page); page.DeflateRect(3, 3);
    page.right = (std::max)(page.left, page.right); page.bottom = (std::max)(page.top, page.bottom);
    welcome_.MoveWindow(page.left + 12, page.top + 36, (std::max)(0, page.Width() - 24), (std::max)(0, page.Height() - 36));
    if (active_session_ >= 0 && active_session_ < static_cast<int>(sessions_.size())) sessions_[active_session_]->MoveWindow(page);
}
void CWorkspaceFrame::OnSize(UINT type, int cx, int cy) { CFrameWnd::OnSize(type, cx, cy); Layout(); }
void CWorkspaceFrame::FocusActive() {
    if (active_session_ < 0 || !IsWindowVisible()) return;
    auto* session = sessions_[active_session_];
    CWnd* target = session->ActiveEditor();
    if (!target) target = session->active_tab_ == 0 ? static_cast<CWnd*>(&session->hex_) : static_cast<CWnd*>(&session->compare_left_);
    if (target->GetSafeHwnd()) target->SetFocus();
}
void CWorkspaceFrame::OnSetFocus(CWnd* previous) { CFrameWnd::OnSetFocus(previous); FocusActive(); }
void CWorkspaceFrame::OnGetMinMaxInfo(MINMAXINFO* info) { CFrameWnd::OnGetMinMaxInfo(info); info->ptMinTrackSize = {720, 480}; }
CMainFrame* CWorkspaceFrame::CreateSession() {
    auto* session = new CMainFrame(false, this);
    if (!session->GetSafeHwnd()) { delete session; return nullptr; }
    session->workspace_session_id_ = ++next_session_id_; ApplyPreferencesToFrame(session); sessions_.push_back(session); sessions_tab_.InsertItem(static_cast<int>(sessions_.size()) - 1, L"New comparison");
    ActivateSession(static_cast<int>(sessions_.size()) - 1); return session;
}
void CWorkspaceFrame::ActivateSession(int index) {
    if (active_session_ >= 0 && active_session_ < static_cast<int>(sessions_.size())) TrackNavigation(sessions_[active_session_]);
    if (index < 0 || index >= static_cast<int>(sessions_.size())) return;
    for (std::size_t i = 0; i < sessions_.size(); ++i) {
        auto* session = sessions_[i];
        if (static_cast<int>(i) != index && session->search_dialog_.GetSafeHwnd()) session->search_dialog_.ShowWindow(SW_HIDE);
        session->ShowWindow(static_cast<int>(i) == index ? SW_SHOW : SW_HIDE);
    }
    active_session_ = index; sessions_tab_.SetCurSel(index); welcome_.ShowWindow(SW_HIDE);
    if (sessions_[index]->comparison_session_) {
        auto* session = sessions_[index]; CString left, right;
        session->compare_left_path_.GetWindowTextW(left); session->compare_right_path_.GetWindowTextW(right);
        const int mode = session->compare_mode_.GetCurSel(); session->SetComparisonPaths(left.GetString(), right.GetString(), mode != 1); session->compare_mode_.SetCurSel(mode);
    }
    SetMenu(&sessions_[index]->session_menu_); DrawMenuBar(); close_button_.EnableWindow(TRUE); Layout(); RefreshLabels(); FocusActive(); TrackNavigation(sessions_[index]);
}
void CWorkspaceFrame::RefreshLabels() {
    for (std::size_t i = 0; i < sessions_.size(); ++i) {
        auto* session = sessions_[i];
        std::wstring label = std::filesystem::path(session->document_.Path()).filename().wstring();
        if (session->untitled_) label = session->untitled_name_;
        if (session->comparison_session_) {
            CString left, right; session->compare_left_path_.GetWindowTextW(left); session->compare_right_path_.GetWindowTextW(right);
            label = left.IsEmpty() || right.IsEmpty() ? L"New comparison" : std::filesystem::path(left.GetString()).filename().wstring() + L" vs " + std::filesystem::path(right.GetString()).filename().wstring();
        }
        if (label.empty()) label = L"New comparison";
        if (session->comparison_session_ && session->displayed_mode_ == 8) label = L"Pasted text comparison";
        if (!session->comparison_session_ && (session->document_.IsModified() || session->text_dirty_ || session->text_.GetModify() || session->untitled_dirty_)) label += L" *";
        if (session->comparison_session_ && session->advanced_.Dirty()) label += L" *";
        TCITEM item{}; item.mask = TCIF_TEXT; item.pszText = label.data(); sessions_tab_.SetItem(static_cast<int>(i), &item);
    }
    if (active_session_ >= 0) {
        CString title; sessions_[active_session_]->GetWindowTextW(title); SetWindowTextW(title);
    }
}
bool CWorkspaceFrame::OpenPath(const std::wstring& path, bool silent) {
    CancelCloseAll();
    for (std::size_t i = 0; i < sessions_.size(); ++i) {
        if (!sessions_[i]->comparison_session_ && sessions_[i]->document_.IsOpen() && SamePath(path, sessions_[i]->document_.Path())) {
            if (sessions_[i]->after_save_ == CMainFrame::AfterSave::Close) sessions_[i]->after_save_ = CMainFrame::AfterSave::None;
            sessions_[i]->saved_close_pending_ = false;
            ActivateSession(static_cast<int>(i)); if (!sessions_[i]->untitled_) RememberRecent(sessions_[i]->document_.Path()); return true;
        }
    }
    // Validate before creating a tab; failed input never changes an existing session.
    simple_editor::core::BinaryDocument opened; std::wstring error;
    if (!opened.Open(NormalPath(path), &error)) { if (!silent) UiMessageBox((L"Cannot open file:\n" + error).c_str(), MB_ICONERROR); return false; }
    auto* session = CreateSession(); if (!session) return false;
    session->document_.Swap(opened); session->hex_.SetDocument(&session->document_);
    session->text_stale_ = true; session->SetWindowTextW((session->document_.Path() + L" - Simple Editor Tool").c_str());
    if (session->document_.Size() <= 32 * 1024 * 1024) { session->RefreshText(); if (session->text_editable_) session->ShowPage(1); }
    session->RequestIndentation(true); session->SetSaveReadOnly(DocumentLocked(session->document_));
    session->status_.SetPaneText(0, session->document_.Path().c_str()); RememberRecent(session->document_.Path()); RefreshLabels(); return true;
}
void CWorkspaceFrame::ChooseFiles() {
    std::vector<wchar_t> names(65536, L'\0');
    CFileDialog dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER,
                       L"All files (*.*)|*.*||", this);
    dialog.m_ofn.lpstrFile = names.data(); dialog.m_ofn.nMaxFile = static_cast<DWORD>(names.size());
    workspace_dialog_ = true; const INT_PTR selected = dialog.DoModal(); workspace_dialog_ = false; if (selected != IDOK) return;
    POSITION position = dialog.GetStartPosition(); while (position) OpenPath(dialog.GetNextPathName(position).GetString());
}
void CWorkspaceFrame::OpenDroppedFiles(HDROP drop) {
    const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    std::vector<std::wstring> paths;
    for (UINT i = 0; i < count; ++i) {
        const UINT length = DragQueryFileW(drop, i, nullptr, 0); std::vector<wchar_t> path(length + 1);
        if (DragQueryFileW(drop, i, path.data(), length + 1)) paths.emplace_back(path.data());
    }
    DragFinish(drop); for (const auto& path : paths) OpenPath(path);
}
void CWorkspaceFrame::NewComparison(const std::wstring& left, bool insertion_aware) {
    CancelCloseAll();
    auto* session = CreateSession(); if (!session) return;
    session->comparison_session_ = true; session->SetSaveReadOnly(false); session->SetComparisonPaths(left, L"", insertion_aware);
    session->ShowPage(2); session->SetWindowTextW(L"New comparison - Simple Editor Tool"); RefreshLabels();
}
void CWorkspaceFrame::NewComparisonInputs(const std::wstring& left, const std::wstring& right, const std::wstring& base, int mode, bool run) {
    NewComparison(left,mode != 1); if (active_session_ < 0) return; auto* session = sessions_[active_session_];
    session->SetComparisonPaths(left,right,mode != 1); SetPathComboText(session->compare_base_path_,base);
    session->compare_mode_.SetCurSel(mode); session->OnCompareModeChanged(); if (run) session->OnCompareRun(); RefreshLabels();
}
bool CWorkspaceFrame::PathLocked(const std::wstring& path) const { return !path.empty() && locked_paths_.count(LockKey(path)) != 0; }
bool CWorkspaceFrame::DocumentLocked(const simple_editor::core::BinaryDocument& document) const { return PathLocked(document.Path()) || (document.IsOpen() && locked_identities_.count(document.SourceIdentity()) != 0); }
bool CWorkspaceFrame::LockPaths(const std::vector<std::wstring>& paths, bool acquire, const void* owner, const std::vector<simple_editor::core::FileStamp>& known) {
    if (acquire) {
        if (lock_groups_.count(owner)) return false;
        LockGroup group;
        for (std::size_t i = 0; i < paths.size(); ++i) if (!paths[i].empty()) {
            group.paths.insert(LockKey(paths[i])); simple_editor::core::FileStamp stamp; std::wstring error;
            if (known.size() == paths.size()) stamp = known[i]; else if (!simple_editor::core::InspectFile(paths[i],&stamp,&error)) return false;
            if (stamp.exists && !stamp.directory) group.identities.insert({stamp.volume,stamp.identity});
        }
        for (const auto& key : group.paths) if (locked_paths_.count(key)) return false;
        for (const auto& identity : group.identities) if (locked_identities_.count(identity)) return false;
        for (auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen() && (group.paths.count(LockKey(session->document_.Path())) || group.identities.count(session->document_.SourceIdentity())) &&
            owner != &session->job_ && (session->IsBusy() || (owner != &open_search_io_ && (session->document_.IsModified() || session->text_dirty_ || session->text_.GetModify() || session->untitled_dirty_)))) return false;
        locked_paths_.insert(group.paths.begin(),group.paths.end()); locked_identities_.insert(group.identities.begin(),group.identities.end()); lock_groups_.emplace(owner,std::move(group));
    } else {
        const auto group = lock_groups_.find(owner); if (group == lock_groups_.end()) return true;
        for (const auto& key : group->second.paths) locked_paths_.erase(key); for (const auto& identity : group->second.identities) locked_identities_.erase(identity); lock_groups_.erase(group);
    }
    if (!destroying_) for (auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen()) session->SetSaveReadOnly(session->IsSaving() || DocumentLocked(session->document_));
    return true;
}
std::vector<std::wstring> CWorkspaceFrame::OpenPaths() const {
    std::vector<std::wstring> paths;
    for (const auto* session : sessions_) if (!session->comparison_session_ && session->document_.IsOpen()) paths.push_back(session->untitled_ ? session->untitled_name_ : session->document_.Path());
    return paths;
}
std::vector<std::wstring> CWorkspaceFrame::FolderPaths() const {
    std::vector<std::wstring> paths;
    for (auto* session : sessions_) if (session->comparison_session_ && session->displayed_mode_ == 4) {
        CString left,right; session->compare_left_path_.GetWindowTextW(left); session->compare_right_path_.GetWindowTextW(right);
        if (!left.IsEmpty()) paths.emplace_back(left.GetString()); if (!right.IsEmpty()) paths.emplace_back(right.GetString());
    }
    return paths;
}
std::unique_ptr<simple_editor::core::BinaryDocument> CWorkspaceFrame::SnapshotForPath(const std::wstring& path, std::wstring* error) {
    for (auto* session : sessions_) {
        if (session->comparison_session_ || !session->document_.IsOpen() || (session->untitled_ ? path != session->untitled_name_ && !SamePath(path,session->document_.Path()) : !SamePath(path, session->document_.Path()))) continue;
        auto snapshot = session->document_.Snapshot(error); if (!snapshot) return nullptr;
        if (session->text_dirty_ || session->text_.GetModify()) {
            CString value; session->text_.GetWindowTextW(value); std::vector<std::uint8_t> bytes;
            if (!simple_editor::core::EncodeText(simple_editor::core::TextFromEditControl(session->text_original_,std::wstring(value.GetString(), value.GetLength())), session->text_codec_, true, &bytes, error) ||
                !snapshot->Replace(0, snapshot->Size(), bytes)) return nullptr;
        }
        return snapshot;
    }
    auto opened = std::make_unique<simple_editor::core::BinaryDocument>();
    if (!opened->Open(NormalPath(path), error)) return nullptr; return opened;
}
bool CWorkspaceFrame::CloseSession(CMainFrame* session) {
    if (close_prompt_active_) return false;
    const auto found = std::find(sessions_.begin(), sessions_.end(), session); if (found == sessions_.end()) return false;
    if (open_search_sessions_.count(session->workspace_session_id_)) { open_search_cancel_ = true; CancelCloseAll(); SearchStatus(L"Cancelling open-document search. Retry closing after it finishes."); return false; }
    if (session->path_dialog_) { closing_all_ = false; return false; }
    close_prompt_active_ = true; const bool advanced_close = session->advanced_.CanClose(); close_prompt_active_ = false;
    if (!advanced_close) { if (!session->advanced_.PendingClose()) CancelCloseAll(); return false; }
    if (session->IsSaving()) {
        session->after_save_ = CMainFrame::AfterSave::Close; session->after_save_path_.clear();
        session->status_.SetPaneText(0, L"This tab will close after saving succeeds. Esc cancels."); return false;
    }
    const bool needs_prompt = !session->comparison_session_ && (session->document_.IsModified() || session->text_dirty_ || session->text_.GetModify() || session->untitled_dirty_);
    if (needs_prompt) ActivateSession(static_cast<int>(found - sessions_.begin()));
    close_prompt_active_ = true;
    const bool confirmed = session->comparison_session_ || session->ConfirmDiscardChanges(CMainFrame::AfterSave::Close);
    close_prompt_active_ = false;
    if (!confirmed) {
        if (!session->IsSaving()) CancelCloseAll(); return false;
    }
    const auto current = std::find(sessions_.begin(), sessions_.end(), session); if (current == sessions_.end()) return false;
    CMainFrame* keep_active = active_session_ >= 0 && sessions_[active_session_] != session ? sessions_[active_session_] : nullptr;
    const int removed_index = static_cast<int>(current - sessions_.begin());
    if (closing_all_ && preferences_.auto_restore) { simple_editor::core::WorkspaceEntry entry; if (CaptureSession(session,&entry)) exit_workspace_[session->workspace_session_id_] = std::move(entry); else exit_workspace_.erase(session->workspace_session_id_); }
    if (!session->comparison_session_ && !session->untitled_ && session->document_.IsOpen()) { closed_files_.push_front(session->document_.Path()); if (closed_files_.size() > 20) closed_files_.pop_back(); }
    SetMenu(&root_menu_); sessions_tab_.DeleteItem(removed_index); sessions_.erase(current); active_session_ = -1;
    session->DestroyWindow();
    if (!sessions_.empty()) {
        const auto retained = std::find(sessions_.begin(), sessions_.end(), keep_active);
        ActivateSession(retained != sessions_.end() ? static_cast<int>(retained - sessions_.begin()) : (std::min)(removed_index, static_cast<int>(sessions_.size()) - 1));
    }
    else { welcome_.ShowWindow(SW_SHOW); close_button_.EnableWindow(FALSE); SetWindowTextW(L"Simple Editor Tool"); DrawMenuBar(); }
    if (closing_all_) PostMessage(kContinueClose); else if (!close_queue_.empty()) PostMessage(WM_APP + 44); return true;
}
void CWorkspaceFrame::OnClose() {
    if (workspace_dialog_ || recovery_modal_) return;
    if (preferences_io_.valid()) { preferences_close_pending_ = true; return; }
    if (preferences_dirty_ && StartPreferenceWrite()) { preferences_close_pending_ = true; return; }
    if (workspace_io_.valid()) { workspace_close_pending_ = true; return; }
    if (recovery_io_.valid()) { recovery_close_pending_ = true; return; }
    if (!closing_all_ && preferences_.auto_restore) { exit_workspace_.clear(); exit_order_.clear(); for (auto* frame : sessions_) { exit_order_.push_back(frame->workspace_session_id_); simple_editor::core::WorkspaceEntry entry; if (CaptureSession(frame,&entry)) exit_workspace_[frame->workspace_session_id_] = std::move(entry); } }
    close_queue_.clear(); closing_all_ = true;
    if (sessions_.empty()) { if (!recovery_root_.empty() && (preferences_.auto_restore || preferences_.recover_drafts) && StartRecoverySnapshot(true)) { recovery_close_pending_ = true; return; } CFrameWnd::OnClose(); return; }
    CloseSession(sessions_[active_session_ >= 0 ? active_session_ : 0]);
}
void CWorkspaceFrame::OnDestroy() { destroying_ = true; StopOpenSearch(); if (scoped_search_.GetSafeHwnd()) scoped_search_.DestroyWindow(); StopRecovery(); if (multi_search_.GetSafeHwnd()) multi_search_.DestroyWindow(); SetMenu(nullptr); CFrameWnd::OnDestroy(); }
void CWorkspaceFrame::OnOpen() { ChooseFiles(); }
void CWorkspaceFrame::OnNewCompare() { NewComparison(active_session_ >= 0 ? sessions_[active_session_]->untitled_ ? sessions_[active_session_]->untitled_name_ : sessions_[active_session_]->document_.Path() : L""); }
void CWorkspaceFrame::OnNewCompareEqual() { NewComparison(active_session_ >= 0 ? sessions_[active_session_]->untitled_ ? sessions_[active_session_]->untitled_name_ : sessions_[active_session_]->document_.Path() : L"", false); }
void CWorkspaceFrame::OnAdvancedComparison(UINT id) { if (id == ID_WORKSPACE_PASTED_TEXT) { NewComparisonInputs(L"",L"",L"",8,false); return; } NewComparisonInputs(active_session_ >= 0 ? sessions_[active_session_]->untitled_ ? sessions_[active_session_]->untitled_name_ : sessions_[active_session_]->document_.Path() : L"",L"",L"",id == ID_WORKSPACE_TEXT_COMPARE ? 2 : id == ID_WORKSPACE_THREE_MERGE ? 3 : 4); }
void CWorkspaceFrame::OnCloseTab() { CancelCloseAll(); if (active_session_ >= 0) CloseSession(sessions_[active_session_]); }
void CWorkspaceFrame::OnUpdateClose(CCmdUI* command) { command->Enable(active_session_ >= 0); }
void CWorkspaceFrame::OnUpdateCompare(CCmdUI* command) { command->Enable(TRUE); }
void CWorkspaceFrame::OnTabChanged(NMHDR*, LRESULT* result) { ActivateSession(sessions_tab_.GetCurSel()); *result = 0; }
void CWorkspaceFrame::OnDropFiles(HDROP drop) { OpenDroppedFiles(drop); }
BOOL CWorkspaceFrame::PreTranslateMessage(MSG* message) {
    if (scoped_search_.GetSafeHwnd() && scoped_search_.IsWindowVisible() && scoped_search_.IsDialogMessage(message)) return TRUE;
    if (message->message == WM_KEYDOWN && message->wParam == VK_ESCAPE && open_search_io_.valid()) { open_search_cancel_ = true; return TRUE; }
    if ((message->message == WM_KEYDOWN || message->message == WM_SYSKEYDOWN) && (GetKeyState(VK_MENU) & 0x8000) && (message->wParam == VK_LEFT || message->wParam == VK_RIGHT)) { NavigateHistory(message->wParam == VK_LEFT); return TRUE; }
    if (message->message == WM_KEYDOWN && message->wParam == 'P' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_SHIFT) & 0x8000)) { SendMessage(WM_COMMAND,ID_WORKFLOW_PALETTE); return TRUE; }
    if (message->message == WM_KEYDOWN && message->wParam == VK_ESCAPE && recovery_io_.valid() && (message->hwnd == GetSafeHwnd() || ::IsChild(GetSafeHwnd(),message->hwnd))) { recovery_cancel_ = true; recovery_close_pending_ = false; return TRUE; }
    if (DispatchShortcut(message)) return TRUE;
    if (message->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000)) {
        if (message->wParam == 'N') { OnNewFile(); return TRUE; }
        if (message->wParam == 'T' && (GetKeyState(VK_SHIFT) & 0x8000)) { OnReopenClosed(); return TRUE; }
        if (message->wParam == 'F' && (GetKeyState(VK_SHIFT) & 0x8000)) { OnSearchFiles(); return TRUE; }
        if (message->wParam == 'W') { OnCloseTab(); return TRUE; }
        if (message->wParam == 'O') { ChooseFiles(); return TRUE; }
        if (message->wParam == VK_TAB && !sessions_.empty()) {
            const int direction = GetKeyState(VK_SHIFT) & 0x8000 ? -1 : 1;
            ActivateSession((active_session_ + direction + static_cast<int>(sessions_.size())) % static_cast<int>(sessions_.size())); return TRUE;
        }
    }
    if (active_session_ >= 0 && message->message == WM_KEYDOWN) {
        const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const UINT key = static_cast<UINT>(message->wParam);
        const bool editor_key = control && (key == 'S' || key == 'R' || key == 'Z' || key == 'Y' || key == 'F' || key == 'H' || key == 'G' || key == '0' || key == VK_NUMPAD0);
        if (editor_key || key == VK_F3 || key == VK_F7 || (key == VK_ESCAPE && sessions_[active_session_]->IsBusy())) return sessions_[active_session_]->PreTranslateMessage(message);
    }
    return CFrameWnd::PreTranslateMessage(message);
}
BOOL CWorkspaceFrame::OnCmdMsg(UINT id, int code, void* extra, AFX_CMDHANDLERINFO* handler) {
    if (CFrameWnd::OnCmdMsg(id, code, extra, handler)) return TRUE;
    return active_session_ >= 0 ? sessions_[active_session_]->OnCmdMsg(id, code, extra, handler) : FALSE;
}
BOOL CWorkspaceFrame::OnCopyData(CWnd*, COPYDATASTRUCT* data) {
    if (!data || !data->lpData || data->cbData < sizeof(wchar_t) || data->cbData > 1024 * 1024 || data->cbData % sizeof(wchar_t)) return FALSE;
    const auto* text = static_cast<const wchar_t*>(data->lpData); const auto length = data->cbData / sizeof(wchar_t);
    if (text[length - 1] != L'\0' || (data->dwData != 1 && data->dwData != 2)) return FALSE;
    std::vector<std::wstring> paths; std::size_t start = 0;
    for (std::size_t i = 0; i < length; ++i) if (!text[i]) { if (i > start) paths.emplace_back(text + start, i - start); start = i + 1; }
    for (const auto& path : paths) queued_paths_.push_back(path);
    PostMessage(kQueuedOpen);
    if (::GetPropW(GetSafeHwnd(), L"SimpleEditorTool.Native.MainWindow")) { ShowWindow(SW_SHOWMAXIMIZED); SetForegroundWindow(); }
    return TRUE;
}
LRESULT CWorkspaceFrame::OnQueuedOpen(WPARAM, LPARAM) {
    if (!queued_paths_.empty()) { auto path = std::move(queued_paths_.front()); queued_paths_.pop_front(); OpenPath(path); }
    if (!queued_paths_.empty()) PostMessage(kQueuedOpen); return 0;
}
LRESULT CWorkspaceFrame::OnContinueClose(WPARAM, LPARAM) { if (closing_all_) OnClose(); return 0; }
void CWorkspaceFrame::OnAboutNative() { if (workspace_dialog_) return; CNativeNoticesDialog dialog(this); workspace_dialog_ = true; dialog.DoModal(); workspace_dialog_ = false; }

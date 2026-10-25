#pragma once

#include <afxcmn.h>
#include <afxwin.h>
#include <deque>
#include <memory>
#include <map>
#include <vector>
#include <set>
#include "core/binary_document.h"
#include "core/workspace_session.h"
#include <future>
#include "ui/multi_search_dialog.h"
#include "ui/workspace_tabs.h"
#include "core/temporary_documents.h"
#include "core/portable_preferences.h"
#include "core/recovery_store.h"

class CMainFrame;

class CWorkspaceFrame : public CFrameWnd {
public:
    explicit CWorkspaceFrame(bool publish_instance = true);
    bool OpenPath(const std::wstring& path, bool silent = false);
    void ChooseFiles();
    void OpenDroppedFiles(HDROP drop);
    void NewComparison(const std::wstring& left = L"", bool insertion_aware = true);
    void NewComparisonInputs(const std::wstring& left, const std::wstring& right, const std::wstring& base, int mode, bool run = false);
    bool PathLocked(const std::wstring& path) const;
    bool DocumentLocked(const simple_editor::core::BinaryDocument& document) const;
    bool LockPaths(const std::vector<std::wstring>& paths, bool acquire, const void* owner = nullptr,
                   const std::vector<simple_editor::core::FileStamp>& known = {});
    std::vector<simple_editor::core::WorkspaceEntry> CaptureWorkspace() const;
    void RestoreWorkspace(const std::vector<simple_editor::core::WorkspaceEntry>& entries);
    bool CloseSession(CMainFrame* session);
    void CancelCloseAll() { closing_all_ = false; close_queue_.clear(); preferences_close_pending_ = false; workspace_close_pending_ = false; recovery_close_pending_ = false; exit_workspace_.clear(); exit_order_.clear(); }
    bool IsTemporaryPath(const std::filesystem::path& path) const { return temporary_documents_.Owns(path); }
    void AppendFileActions(CMenu& menu);
    static void AppendExplorerMenu(CMenu& menu);
    void UpdateMenuShortcuts(CMenu& menu) const;
    void RememberRecent(const std::wstring& path);
    void RememberView(CMainFrame* frame);
    bool DispatchShortcut(MSG* message);
    void RefreshLabels();
    void TrackNavigation(CMainFrame* frame);
    void NavigateHistory(bool previous);
    std::vector<std::wstring> OpenPaths() const;
    std::vector<std::wstring> FolderPaths() const;
    std::unique_ptr<simple_editor::core::BinaryDocument> SnapshotForPath(const std::wstring& path, std::wstring* error);
    BOOL PreTranslateMessage(MSG* message) override;
    BOOL OnCmdMsg(UINT id, int code, void* extra, AFX_CMDHANDLERINFO* handler) override;
    friend class NativeUiTest;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT create);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* previous);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
    afx_msg void OnDropFiles(HDROP drop);
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    afx_msg void OnOpen();
    afx_msg void OnNewCompare();
    afx_msg void OnNewCompareEqual();
    afx_msg void OnAdvancedComparison(UINT id);
    afx_msg void OnSaveWorkspace();
    afx_msg void OnLoadWorkspace();
    afx_msg void OnAboutNative();
    afx_msg void OnSearchFiles();
    afx_msg void OnBatchReplace();
    afx_msg void OnWorkflowRoot(UINT command);
    afx_msg void OnNewFile();
    afx_msg void OnNewClipboard();
    afx_msg void OnReopenClosed();
    afx_msg void OnRecentFile(UINT id);
    afx_msg void OnTabAction(UINT id);
    afx_msg void OnPreferences();
    afx_msg void OnExplorerMenu(UINT command);
    afx_msg void OnUpdateExplorerMenu(CCmdUI* command);
    afx_msg void OnRecoveryCommand(UINT id);
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnCloseTab();
    afx_msg void OnTabChanged(NMHDR*, LRESULT* result);
    afx_msg void OnUpdateClose(CCmdUI* command);
    afx_msg void OnUpdateCompare(CCmdUI* command);
    afx_msg BOOL OnCopyData(CWnd*, COPYDATASTRUCT* data);
    afx_msg LRESULT OnQueuedOpen(WPARAM, LPARAM);
    afx_msg LRESULT OnContinueClose(WPARAM, LPARAM);
    afx_msg LRESULT OnContinueCloseMany(WPARAM,LPARAM);
    DECLARE_MESSAGE_MAP()

private:
    CMainFrame* CreateSession();
    void ActivateSession(int index);
    void Layout();
    void FocusActive();
    bool CaptureSearchSources(int mode,std::vector<simple_editor::services::MultiSearchSource>* sources,std::wstring* error);
    void NavigateSearchResult(const simple_editor::services::MultiSearchMatch& match);
    bool NewDocument(const std::vector<std::uint8_t>& bytes = {},simple_editor::core::TextEncoding encoding = simple_editor::core::TextEncoding::Utf8,int page = 1);
    void ReorderSessions(int from,int to);
    void TabContext(int index,CPoint point);
    void RefreshRecentMenus();
    bool LoadConfiguration(const std::filesystem::path& path,std::wstring* error);
    bool ChangePreferences(const simple_editor::core::AppPreferences& value,bool persist);
    void ApplyPreferencesToFrame(CMainFrame* frame);
    bool StartPreferenceWrite();
    void TickPreferences();
    bool CaptureSession(CMainFrame* frame,simple_editor::core::WorkspaceEntry* entry) const;
    void InitializeRecovery(bool load = true);
    bool StartRecoverySnapshot(bool clean_exit,bool manual = false);
    bool StartRecoveryLoad(bool manual);
    bool StartRecoveryRead(const simple_editor::core::RecoveryIndex& index,const std::vector<std::size_t>& items);
    bool StartRecoveryClear();
    void TickRecovery();
    void OfferRecovery(const simple_editor::core::RecoveryIndex& index);
    void StopRecovery();
    simple_editor::core::AppPreferences preferences_;
    std::filesystem::path preferences_path_;
    std::future<std::wstring> preferences_io_;
    bool preferences_dirty_ = false,preferences_close_pending_ = false,applying_preferences_ = false;
    simple_editor::core::TemporaryDocuments temporary_documents_;
    struct RecoveryJob {
        int kind = 0; bool manual = false;
        simple_editor::core::RecoveryIndex index;
        simple_editor::core::RecoverySaveResult saved;
        std::vector<std::pair<std::size_t,std::vector<std::uint8_t>>> payloads;
        std::wstring error;
    };
    std::future<RecoveryJob> recovery_io_;
    std::atomic_bool recovery_cancel_{false};
    std::atomic_uint recovery_progress_{0};
    std::filesystem::path recovery_root_;
    bool published_instance_ = false,recovery_initialized_ = false,recovery_paused_ = false,recovery_modal_ = false,recovery_close_pending_ = false;
    ULONGLONG recovery_last_snapshot_ = 0;
    std::map<std::uint64_t,simple_editor::core::WorkspaceEntry> exit_workspace_;
    std::vector<std::uint64_t> exit_order_;
    CMultiSearchDialog multi_search_;
    std::uint64_t next_session_id_ = 0;
    CWorkspaceTabs sessions_tab_;
    CButton open_button_, compare_button_, close_button_;
    CStatic welcome_;
    CMenu root_menu_;
    std::vector<CMainFrame*> sessions_;
    std::deque<std::wstring> queued_paths_;
    std::deque<std::wstring> recent_files_,closed_files_;
    std::deque<std::uint64_t> close_queue_;
    int context_session_ = -1;
    unsigned untitled_counter_ = 0;
    std::vector<std::wstring> untitled_aliases_;
    int active_session_ = -1;
    struct NavigationPoint { std::uint64_t session = 0,generation = 0,offset = 0; int page = 0; bool clone = false; };
    std::vector<NavigationPoint> navigation_;
    std::size_t navigation_index_ = 0;
    bool navigating_ = false;
    bool closing_all_ = false;
    bool close_prompt_active_ = false;
    bool destroying_ = false;
    struct LockGroup { std::set<std::wstring,simple_editor::core::OrdinalPathLess> paths; std::set<std::pair<DWORD,std::uint64_t>> identities; };
    std::set<std::wstring,simple_editor::core::OrdinalPathLess> locked_paths_;
    std::set<std::pair<DWORD,std::uint64_t>> locked_identities_;
    std::map<const void*,LockGroup> lock_groups_;
    struct WorkspaceIO { bool loading = false, saved = false; std::wstring path, error; std::vector<simple_editor::core::WorkspaceEntry> entries; };
    std::future<WorkspaceIO> workspace_io_;
    bool workspace_close_pending_ = false, workspace_dialog_ = false;
    bool workspace_io_writes_ = false;
    std::wstring workspace_io_path_;
};

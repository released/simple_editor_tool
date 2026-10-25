#pragma once

#include <afxcmn.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxwin.h>
#include <atomic>
#include <future>
#include <climits>
#include <vector>
#include <set>
#include "core/binary_document.h"
#include "core/diff_engine.h"
#include "hex_view.h"
#include "core/text_codec.h"
#include "core/text_output.h"
#include "core/compare_region.h"
#include "ui/editor_dialogs.h"
#include "ui/splitter_bar.h"
#include "ui/wheel_zoom_control.h"
#include "ui/advanced_compare.h"
#include "ui/path_combo.h"
#include "ui/code_editor.h"
#include "ui/live_inspector.h"
#include "ui/log_tail_dialog.h"
#include "ui/search_results.h"
#include "services/editor_job.h"
#include "services/difference_mailbox.h"

class CWorkspaceFrame;

class CMainFrame : public CFrameWnd {
public:
    explicit CMainFrame(bool publish_instance = true, CWorkspaceFrame* workspace = nullptr);
    ~CMainFrame() override;
    bool OpenPath(const std::wstring& path);
    BOOL PreTranslateMessage(MSG* message) override;
    friend class NativeUiTest;
    friend class CWorkspaceFrame;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT create);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* info);
    afx_msg void OnDropFiles(HDROP drop);
    afx_msg void OnClose();
    afx_msg void OnDestroy();
    afx_msg LRESULT OnSavedClose(WPARAM, LPARAM);
    afx_msg void OnFileOpen();
    afx_msg void OnReload();
    afx_msg void OnSave();
    afx_msg void OnSaveAs();
    afx_msg void OnCompare();
    afx_msg void OnCompareEqual();
    afx_msg void OnCompareCancel();
    afx_msg void OnUndo();
    afx_msg void OnRedo();
    afx_msg void OnInsert();
    afx_msg void OnDelete();
    afx_msg void OnHexCommand(UINT command);
    afx_msg void OnExportBinaryReport();
    afx_msg void OnEditorCommand(UINT command);
    afx_msg void OnBinaryTool(UINT command);
    afx_msg void OnConvertCopy();
    afx_msg void OnBinaryMerge(UINT command);
    afx_msg void OnComparisonRegion();
    afx_msg void OnExportTextReport();
    afx_msg void OnApplyPatch();
    afx_msg void OnAboutNative();
    afx_msg void OnTemplate();
    afx_msg void OnTextPager();
    afx_msg void OnFeature(UINT command);
    afx_msg void OnWorkflow(UINT command);
    bool StartWorkflowJob(UINT command,const simple_editor::core::IntegerTransform& transform = {},unsigned minimum = 4,unsigned kinds = 7);
    void FinishWorkflowJob(EditorJobResult result);
    void ExportExtractedStrings();
    bool StartStringExport(const std::wstring& path);
    void NavigateModification(bool previous);
    void BookmarkCommand(bool add);
    void ShowLiveInspector();
    void StartTextCommand(UINT command);
    void FinishTextCommand(EditorJobResult result);
    void ToggleTextClone();
    bool StartCustomCrc(const std::wstring& settings);
    void CustomCrcDialog();
    bool IsFileEditor(const CCodeEditor* editor) const { return editor == &text_ || editor == &text_clone_; }
    afx_msg void OnTextChanged();
    afx_msg void OnFind();
    afx_msg void OnReplace();
    afx_msg void OnFindNext();
    afx_msg void OnGoTo();
    afx_msg void OnPreviousDifference();
    afx_msg void OnNextDifference();
    afx_msg void OnCompareBrowseLeft();
    afx_msg void OnCompareBrowseRight();
    afx_msg void OnCompareRun();
    afx_msg void OnCompareSwap();
    afx_msg void OnComparePathChanged();
    afx_msg void OnCompareModeChanged();
    afx_msg void OnCompareBrowseBase();
    afx_msg void OnEncoding(UINT id);
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnUpdateJobCommand(CCmdUI* command);
    afx_msg void OnUpdateSaveCommand(CCmdUI* command);
    afx_msg void OnUpdateZoomCommand(CCmdUI* command);
    afx_msg void OnTabChanging(NMHDR*, LRESULT* result);
    afx_msg void OnTabChanged(NMHDR*, LRESULT* result);
    afx_msg void OnDifferenceDisplay(NMHDR*, LRESULT* result);
    afx_msg void OnDifferenceActivate(NMHDR*, LRESULT* result);
    afx_msg BOOL OnCopyData(CWnd*, COPYDATASTRUCT* data);
    DECLARE_MESSAGE_MAP()

private:
    enum class AfterSave { None, Open, OpenDialog, Reload, Close, CompareInsertion, CompareEqual };
    void Layout();
    void ShowPage(int tab);
    void InitializeZoom();
    void ZoomWheel(int tab, short delta);
    bool SetTabZoom(int tab, int percent);
    void UpdateZoomStatus();
    void UpdateDifferenceColumns();
    void DocumentChanged();
    CHexView* ActiveHex();
    void HexSelectionChanged();
    CCodeEditor* ActiveEditor();
    bool StartBinaryReport(const std::wstring& path, int format);
    bool StartBinaryTool(UINT command, const std::wstring& output, const std::vector<std::uint8_t>& pattern = {}, const std::vector<std::filesystem::path>& paths = {}, std::uint64_t part = 0);
    bool StartConvertedCopy(const std::wstring& output,const simple_editor::core::TextOutputOptions& options);
    bool StartBinaryMerge(const std::wstring& output,bool right_into_left,const std::set<std::size_t>& selected);
    void ReleaseJobLocks();
    void ClearComparison();
    bool IsBusy() const;
    bool IsSaving() const;
    bool StartSave(const std::wstring& destination, AfterSave next = AfterSave::None,
                   const std::wstring& next_path = L"");
    bool SaveCurrent(AfterSave next = AfterSave::None,const std::wstring& next_path = L"",bool choose = false);
    void FinishSave(EditorJobResult result);
    void SetSaveReadOnly(bool saving);
    void ExecuteSearch(const SearchRequest& request);
    void ExecuteEditorSearch(const SearchRequest& request, CCodeEditor* editor);
    void FinishEditorSearch(EditorJobResult result);
    void UnlockEditorSearch();
    void ExecuteByteSearch(const SearchRequest& request);
    void FinishByteSearch(EditorJobResult result);
    bool StartPatch(const std::wstring& path);
    void FinishPatch(EditorJobResult result);
    void NavigateDifference(int index);
    void SynchronizeCompareScroll(bool from_left,std::uint64_t row);
    void FinishJob();
    void DrainDifferences(unsigned batches = 8);
    std::shared_ptr<DifferenceMailbox> difference_mailbox_;
    bool StartTemplate(const std::wstring& path,std::uint64_t base);
    void FinishTemplate(EditorJobResult result);
    bool ConfirmDiscardChanges(AfterSave next = AfterSave::None, const std::wstring& next_path = L"");
    void RefreshText();
    bool ApplyTextEdits();
    bool PrepareComparison(bool insertion_aware = true);
    void StartCompare(const std::wstring& right, bool insertion_aware);
    void SetComparisonPaths(const std::wstring& left, const std::wstring& right, bool insertion_aware);
    void BrowseComparisonPath(bool left);
    void SetComparisonControlsEnabled(bool enabled);
    bool AdvancedActive() const { return active_tab_ == 2 && displayed_mode_ >= 2; }
    bool MutationBlocked() const;
    void InitializeAdvanced();
    bool EnsureAdvanced();
    CWorkspaceFrame* workspace_ = nullptr;
    std::uint64_t workspace_session_id_ = 0;
    bool untitled_ = false;
    bool untitled_dirty_ = false;
    std::wstring untitled_name_;
    CMenu session_menu_;
    bool comparison_session_ = false;
    bool suppress_compare_path_change_ = false;
    bool saved_close_pending_ = false;
    CComboBox compare_left_path_, compare_right_path_, compare_mode_;
    CButton compare_browse_left_, compare_browse_right_, compare_run_, compare_swap_;
    CComboBox compare_base_path_;
    CStatic compare_base_label_;
    CButton compare_browse_base_;
    CAdvancedCompareView advanced_;
    int displayed_mode_ = 0;
    bool path_dialog_ = false;
    CRect compare_view_rect_;
    simple_editor::core::BinaryDocument document_;
    simple_editor::core::HexBookmarks bookmarks_;
    CLiveInspector live_inspector_;
    CLogTailDialog log_tail_;
    std::wstring crc_profile_;
    std::vector<simple_editor::core::ModifiedRange> modified_ranges_;
    std::shared_ptr<simple_editor::core::StringExtraction> strings_;
    std::uint64_t strings_revision_ = UINT64_MAX,modified_revision_ = UINT64_MAX;
    simple_editor::core::ProtectedInput strings_source_;
    simple_editor::core::BinaryDocument compare_right_document_;
    simple_editor::core::BinaryDocument compare_left_document_;
    CWheelZoomControl<CHexView> hex_;
    CWheelZoomControl<CHexView> compare_left_;
    CWheelZoomControl<CHexView> compare_right_;
    CStatic compare_left_label_;
    CStatic compare_right_label_;
    CSplitterBar vertical_splitter_;
    CSplitterBar horizontal_splitter_;
    CRect page_rect_;
    double horizontal_ratio_ = 0.5;
    double vertical_ratio_ = 0.66;
    CWheelZoomControl<CCodeEditor> text_;
    CWheelZoomControl<CCodeEditor> text_clone_;
    bool dual_view_ = false,clone_active_ = false;
    CFont text_font_;
    std::wstring text_face_ = L"Consolas";
    unsigned content_font_tenths_ = 100,content_theme_ = 0;
    bool dark_content_ = false;
    CWheelZoomControl<CListCtrl> differences_;
    CFont comparison_font_;
    LOGFONT comparison_base_font_{};
    int zoom_percent_[3] = {100, 100, 100};
    int wheel_remainder_[3] = {};
    CTabCtrl tabs_;
    CStatusBar status_;
    std::vector<simple_editor::core::Difference> pending_differences_;
    std::future<EditorJobResult> job_;
    std::atomic_bool cancelled_;
    std::uint64_t revision_ = 0;
    std::uint64_t job_revision_ = 0;
    EditorJobKind job_kind_ = EditorJobKind::Compare;
    AfterSave after_save_ = AfterSave::None;
    bool user_read_only_ = false, job_locks_held_ = false;
    bool active_compare_right_ = false;
    std::vector<std::wstring> job_locked_paths_;
    std::wstring after_save_path_;
    unsigned int shown_progress_ = UINT_MAX;
    ULONGLONG source_check_tick_ = 0;
    std::atomic_uint progress_{0};
    bool suppress_text_change_;
    bool text_dirty_;
    bool text_stale_ = true;
    bool text_editable_ = false;
    simple_editor::core::TextEncoding requested_encoding_ = simple_editor::core::TextEncoding::Auto;
    simple_editor::core::TextCodec text_codec_;
    std::wstring text_original_;
    CSearchDialog search_dialog_;
    CSearchResultsDialog search_results_;
    CCodeEditor* job_editor_ = nullptr;
    std::uint64_t job_editor_generation_ = 0, search_input_size_ = 0;
    CCodeEditor* scope_editor_ = nullptr;
    std::wstring scope_pattern_;
    std::uint64_t scope_generation_ = 0, scope_begin_ = 0, scope_end_ = 0, scope_last_begin_ = 0, scope_last_end_ = 0;
    CHexView* byte_search_view_ = nullptr;
    std::wstring byte_search_key_;
    std::uint64_t byte_scope_revision_ = 0, byte_scope_begin_ = 0, byte_scope_end_ = 0, byte_last_match_ = UINT64_MAX, byte_last_length_ = 0;
    SearchRequest last_search_;
    std::vector<std::uint8_t> match_pattern_;
    std::uint64_t match_offset_ = UINT64_MAX;
    int active_tab_ = 0;
    int difference_index_ = -1;
    simple_editor::core::CompareRegion compare_region_;
    std::wstring compare_region_spec_;
    std::uint64_t compare_origin_left_ = 0, compare_origin_right_ = 0;
};


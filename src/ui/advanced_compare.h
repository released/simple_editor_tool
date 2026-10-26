#pragma once
#include <afxcmn.h>
#include <afxdlgs.h>
#include <afxwin.h>
#include <atomic>
#include <future>
#include <climits>
#include <memory>
#include <tuple>
#include "wheel_zoom_control.h"
#include "code_editor.h"
#include "diff_overview.h"
#include "../core/diff_presentation.h"
#include "../core/text_report.h"
#include "../core/line_merge.h"
#include "../core/folder_sync.h"
#include "../core/folder_policy.h"
#include "../core/structured_compare.h"
#include "../core/rename_pairs.h"
#include "../core/workspace_session.h"
#include "../core/workflow_views.h"
#include "../core/text_review.h"
#include "../core/review_notes.h"

class CAdvancedCompareView : public CWnd {
public:
    std::function<void(const std::wstring&)> report;
    std::function<bool(const std::vector<std::wstring>&, bool, const std::vector<simple_editor::core::FileStamp>&)> lock_paths;
    std::function<void(bool)> saved_close;
    std::function<void(const std::wstring&, const std::wstring&, int)> open_pair;
    std::function<void(short)> wheel_zoom;
    std::function<void(bool)> busy_changed;
    std::function<void()> changed;
    bool Busy() const { return job_.valid() || modal_ || search_busy_; }
    void SearchBusy(bool busy) { search_busy_ = busy; SetBusy(busy); }
    bool PendingClose() const { return close_after_save_; }
    bool Dirty() const { return dirty_ || notes_dirty_ || HasPastedInputs(); }
    bool CanClose();
    bool CanReset();
    void Configure(int mode);
    bool SetPastedInputs(bool enabled);
    bool HasPastedInputs() const;
    void StartPastedText();
    void SetSnapshotInputs(const std::wstring& left,const std::wstring& right,const std::wstring& path,const simple_editor::core::TextCodec& codec,const simple_editor::core::FileStamp& stamp);
    void SwapPastedInputs();
    void InvalidateInputs();
    void StartText(std::unique_ptr<simple_editor::core::BinaryDocument> base,
                   std::unique_ptr<simple_editor::core::BinaryDocument> left,
                   std::unique_ptr<simple_editor::core::BinaryDocument> right);
    void StartFolders(const std::wstring& left, const std::wstring& right);
    void Tick();
    void Cancel();
    void SaveResult();
    bool SaveTo(const std::wstring& path);
    void Undo(bool redo = false);
    void Navigate(bool previous, bool conflicts_only = false);
    void SetZoom(int percent);
    void SetAppearance(const std::wstring& face,unsigned font_tenths,unsigned theme);
    void SetTextViewPreferences(bool wrap,bool whitespace,unsigned tab_width,bool use_tabs);
    void Find(const std::wstring& text);
    CCodeEditor* OutputEditor() { return model_ && mode_ != 4 && !manual_view_ ? &output_ : nullptr; }
    CCodeEditor* SearchEditor();
    std::wstring RawOutputText() const;
    simple_editor::core::TextCodec OutputCodec() const { return codec_; }
    void ExportReportDialog();
    bool StartTextReport(const std::wstring& path,simple_editor::core::TextReportFormat format);
    void SetEncoding(simple_editor::core::TextEncoding encoding);
    void CapturePreferences(simple_editor::core::WorkspaceEntry& entry) const;
    void RestorePreferences(const simple_editor::core::WorkspaceEntry& entry);
    void FeatureSettings(UINT command);
    void WorkflowSettings(UINT command);
    void ReviewMovedBlocks();
    void ToggleBaseSource();
    bool AddSelectedAnchor();
    void ManageAnchors();
    bool RefreshSelectedFolders();
    simple_editor::core::FolderOptions CurrentFolderOptions() const;
    struct FolderFile { std::wstring relative,left,right; int mode = 0; };
    std::wstring FolderPairKey(const std::wstring& left,const std::wstring& right) const;
    bool AdjacentFolderFile(const std::wstring& relative,int direction,FolderFile*) const;
    std::uint64_t FolderGeneration() const { return folder_generation_; }
    void ShowAnchorMarkers();
    bool LoadPastedPair(const std::wstring& left,const std::wstring& right);
    void NotesCommand(UINT command);
    bool SetReviewNote(std::size_t block,const std::wstring&,bool reviewed,std::wstring* error);
    bool StartNotesIo(const std::wstring& path,bool save,bool report = false);
    bool ConfirmNotesClose();
    void EditJsonRules();
    void StartContentPairing();
    void FinishContentPairing(const std::vector<simple_editor::core::ContentPair>& pairs);
    friend class NativeUiTest;
    ~CAdvancedCompareView() override;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT create);
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg void OnDisplay(NMHDR*, LRESULT* result);
    afx_msg void OnDraw(NMHDR*, LRESULT* result);
    afx_msg void OnDoubleClick(NMHDR*, LRESULT* result);
    afx_msg void OnAction(UINT id);
    afx_msg void OnResultChanged();
    afx_msg void OnPastedInputChanged();
    afx_msg void OnFilterChanged();
    afx_msg void OnRulesChanged();
    afx_msg void OnSimilarChanged();
    afx_msg void OnFolderColumn(NMHDR*,LRESULT*);
    afx_msg void OnFolderAction(UINT id);
    DECLARE_MESSAGE_MAP()

private:
    enum class JobKind { Text, Render, Scan, Refresh, Plan, Sync, Save, Presentation, Report, Structured, Pairing, NotesLoad, NotesSave };
    struct Result {
        JobKind kind = JobKind::Text;
        std::shared_ptr<simple_editor::core::TextMergeModel> model;
        std::shared_ptr<simple_editor::core::FolderScan> scan;
        simple_editor::core::FolderOptions folder_options;
        simple_editor::core::SyncPlan plan;
        simple_editor::core::SyncBundle bundle;
        std::shared_ptr<simple_editor::core::StructuredComparison> structured;
        std::shared_ptr<std::vector<simple_editor::core::ContentPair>> pairs;
        bool anchored = false;
        simple_editor::core::SyncResult sync;
        simple_editor::core::TextCodec codec;
        simple_editor::core::FileStamp written;
        std::wstring text, error, output_path,codec_details,review_fingerprint;
        std::shared_ptr<simple_editor::core::ComparisonNotes> review_notes;
        bool saved = false;
        std::size_t extra_conflicts = 0;
        std::shared_ptr<simple_editor::core::DiffPresentation> presentation;
        simple_editor::core::LineOptions options;
        bool similar = false;
        std::vector<simple_editor::core::MovedBlock> moves;
        std::vector<simple_editor::core::ProtectedInput> inputs;
    };
    void Layout();
    void FinishNotesIo(Result&);
    std::wstring NoteCell(std::size_t block) const;
    void LayoutPastedInputs(int width, int height, int* top);
    bool CreateSourceEditors();
    void RefreshSources();
    void SourceChanged(unsigned side);
    void SelectSourceRow(std::size_t row);
    afx_msg void OnSourceRowChanged(NMHDR*, LRESULT* result);
    CCodeEditor* FocusedInput();
    void StartTextJob(std::unique_ptr<simple_editor::core::BinaryDocument> base,
                      std::unique_ptr<simple_editor::core::BinaryDocument> left,
                      std::unique_ptr<simple_editor::core::BinaryDocument> right,
                      std::wstring left_text, std::wstring right_text, bool pasted);
    void StartStructured(std::unique_ptr<simple_editor::core::BinaryDocument> left,std::unique_ptr<simple_editor::core::BinaryDocument> right);
    void Columns();
    void SetBusy(bool busy);
    void Status(const std::wstring& text);
    void AdoptText(std::wstring text, bool initial);
    void RebuildRows();
    void OrderFolderRows();
    bool FolderRowVisible(const simple_editor::core::FolderRow& row) const;
    void Take(simple_editor::core::MergeChoice choice);
    void RenderAsync();
    void PlanSync(bool left_to_right, bool all);
    void ApplyPlan(simple_editor::core::SyncPlan plan);
    void ApplyBundle(simple_editor::core::SyncBundle bundle);
    void ChooseBaseline(bool save);
    bool StartBaselineSave(const std::wstring& path);
    void ChooseJournal(bool recover);
    bool StartJournalPlan(const std::wstring& path,bool recover);
    void ReleaseLocks();
    void RenderCheckpoint();
    void RestoreChoiceAction(int token, bool undo);
    void RefreshOutputState();
    std::wstring Cell(int row, int column) const;
    int mode_ = 2;
    CWheelZoomControl<CListCtrl> rows_;
    CDiffOverview overview_;
    CWheelZoomControl<CCodeEditor> output_;
    CWheelZoomControl<CCodeEditor> input_left_, input_right_;
    CWheelZoomControl<CCodeEditor> source_left_, source_right_, source_base_;
    CCodeEditor* review_focus_ = nullptr;
    CStatic source_left_label_, source_right_label_,source_base_label_;
    std::wstring source_left_path_,source_right_path_,source_base_path_;
    bool show_base_source_ = true;
    std::vector<int> left_peer_,right_peer_;
    std::intptr_t source_top_[3] = {-1,-1,-1};
    bool source_sync_ = false;
    bool source_aligned_ = false;
    CStatic input_left_label_, input_right_label_;
    bool pasted_inputs_ = false;
    bool read_only_snapshots_ = false;
    std::vector<simple_editor::core::ProtectedInput> snapshot_protection_;
    simple_editor::core::TextCodec snapshot_codec_;
    CEdit status_, include_, exclude_;
    CStatic output_label_, include_label_, exclude_label_, encoding_label_;
    CStatic encoding_right_label_,encoding_base_label_,codec_details_;
    CComboBox encoding_,encoding_right_,encoding_base_;
    CButton ignore_space_, ignore_case_, ignore_eol_, changes_only_, conflicts_only_, syntax_;
    CButton similar_;
    CButton take_left_, take_right_, take_base_, take_both_, undo_, redo_, save_, allow_markers_, manual_resolved_button_;
    CButton recursive_, show_equal_, mirror_, copy_right_, copy_left_, sync_right_, sync_left_;
    CComboBox folder_criterion_, folder_filter_, folder_tolerance_;
    CComboBox sync_policy_;
    CButton baseline_save_, baseline_load_, propagate_deletions_;
    CButton journal_retry_,journal_recover_,backup_browse_;
    std::wstring baseline_path_,last_journal_;
    unsigned folder_sort_ = 0;
    bool folder_sort_descending_ = false;
    CFont content_font_, ui_font_;
    std::shared_ptr<simple_editor::core::TextMergeModel> model_;
    simple_editor::core::ComparisonNotes notes_;
    std::wstring review_fingerprint_,notes_path_;
    bool notes_dirty_ = false;
    std::vector<simple_editor::core::MovedBlock> moves_;
    std::vector<simple_editor::core::ProtectedInput> report_inputs_;
    std::shared_ptr<simple_editor::core::FolderScan> scan_;
    simple_editor::core::FolderOptions scan_options_;
    std::set<std::wstring,simple_editor::core::OrdinalPathLess> refresh_selected_,refresh_expanded_;
    std::wstring refresh_top_;
    std::uint64_t folder_generation_ = 0;
    std::shared_ptr<simple_editor::core::StructuredComparison> structured_;
    std::vector<simple_editor::core::TextMergeRow> text_rows_;
    std::shared_ptr<simple_editor::core::DiffPresentation> presentation_;
    simple_editor::core::LineOptions line_options_;
    bool presentation_similar_ = false;
    std::vector<std::size_t> visible_;
    std::vector<simple_editor::core::MergeChoice> render_before_;
    struct ChoiceAction { std::vector<std::tuple<std::size_t,simple_editor::core::MergeChoice,simple_editor::core::MergeChoice>> delta; std::size_t before_extra = 0, after_extra = 0; };
    std::vector<ChoiceAction> choice_actions_;
    std::size_t extra_conflicts_ = 0;
    std::future<Result> job_;
    JobKind job_kind_ = JobKind::Text;
    std::atomic_bool cancelled_{false};
    std::atomic_uint progress_{0};
    unsigned shown_progress_ = UINT_MAX;
    simple_editor::core::TextCodec codec_;
    simple_editor::core::FileStamp saved_stamp_;
    std::wstring generated_, display_generated_, saved_text_, saved_path_, folder_left_, folder_right_, sync_summary_;
    std::vector<std::wstring> locked_paths_;
    std::vector<simple_editor::core::FileStamp> locked_stamps_;
    bool suppress_result_ = false, dirty_ = false, manual_ = false, stale_ = false, close_after_save_ = false;
    bool locks_held_ = false;
    bool modal_ = false;
    bool search_busy_ = false;
    std::size_t unresolved_ = 0;
    std::size_t model_unresolved_ = 0;
    bool manual_resolved_ = false;
    simple_editor::core::WorkspaceEntry pending_preferences_;
    bool has_pending_preferences_ = false;
    std::wstring table_rules_,text_rules_,json_rules_,manual_anchors_;
    bool manual_view_ = false;
    std::wstring folder_text_rules_;
    unsigned context_lines_ = 0;
    bool folder_tree_ = false;
    std::set<std::size_t> context_expanded_,tree_expanded_;
    simple_editor::core::ContextRows context_rows_;
    simple_editor::core::FolderTreeRows tree_rows_;
    std::vector<std::size_t> eligible_folder_rows_;
    int zoom_percent_ = 100;
    std::wstring font_face_ = L"Consolas";
    unsigned font_tenths_ = 100,theme_ = 0;
    bool wrap_text_ = false,whitespace_ = false,use_tabs_ = false;
    unsigned tab_width_ = 4;
};

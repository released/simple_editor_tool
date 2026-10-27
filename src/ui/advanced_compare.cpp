#include "advanced_compare.h"
#include "../core/manual_alignment.h"
#include "../resource.h"
#include "localization.h"
#include "ui_font.h"
#include <algorithm>
#include <chrono>
#include <cwctype>
#include <set>

using namespace simple_editor::core;
namespace {
constexpr int kRows = 540, kOutput = 541;
const wchar_t* State(FolderState state) {
    switch (state) { case FolderState::Equal: return L"Equal"; case FolderState::Different: return L"Different"; case FolderState::LeftOnly: return L"Left only"; case FolderState::RightOnly: return L"Right only"; case FolderState::TypeConflict: return L"Type conflict"; default: return L"Error"; }
}
bool DecodeDocument(BinaryDocument* document, TextEncoding encoding, std::wstring* text, TextCodec* codec, std::wstring* error) {
    if (!document || document->Size() > 16 * 1024 * 1024) { *error = L"Text merge supports files up to 16 MiB per input. Use Hex for larger files."; return false; }
    std::vector<std::uint8_t> bytes;
    if (!document->Read(0, static_cast<std::size_t>(document->Size()), &bytes) || !document->ValidateSource(true, error)) { if (error->empty()) *error = L"Source changed while reading."; return false; }
    return DecodeText(bytes, encoding, text, codec, error);
}
}
BEGIN_MESSAGE_MAP(CAdvancedCompareView, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_NOTIFY(LVN_GETDISPINFO, kRows, OnDisplay)
    ON_NOTIFY(NM_CUSTOMDRAW, kRows, OnDraw)
    ON_NOTIFY(NM_DBLCLK, kRows, OnDoubleClick)
    ON_NOTIFY(LVN_COLUMNCLICK, kRows, OnFolderColumn)
    ON_COMMAND_RANGE(ID_ADV_TAKE_LEFT, ID_ADV_MANUAL_RESOLVED, OnAction)
    ON_CONTROL(EN_CHANGE, kOutput, OnResultChanged)
    ON_CONTROL(EN_CHANGE, 582, OnPastedInputChanged)
    ON_CONTROL(EN_CHANGE, 583, OnPastedInputChanged)
    ON_CONTROL(BN_CLICKED, IDC_ADV_CHANGES, OnFilterChanged)
    ON_CONTROL(BN_CLICKED, IDC_ADV_CONFLICTS, OnFilterChanged)
    ON_CONTROL(BN_CLICKED, IDC_ADV_SHOW_EQUAL, OnFilterChanged)
    ON_CONTROL(BN_CLICKED, 555, OnFilterChanged)
    ON_CONTROL(BN_CLICKED, IDC_ADV_SIMILAR, OnSimilarChanged)
    ON_CONTROL(BN_CLICKED, 550, OnRulesChanged)
    ON_CONTROL(BN_CLICKED, 551, OnRulesChanged)
    ON_CONTROL(BN_CLICKED, 552, OnRulesChanged)
    ON_CONTROL(CBN_SELCHANGE, 565, OnRulesChanged)
    ON_CONTROL(CBN_SELCHANGE, 610, OnRulesChanged)
    ON_CONTROL(CBN_SELCHANGE, 611, OnRulesChanged)
    ON_CONTROL(CBN_SELCHANGE, 572, OnRulesChanged)
    ON_CONTROL(CBN_SELCHANGE, 573, OnFilterChanged)
    ON_CONTROL(CBN_SELCHANGE, 574, OnRulesChanged)
    ON_NOTIFY(LVN_ITEMCHANGED, kRows, OnSourceRowChanged)
    ON_COMMAND_RANGE(575,581,OnFolderAction)
END_MESSAGE_MAP()
CAdvancedCompareView::~CAdvancedCompareView() { cancelled_ = true; if (job_.valid()) job_.wait(); ReleaseLocks(); }
int CAdvancedCompareView::OnCreate(LPCREATESTRUCT create) {
    if (CWnd::OnCreate(create) == -1) return -1;
    if (!simple_editor::ui::CreateUiFont(ui_font_, *this)) return -1;
    content_font_.CreatePointFont(100, L"Consolas");
    const auto button = [&](CButton& control, const wchar_t* text, int id, bool check = false) {
        control.Create(text, WS_CHILD | WS_TABSTOP | (check ? BS_AUTOCHECKBOX : BS_PUSHBUTTON), CRect(), this, id); control.SetFont(&ui_font_);
    };
    button(ignore_space_, L"Ignore spaces", 550, true); button(ignore_case_, L"Ignore case", 551, true); button(ignore_eol_, L"Ignore EOL", 552, true);
    button(changes_only_, L"Changes only", IDC_ADV_CHANGES, true); button(conflicts_only_, L"Conflicts only", IDC_ADV_CONFLICTS, true); button(syntax_, L"Syntax", 555, true); syntax_.SetCheck(BST_CHECKED);
    button(similar_,L"Align similar lines (2-way)",IDC_ADV_SIMILAR,true);
    button(take_left_, L"Take left", ID_ADV_TAKE_LEFT); button(take_right_, L"Take right", ID_ADV_TAKE_RIGHT); button(take_base_, L"Take base", ID_ADV_TAKE_BASE); button(take_both_, L"Take both", ID_ADV_TAKE_BOTH);
    button(undo_, L"Undo", ID_ADV_UNDO); button(redo_, L"Redo", ID_ADV_REDO); button(save_, L"Save result...", ID_ADV_SAVE);
    button(allow_markers_, L"Allow unresolved markers", 556, true);
    button(manual_resolved_button_,L"Mark manual result resolved",ID_ADV_MANUAL_RESOLVED);
    button(recursive_, L"Subfolders", 557, true); recursive_.SetCheck(BST_CHECKED);
    button(show_equal_, L"Show equal", IDC_ADV_SHOW_EQUAL, true); show_equal_.SetCheck(BST_CHECKED);
    button(mirror_, L"Mirror: move target-only files to backup", 558, true);
    button(copy_right_, L"Selected ->", ID_ADV_COPY_RIGHT); button(copy_left_, L"<- Selected", ID_ADV_COPY_LEFT); button(sync_right_, L"Sync all ->...", ID_ADV_SYNC_RIGHT); button(sync_left_, L"<- Sync all...", ID_ADV_SYNC_LEFT);
    folder_criterion_.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,CRect(),this,572);
    for (auto value : {L"Compare: content",L"Compare: size",L"Compare: size + time",L"Compare: SHA-256"}) folder_criterion_.AddString(value);
    folder_criterion_.SetCurSel(0); folder_criterion_.SetFont(&ui_font_);
    folder_filter_.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,CRect(),this,573);
    for (auto value : {L"Filter: all states",L"Different",L"Left only",L"Right only",L"Conflicts / errors",L"Left newer",L"Right newer",L"Equal",L"Same-time different",L"Important text differences",L"Equal under text rules",L"Text analysis errors"}) folder_filter_.AddString(value);
    folder_filter_.SetCurSel(0); folder_filter_.SetFont(&ui_font_);
    folder_tolerance_.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,CRect(),this,574);
    for (auto value : {L"Time tolerance: exact",L"Time tolerance: 1 second",L"Time tolerance: 2 seconds"}) folder_tolerance_.AddString(value);
    folder_tolerance_.SetCurSel(0); folder_tolerance_.SetFont(&ui_font_);
    sync_policy_.Create(WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,CRect(),this,577);
    for (auto value : {L"Sync: directed update",L"Sync: directed newer only",L"Sync: newer both ways",L"Sync: baseline both ways"}) sync_policy_.AddString(value);
    sync_policy_.SetCurSel(0); sync_policy_.SetFont(&ui_font_);
    button(baseline_save_,L"Save baseline...",575); button(baseline_load_,L"Choose baseline...",576); button(propagate_deletions_,L"Baseline: propagate deletions",578,true);
    button(journal_retry_,L"Retry journal...",579); button(journal_recover_,L"Recover journal...",580); button(backup_browse_,L"Browse backups",581);
    include_label_.Create(L"Include:", WS_CHILD, CRect(), this, 560); exclude_label_.Create(L"Exclude:", WS_CHILD, CRect(), this, 561);
    include_.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, CRect(), this, 562); include_.SetWindowTextW(L"*");
    exclude_.Create(WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, CRect(), this, 563);
    encoding_label_.Create(L"Left encoding:", WS_CHILD, CRect(), this, 564);
    encoding_.Create(WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, CRect(), this, 565);
    for (auto value : {L"Auto",L"UTF-8",L"UTF-16 LE",L"UTF-16 BE",L"Big5 / CP950",L"System"}) encoding_.AddString(value); encoding_.SetCurSel(0);
    encoding_right_label_.Create(L"Right encoding:",WS_CHILD,CRect(),this,612); encoding_base_label_.Create(L"Base encoding:",WS_CHILD,CRect(),this,613);
    encoding_right_.Create(WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,CRect(),this,610); encoding_base_.Create(WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,CRect(),this,611);
    for (auto* combo : {&encoding_right_,&encoding_base_}) { for (auto value : {L"Auto",L"UTF-8",L"UTF-16 LE",L"UTF-16 BE",L"Big5 / CP950",L"System"}) combo->AddString(value); combo->SetCurSel(0); combo->SetFont(&ui_font_); }
    codec_details_.Create(L"",WS_CHILD,CRect(),this,614); for (auto* label : {&encoding_right_label_,&encoding_base_label_,&codec_details_}) label->SetFont(&ui_font_);
    rows_.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA, CRect(), this, kRows);
    rows_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER); rows_.SetFont(&content_font_);
    overview_.Create(this,571); overview_.navigate = [this](std::size_t row) { if (stale_ || row >= visible_.size()) return; rows_.SetItemState(-1,0,LVIS_SELECTED); rows_.SetItemState(static_cast<int>(row),LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED); rows_.EnsureVisible(static_cast<int>(row),FALSE); rows_.SetFocus(); };
    output_label_.Create(L"Merged output (editable). Save result writes only the chosen output file.", WS_CHILD, CRect(), this, 566);
    output_.Create(WS_CHILD | WS_BORDER | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_WANTRETURN | WS_TABSTOP, CRect(), this, kOutput);
    output_.SetLimitText(128 * 1024 * 1024); output_.SetFont(&content_font_);
    SetTextViewPreferences(wrap_text_,whitespace_,tab_width_,use_tabs_);
    output_.undo_container = [this](int token,bool undo) { RestoreChoiceAction(token,undo); };
    status_.Create(WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL, CRect(), this, 567);
    for (CWnd* control : {static_cast<CWnd*>(&status_),static_cast<CWnd*>(&include_),static_cast<CWnd*>(&exclude_),static_cast<CWnd*>(&include_label_),static_cast<CWnd*>(&exclude_label_),static_cast<CWnd*>(&encoding_),static_cast<CWnd*>(&encoding_label_),static_cast<CWnd*>(&output_label_)}) control->SetFont(&ui_font_);
    rows_.wheel_zoom = output_.wheel_zoom = [this](short delta) { if (wheel_zoom) wheel_zoom(delta); };
    if (!CreateSourceEditors()) return -1; Configure(mode_); if (has_pending_preferences_) RestorePreferences(pending_preferences_); simple_editor::ui::RegisterLocalizationRoot(this); return 0;
}
void CAdvancedCompareView::OnDestroy() { cancelled_ = true; CWnd::OnDestroy(); }
void CAdvancedCompareView::OnSize(UINT type, int cx, int cy) { CWnd::OnSize(type, cx, cy); Layout(); }
void CAdvancedCompareView::Configure(int mode) {
    const bool mode_changed = mode_ != mode;
    mode_ = mode;
    if (!rows_.GetSafeHwnd()) return;
    if (mode_changed) {
        presentation_.reset(); text_rows_.clear();
        model_.reset(); scan_.reset(); structured_.reset(); visible_.clear(); generated_.clear(); saved_text_.clear(); saved_path_.clear(); dirty_ = manual_ = false; stale_ = true;
        suppress_result_ = true; output_.SetWindowTextW(L""); suppress_result_ = false;
    }
    while (rows_.DeleteColumn(0)) {}
    if (mode == 4) for (auto label : {L"Relative path",L"State",L"Left bytes",L"Right bytes",L"Newness / details",L"Left modified (local)",L"Right modified (local)",L"Text rules",L"Text analysis error"}) rows_.InsertColumn(rows_.GetHeaderCtrl()->GetItemCount(), label, LVCFMT_LEFT, 120);
    else if (mode >= 5) for (auto label : {L"Field / path",L"Left value",L"Right value",L"State"}) rows_.InsertColumn(rows_.GetHeaderCtrl()->GetItemCount(),label,LVCFMT_LEFT,180);
    else for (auto label : {L"Base #",L"Base",L"Left #",L"Left",L"Right #",L"Right",L"State",L"Review note"}) rows_.InsertColumn(rows_.GetHeaderCtrl()->GetItemCount(), label, LVCFMT_LEFT, 100);
    output_.SetReadOnly(Busy() || !model_); Layout(); Columns(); RebuildRows();
}
void CAdvancedCompareView::Layout() {
    if (!rows_.GetSafeHwnd() || !status_.GetSafeHwnd()) return;
    CRect r; GetClientRect(r); const int width = (std::max)(0, r.Width()); int x = 0, y = 0;
    LayoutPastedInputs(width,r.Height(),&y);
    const auto place = [&](CWnd& control, int minimum, bool visible, bool dropdown = false) {
        control.ShowWindow(visible ? SW_SHOW : SW_HIDE); if (!visible) return;
        CString label; control.GetWindowTextW(label); CClientDC dc(&control); auto* old = dc.SelectObject(&ui_font_);
        const int preferred = (std::max)(minimum, static_cast<int>(dc.GetTextExtent(label).cx) + 28); dc.SelectObject(old);
        if (x && x + preferred > width) { x = 0; y += 27; }
        control.MoveWindow(x, y, (std::min)(preferred, width), dropdown ? 180 : 24); x += preferred + 5;
    };
    const bool folder = mode_ == 4,structured = mode_ >= 5 || manual_view_;
    for (CButton* c : {&ignore_space_,&ignore_case_,&ignore_eol_,&changes_only_,&conflicts_only_,&syntax_}) place(*c, 65, !folder && !structured);
    place(similar_,150,!folder && mode_ == 2);
    for (CButton* c : {&recursive_,&show_equal_,&mirror_}) place(*c, 80, folder);
    if (folder) { x = 0; y += 27; }
    place(folder_criterion_,190,folder,true); place(folder_filter_,190,folder,true); place(folder_tolerance_,210,folder,true);
    x = 0; y += 27;
    for (CButton* c : {&take_left_,&take_right_,&take_base_,&take_both_,&undo_,&redo_,&save_}) place(*c, 55, !folder && !structured && (c != &take_base_ || mode_ == 3));
    for (CButton* c : {&copy_right_,&copy_left_,&sync_right_,&sync_left_}) place(*c, 80, folder);
    if (folder) { x = 0; y += 27; }
    place(sync_policy_,230,folder,true); place(baseline_save_,125,folder); place(baseline_load_,125,folder); place(propagate_deletions_,210,folder);
    if (folder) { x = 0; y += 27; }
    place(journal_retry_,130,folder); place(journal_recover_,140,folder); place(backup_browse_,135,folder);
    x = 0; y += 27; if (structured) y = 0; place(encoding_label_, 75, !folder && !pasted_inputs_); place(encoding_, 110, !folder && !pasted_inputs_, true);
    place(encoding_right_label_,75,!folder && !pasted_inputs_); place(encoding_right_,110,!folder && !pasted_inputs_,true); place(encoding_base_label_,75,mode_ == 3); place(encoding_base_,110,mode_ == 3,true);
    place(allow_markers_,150,!folder && !structured); place(manual_resolved_button_,170,!folder && !structured);
    codec_details_.ShowWindow(!folder && !pasted_inputs_); if (!folder && !pasted_inputs_) { x = 0; y += 27; codec_details_.MoveWindow(0,y,width,24); }
    include_label_.ShowWindow(folder); exclude_label_.ShowWindow(folder); include_.ShowWindow(folder); exclude_.ShowWindow(folder);
    if (folder) { const int half = width / 2; include_label_.MoveWindow(0, y, 55, 22); include_.MoveWindow(58, y, (std::max)(0, half - 65), 24); exclude_label_.MoveWindow(half, y, 60, 22); exclude_.MoveWindow(half + 63, y, (std::max)(0, width - half - 63), 24); }
    int top = y + 29; const bool sources = model_ && !stale_ && mode_ < 4 && !pasted_inputs_ && r.Height() - top > 180; const int source_height = sources ? (std::max)(60,(r.Height() - top - 80) / 3) : 0;
    for (CWnd* c : {static_cast<CWnd*>(&source_left_),static_cast<CWnd*>(&source_right_),static_cast<CWnd*>(&source_left_label_),static_cast<CWnd*>(&source_right_label_)}) c->ShowWindow(sources ? SW_SHOW : SW_HIDE);
    const bool base_source = sources && mode_ == 3 && show_base_source_;
    source_base_.ShowWindow(base_source); source_base_label_.ShowWindow(base_source);
    if (sources) { const int columns = base_source ? 3 : 2,part = width / columns; const auto place_source = [&](CWnd& editor,CWnd& label,int column) { const int left = part * column; const int size = column == columns - 1 ? width - left : part - 5; label.MoveWindow(left,top,(std::max)(0,size),20); editor.MoveWindow(left,top + 20,(std::max)(0,size),source_height); }; place_source(source_left_,source_left_label_,0); if (base_source) place_source(source_base_,source_base_label_,1); place_source(source_right_,source_right_label_,columns - 1); top += source_height + 25; }
    const int bottom = (std::max)(top, r.Height() - 42), available = bottom - top;
    const int list_height = folder || structured ? available : (std::max)(0, (available - 22) / 2);
    rows_.MoveWindow(0, top, (std::max)(0,width - 22), list_height); overview_.MoveWindow((std::max)(0,width - 20),top,(std::min)(20,width),list_height); output_label_.ShowWindow(!folder && !structured); output_.ShowWindow(!folder && !structured);
    output_label_.MoveWindow(0, top + list_height, width, (std::min)(22, available));
    output_.MoveWindow(0, top + list_height + 22, width, (std::max)(0, available - list_height - 22));
    status_.MoveWindow(0, bottom, width, (std::max)(0, r.Height() - bottom)); Columns();
}
void CAdvancedCompareView::Columns() {
    if (!rows_.GetSafeHwnd()) return; CRect r; rows_.GetClientRect(r);
    CClientDC dc(&rows_); auto* old = dc.SelectObject(&content_font_);
    const int number_width = (std::max)({58,static_cast<int>(dc.GetTextExtent(L"200000").cx) + 14,static_cast<int>(dc.GetTextExtent(L"Right #").cx) + 20});
    const int state_width = (std::max)(115,static_cast<int>(dc.GetTextExtent(mode_ == 4 ? L"Different [not rescanned]" : L"Moved L200000 -> R200000").cx) + 20);
    const int bytes_width = (std::max)(110,static_cast<int>(dc.GetTextExtent(L"18446744073709551615").cx) + 16); dc.SelectObject(old);
    if (mode_ == 4) { const int widths[] = {MulDiv(260,zoom_percent_,100),state_width,bytes_width,bytes_width,MulDiv(280,zoom_percent_,100),MulDiv(220,zoom_percent_,100),MulDiv(220,zoom_percent_,100),MulDiv(260,zoom_percent_,100),MulDiv(400,zoom_percent_,100)}; for (int i = 0; i < 9; ++i) rows_.SetColumnWidth(i, widths[i]); int order[] = {0,1,7,2,3,4,5,6,8}; rows_.SetColumnOrderArray(9,order); if (folder_text_rules_.empty()) { rows_.SetColumnWidth(7,0); rows_.SetColumnWidth(8,0); } }
    else if (mode_ >= 5) { const int widths[] = {300,400,400,140}; for (int i = 0; i < 4; ++i) rows_.SetColumnWidth(i,MulDiv(widths[i],zoom_percent_,100)); }
    else {
        const int count = mode_ == 3 ? 3 : 2;
        const int text_width = (std::max)(MulDiv(160,zoom_percent_,100),(r.Width() - number_width * count - state_width) / count);
        for (int i = 0; i < 6; ++i) rows_.SetColumnWidth(i, i < 2 && mode_ != 3 ? 0 : i % 2 ? text_width : number_width);
        rows_.SetColumnWidth(6,state_width);
        rows_.SetColumnWidth(7,MulDiv(240,zoom_percent_,100));
    }
}
void CAdvancedCompareView::Status(const std::wstring& text) { if (status_.GetSafeHwnd()) status_.SetWindowTextW(text.c_str()); if (report) report(text); }
void CAdvancedCompareView::SetBusy(bool busy) {
    if (busy_changed) busy_changed(busy);
    output_.SetReadOnly(busy || !model_ || manual_view_);
    if (input_left_.GetSafeHwnd()) input_left_.SetReadOnly(busy || read_only_snapshots_);
    if (input_right_.GetSafeHwnd()) input_right_.SetReadOnly(busy || read_only_snapshots_);
    for (CWnd* control : {static_cast<CWnd*>(&ignore_space_),static_cast<CWnd*>(&ignore_case_),static_cast<CWnd*>(&ignore_eol_),static_cast<CWnd*>(&encoding_),static_cast<CWnd*>(&encoding_right_),static_cast<CWnd*>(&encoding_base_),static_cast<CWnd*>(&take_left_),static_cast<CWnd*>(&take_right_),static_cast<CWnd*>(&take_base_),static_cast<CWnd*>(&take_both_),static_cast<CWnd*>(&undo_),static_cast<CWnd*>(&redo_),static_cast<CWnd*>(&save_),static_cast<CWnd*>(&copy_right_),static_cast<CWnd*>(&copy_left_),static_cast<CWnd*>(&sync_right_),static_cast<CWnd*>(&sync_left_),static_cast<CWnd*>(&mirror_),static_cast<CWnd*>(&recursive_),static_cast<CWnd*>(&include_),static_cast<CWnd*>(&exclude_)}) control->EnableWindow(!busy);
    allow_markers_.EnableWindow(!busy); manual_resolved_button_.EnableWindow(!busy);
    similar_.EnableWindow(!busy);
    folder_criterion_.EnableWindow(!busy); folder_tolerance_.EnableWindow(!busy); folder_filter_.EnableWindow(!busy);
    sync_policy_.EnableWindow(!busy); baseline_save_.EnableWindow(!busy); baseline_load_.EnableWindow(!busy); propagate_deletions_.EnableWindow(!busy);
    journal_retry_.EnableWindow(!busy); journal_recover_.EnableWindow(!busy); backup_browse_.EnableWindow(!busy);
}
void CAdvancedCompareView::InvalidateInputs() { if (codec_details_.GetSafeHwnd()) codec_details_.SetWindowTextW(L""); stale_ = true; rows_.SetItemCountEx(0); overview_.SetBands({}); RefreshSources(); Layout(); Status(L"Inputs changed. Press Compare. Existing merge output is retained."); }
void CAdvancedCompareView::StartText(std::unique_ptr<BinaryDocument> base, std::unique_ptr<BinaryDocument> left, std::unique_ptr<BinaryDocument> right) {
    source_left_path_ = left ? left->Path() : L""; source_right_path_ = right ? right->Path() : L"";
    source_base_path_ = base ? base->Path() : L"";
    StartTextJob(std::move(base),std::move(left),std::move(right),L"",L"",false);
}
void CAdvancedCompareView::StartTextJob(std::unique_ptr<BinaryDocument> base, std::unique_ptr<BinaryDocument> left, std::unique_ptr<BinaryDocument> right, std::wstring left_text, std::wstring right_text, bool pasted) {
    if (Busy() || !CanReset()) return;
    if (mode_ >= 5) { StartStructured(std::move(left),std::move(right)); return; }
    const int mode = mode_; const auto encoding = static_cast<TextEncoding>((std::max)(0, encoding_.GetCurSel()));
    const auto right_encoding = static_cast<TextEncoding>((std::max)(0,encoding_right_.GetCurSel())),base_encoding = static_cast<TextEncoding>((std::max)(0,encoding_base_.GetCurSel()));
    LineOptions options{ignore_space_.GetCheck() == BST_CHECKED,ignore_case_.GetCheck() == BST_CHECKED,ignore_eol_.GetCheck() == BST_CHECKED}; std::wstring rule_error; if (mode == 2 && !ParseTextRules(text_rules_,&options.rules,&rule_error)) { Status(rule_error); return; } const bool similar = similar_.GetCheck() != 0;
    LineAnchors anchors; if (mode == 2 && !ParseLineAnchors(manual_anchors_,&anchors,&rule_error)) { Status(rule_error); return; }
    cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = JobKind::Text; SetBusy(true); Status(L"Comparing text lines...");
    try { job_ = std::async(std::launch::async, [this, mode, encoding,right_encoding,base_encoding, options, similar,anchors,pasted, protection = snapshot_protection_, snapshot_codec = snapshot_codec_, a = std::move(left_text), b = std::move(right_text), base = std::move(base), left = std::move(left), right = std::move(right)]() mutable {
        Result result; result.kind = JobKind::Text; result.model = std::make_shared<TextMergeModel>(); if (pasted) { result.inputs = std::move(protection); result.codec = snapshot_codec; } std::wstring c; TextCodec other;
        for (const auto* input : {left.get(),right.get(),base.get()}) if (input) result.inputs.push_back({input->Path(),input->SourceIdentity()});
        if (!pasted) {
            TextCodec base_codec;
            if (!DecodeDocument(left.get(),encoding,&a,&result.codec,&result.error) || !DecodeDocument(right.get(),right_encoding,&b,&other,&result.error) || (mode == 3 && !DecodeDocument(base.get(),base_encoding,&c,&base_codec,&result.error))) return result;
            result.codec_details = L"Left: " + DescribeTextCodec(result.codec,a) + L" | Right: " + DescribeTextCodec(other,b);
            if (mode == 3) result.codec_details += L" | Base: " + DescribeTextCodec(base_codec,c);
        }
        result.anchored = !anchors.empty(); const bool built = anchors.empty() ? BuildTextMerge(c,a,b,mode == 3,options,cancelled_,result.model.get(),&result.error) : BuildAnchoredComparison(a,b,options,anchors,cancelled_,result.model.get(),&result.error);
        if (!built && result.error.empty() && !cancelled_) result.error = L"Text comparison could not complete. Check the 200,000-line limit and alignment anchors.";
        if (built) { result.text = RenderMerge(*result.model); result.options = options; result.similar = similar; result.presentation = std::make_shared<DiffPresentation>(); if (!BuildDiffPresentation(*result.model,options,similar,cancelled_,result.presentation.get())) result.error = L"Text presentation cancelled."; }
        if (built && mode == 2) { bool truncated = false; if (!FindMovedBlocks(*result.model,cancelled_,&result.moves,&truncated)) result.error = L"Moved-block analysis cancelled."; }
        if (built && !FingerprintComparison(*result.model,cancelled_,&result.review_fingerprint,&result.error) && result.error.empty()) result.error = L"Could not fingerprint comparison sources for review notes.";
        return result;
    }); } catch (const std::exception&) { SetBusy(false); Status(L"Could not start text comparison. The inputs are retained."); }
}
void CAdvancedCompareView::StartFolders(const std::wstring& left, const std::wstring& right) {
    if (Busy() || !CanReset()) return;
    folder_left_ = left; folder_right_ = right; CString include, exclude; include_.GetWindowTextW(include); exclude_.GetWindowTextW(exclude);
    FolderOptions options{recursive_.GetCheck() == BST_CHECKED,include.IsEmpty() ? L"*" : include.GetString(),exclude.GetString()};
    options.criterion = static_cast<FolderCriterion>((std::max)(0,folder_criterion_.GetCurSel())); options.time_tolerance_seconds = static_cast<unsigned>((std::max)(0,folder_tolerance_.GetCurSel()));
    options.text_rules = folder_text_rules_;
    cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = JobKind::Scan; SetBusy(true); Status(L"Comparing folders and file contents...");
    job_ = std::async(std::launch::async, [this,left,right,options]() { Result result; result.kind = JobKind::Scan; result.folder_options = options; result.scan = std::make_shared<FolderScan>(); if (!ScanFolders(left,right,options,cancelled_,[this](unsigned p) { progress_ = p; },result.scan.get(),&result.error) && result.error.empty()) result.error = L"Folder comparison failed or was cancelled."; return result; });
}
void CAdvancedCompareView::AdoptText(std::wstring text, bool initial) {
    output_.SetReadOnly(FALSE);
    generated_ = RenderMerge(*model_,&model_unresolved_); display_generated_ = TextForEditControl(generated_); const auto displayed = TextForEditControl(text);
    suppress_result_ = true;
    if (initial) { choice_actions_.clear(); extra_conflicts_ = 0; saved_text_ = mode_ == 3 ? std::wstring() : displayed; output_.SetWindowTextW(displayed.c_str()); output_.SetModify(FALSE); }
    else { ChoiceAction action; action.before_extra = extra_conflicts_; for (std::size_t i = 0; i < render_before_.size(); ++i) if (render_before_[i] != model_->blocks[i].choice) action.delta.emplace_back(i,render_before_[i],model_->blocks[i].choice); action.after_extra = extra_conflicts_; choice_actions_.push_back(std::move(action)); output_.ReplaceGenerated(displayed,static_cast<int>(choice_actions_.size())); }
    suppress_result_ = false; manual_resolved_ = false; RefreshOutputState();
    Status(L"Text comparison complete. " + std::to_wstring(unresolved_) + L" unresolved conflict(s). Select changed rows and Take a side, or edit the output. Save result keeps source tabs intact.");
    if (changed) changed();
    if (manual_view_) { output_.SetReadOnly(TRUE); Status(L"Read-only manually aligned comparison. Clear manual anchors and compare again to enable merging. Existing source files remain unchanged."); }
    else if (!text_rules_.empty()) { const auto ignored = std::count_if(model_->blocks.begin(),model_->blocks.end(),[](const auto& block) { return block.changed && !block.important; }); Status(L"Advanced text rules active: " + std::to_wstring(ignored) + L" ignored difference block(s). Original source/output text is retained."); }
}
void CAdvancedCompareView::Tick() {
    if (overview_.GetSafeHwnd() && rows_.GetSafeHwnd()) overview_.SetViewport(static_cast<std::size_t>((std::max)(0,rows_.GetTopIndex())),static_cast<std::size_t>((std::max)(0,rows_.GetCountPerPage())));
    if (!job_.valid()) return;
    if (job_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        if (progress_.load() != shown_progress_) { shown_progress_ = progress_.load(); Status(L"Working... " + std::to_wstring(shown_progress_) + L"%. Esc cancels before the current file commits."); }
        return;
    }
    Result result;
    try { result = job_.get(); } catch (const std::exception&) { result.error = L"Operation failed: insufficient memory or I/O error."; }
    SetBusy(false); ReleaseLocks();
    if (result.kind == JobKind::Sync) {
        if (!result.output_path.empty()) last_journal_ = result.output_path;
        sync_summary_ = std::to_wstring(result.sync.completed) + L" operation(s) committed. Backup: " + (result.text.empty() ? result.sync.backup.wstring() : result.text);
        if (!result.sync.error.empty()) sync_summary_ += L". " + result.sync.error;
        if (result.sync.cancelled) sync_summary_ += L". Cancelled; committed files remain committed.";
        Status(sync_summary_); StartFolders(folder_left_,folder_right_); return;
    }
    if (!result.error.empty() || (cancelled_ && !((result.kind == JobKind::Save || result.kind == JobKind::NotesSave || result.kind == JobKind::Report) && result.saved))) {
        if (job_kind_ == JobKind::Presentation) similar_.SetCheck(presentation_similar_ ? BST_CHECKED : BST_UNCHECKED);
        if (job_kind_ == JobKind::Render && model_ && render_before_.size() == model_->blocks.size()) {
            for (std::size_t i = 0; i < render_before_.size(); ++i) model_->blocks[i].choice = render_before_[i]; render_before_.clear(); RebuildRows();
        }
        Status(cancelled_ ? L"Operation cancelled; existing output retained." : result.error);
        if (close_after_save_) { close_after_save_ = false; if (saved_close) saved_close(false); } return;
    }
    if (result.kind == JobKind::Pairing) { if (result.pairs) FinishContentPairing(*result.pairs); return; }
    if (result.kind == JobKind::NotesLoad || result.kind == JobKind::NotesSave) { FinishNotesIo(result); return; }
    if (result.kind == JobKind::Refresh) {
        ++folder_generation_;
        scan_ = std::move(result.scan); tree_expanded_.clear();
        for (std::size_t i = 0; i < scan_->rows.size(); ++i) if (refresh_expanded_.count(scan_->rows[i].relative.wstring())) tree_expanded_.insert(i);
        RebuildRows(); for (std::size_t i = 0; i < visible_.size(); ++i) { const auto& path = scan_->rows[visible_[i]].relative.wstring(); if (refresh_selected_.count(path)) rows_.SetItemState(static_cast<int>(i),LVIS_SELECTED,LVIS_SELECTED); if (path == refresh_top_) rows_.EnsureVisible(static_cast<int>(i),FALSE); }
        Status(L"Selected paths refreshed. Rows marked [not rescanned] retain older results. Full Compare is required before Sync all or saving a baseline."); if (changed) changed(); return;
    }
    if (result.kind == JobKind::Text || result.kind == JobKind::Structured) codec_details_.SetWindowTextW(result.codec_details.c_str());
    if (result.kind == JobKind::Text || result.kind == JobKind::Scan || result.kind == JobKind::Structured) stale_ = false;
    if (result.kind == JobKind::Structured) { structured_ = std::move(result.structured); model_.reset(); scan_.reset(); dirty_ = manual_ = false; RebuildRows(); Status(L"Read-only field comparison: " + std::to_wstring(structured_->different) + L" difference(s), " + std::to_wstring(structured_->compared) + L" compared nodes/cells." + (structured_->truncated ? L" First 20,000 differences shown." : L"") + L" Value/path previews: 512/2048 characters. Original inputs remain unchanged."); return; }
    if (result.kind == JobKind::Text) { moves_ = std::move(result.moves); manual_view_ = result.anchored; model_ = std::move(result.model); review_fingerprint_ = result.review_fingerprint; if (notes_.notes.empty()) notes_.fingerprint = review_fingerprint_; context_expanded_.clear(); codec_ = result.codec; report_inputs_ = std::move(result.inputs); presentation_ = std::move(result.presentation); line_options_ = result.options; presentation_similar_ = result.similar; if (presentation_) text_rows_ = std::move(presentation_->rows); scan_.reset(); saved_path_.clear(); AdoptText(std::move(result.text),true); RebuildRows(); RefreshSources(); Layout(); }
    else if (result.kind == JobKind::Presentation) { presentation_ = std::move(result.presentation); presentation_similar_ = result.similar; if (presentation_) text_rows_ = std::move(presentation_->rows); RebuildRows(); RefreshSources(); Status(L"Source alignment updated. Original line numbers and merged-output edits are retained."); }
    else if (result.kind == JobKind::Render) { const auto before_extra = extra_conflicts_; extra_conflicts_ = result.extra_conflicts; AdoptText(std::move(result.text),false); if (!choice_actions_.empty()) choice_actions_.back().before_extra = before_extra; render_before_.clear(); RebuildRows(); }
    else if (result.kind == JobKind::Scan) { ++folder_generation_; scan_options_ = result.folder_options; scan_ = std::move(result.scan); tree_expanded_.clear(); model_.reset(); Columns(); RebuildRows(); Status(sync_summary_ + L" Folder comparison complete: " + std::to_wstring(scan_->rows.size()) + L" entries. Sync uses saved disk contents. Preview every operation before applying."); }
    else if (result.kind == JobKind::Plan) { ApplyBundle(std::move(result.bundle)); }
    else if (result.kind == JobKind::Report) { if (result.saved && mode_ == 4 && !result.output_path.empty()) baseline_path_ = result.output_path; Status(result.text); }
    else if (result.kind == JobKind::Save) {
        if (result.saved) { saved_path_ = result.output_path; saved_stamp_ = result.written; CString value; output_.GetWindowTextW(value); saved_text_ = value.GetString(); dirty_ = false; output_.SetModify(FALSE); Status(L"Merge result saved: " + saved_path_); if (changed) changed(); }
        if (close_after_save_) { close_after_save_ = false; if (saved_close) saved_close(result.saved); }
    }
}
void CAdvancedCompareView::RebuildRows() {
    visible_.clear(); context_rows_ = {}; eligible_folder_rows_.clear();
    if (stale_) { rows_.SetItemCountEx(0); overview_.SetBands({}); return; }
    if (mode_ == 4 && scan_) { for (std::size_t i = 0; i < scan_->rows.size(); ++i) if (FolderRowVisible(scan_->rows[i])) visible_.push_back(i); OrderFolderRows(); eligible_folder_rows_ = visible_; if (folder_tree_) { tree_rows_ = BuildFolderTreeRows(*scan_,eligible_folder_rows_,tree_expanded_); visible_ = tree_rows_.rows; } }
    else if (mode_ >= 5 && structured_) for (std::size_t i = 0; i < structured_->rows.size(); ++i) visible_.push_back(i);
    else if (model_) {
        if (!presentation_) text_rows_ = MergeRows(*model_);
        std::vector<unsigned char> significant;
        for (std::size_t i = 0; i < text_rows_.size(); ++i) {
            const auto& b = model_->blocks[text_rows_[i].block];
            if (context_lines_) { significant.push_back(conflicts_only_.GetCheck() ? !manual_resolved_ && b.conflict && b.choice == MergeChoice::Unresolved : b.changed && b.important); continue; }
            if (changes_only_.GetCheck() && (!b.changed || !b.important)) continue;
            if (conflicts_only_.GetCheck() && (manual_resolved_ || !b.conflict || b.choice != MergeChoice::Unresolved)) continue;
            visible_.push_back(i);
        }
        if (context_lines_) { context_rows_ = BuildContextRows(significant,context_lines_,context_expanded_); visible_ = context_rows_.rows; }
    }
    if (mode_ >= 4) rows_.SetItemState(-1,0,LVIS_SELECTED | LVIS_FOCUSED);
    rows_.SetItemCountEx(static_cast<int>(visible_.size())); rows_.Invalidate();
    std::vector<unsigned char> bands; bands.reserve(visible_.size()); for (auto index : visible_) { unsigned char kind = mode_ >= 5 && structured_ ? 1 : 0; if (mode_ == 4 && scan_) { const auto state = scan_->rows[index].state; kind = state == FolderState::Equal ? 0 : state == FolderState::Error || state == FolderState::TypeConflict ? 2 : 1; } else if (model_) { const auto& block = model_->blocks[text_rows_[index].block]; kind = block.conflict && block.choice == MergeChoice::Unresolved && !manual_resolved_ ? 2 : block.changed ? 1 : 0; if (kind == 1 && presentation_ && index < presentation_->hints.size() && !presentation_->hints[index].empty()) kind = 3; } bands.push_back(kind); } overview_.SetBands(std::move(bands));
}
std::wstring CAdvancedCompareView::Cell(int row, int column) const {
    if (row < 0 || static_cast<std::size_t>(row) >= visible_.size()) return L"";
    if (mode_ >= 5 && structured_) { const auto& item = structured_->rows[visible_[row]]; return column == 0 ? item.path : column == 1 ? item.left : column == 2 ? item.right : column == 3 ? simple_editor::ui::Localize(item.state) : L""; }
    if (mode_ == 4 && scan_) {
        const auto& item = scan_->rows[visible_[row]]; const auto state_text = [&](const std::wstring& text) { return simple_editor::ui::Localize(text) + (item.fresh ? L"" : simple_editor::ui::Localize(L" [not rescanned]")); };
        if (column == 0) { if (!folder_tree_) return item.relative.wstring(); const auto index = visible_[row]; const auto prefix = item.left.directory || item.right.directory ? tree_expanded_.count(index) ? L"[-] " : L"[+] " : L"    "; return std::wstring((std::min)(16u,tree_rows_.depth[index]) * 2,L' ') + prefix + item.relative.filename().wstring(); } if (column == 1) { if (folder_tree_ && (item.left.directory || item.right.directory) && item.state == FolderState::Equal) { const auto index = visible_[row]; return state_text(tree_rows_.errors[index] ? L"Subtree errors" : tree_rows_.differences[index] ? L"Subtree different" : L"Subtree equal"); } return state_text(State(item.state)); }
        if (column == 7) return simple_editor::ui::Localize(item.text_state); if (column == 8) return simple_editor::ui::Localize(item.text_error);
        if (column == 4 && folder_tree_ && (item.left.directory || item.right.directory)) { const auto index = visible_[row]; return std::to_wstring(tree_rows_.differences[index]) + L" changed files, " + std::to_wstring(tree_rows_.errors[index]) + L" errors in scanned subtree"; }
        if (column == 2) return !item.left.exists ? L"-" : item.left.directory ? L"<DIR>" : std::to_wstring(item.left.size);
        if (column == 3) return !item.right.exists ? L"-" : item.right.directory ? L"<DIR>" : std::to_wstring(item.right.size);
        if (column == 4) return !item.error.empty() ? item.error : !item.left.exists || !item.right.exists || item.left.directory || item.right.directory ? L"" : item.newness == FolderNewness::LeftNewer ? L"Left newer" : item.newness == FolderNewness::RightNewer ? L"Right newer" : item.state == FolderState::Different ? L"Same time; different data" : L"Same time";
        const auto& stamp = column == 5 ? item.left : item.right; if (!stamp.exists) return L"-";
        FILETIME utc{static_cast<DWORD>(stamp.modified),static_cast<DWORD>(stamp.modified >> 32)},local{}; SYSTEMTIME time{}; if (!FileTimeToLocalFileTime(&utc,&local) || !FileTimeToSystemTime(&local,&time)) return L"?";
        wchar_t value[48]{}; swprintf_s(value,L"%04u-%02u-%02u %02u:%02u:%02u",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond); return value;
    }
    if (!model_) return L"";
    const auto hidden = context_rows_.hidden.find(visible_[row]); if (hidden != context_rows_.hidden.end()) return column == 3 ? L"[+] " + std::to_wstring(hidden->second) + L" hidden rows - double-click to expand" : L"";
    const auto& item = text_rows_[visible_[row]]; const auto& b = model_->blocks[item.block];
    if (column == 7) return NoteCell(text_rows_[visible_[row]].block);
    if (column == 6) { if (presentation_ && visible_[row] < presentation_->hints.size() && !presentation_->hints[visible_[row]].empty()) return simple_editor::ui::Localize(presentation_->hints[visible_[row]]); return simple_editor::ui::Localize(!b.changed ? L"Equal" : b.conflict && b.choice == MergeChoice::Unresolved && !manual_resolved_ ? L"CONFLICT" : !b.important ? L"Ignored" : b.conflict ? L"Resolved" : L"Changed"); }
    const int index = column < 2 ? item.base : column < 4 ? item.left : item.right;
    if (index < 0) return L""; if (!(column % 2)) return std::to_wstring(index + 1);
    return (column < 2 ? model_->base : column < 4 ? model_->left : model_->right)[index].text.substr(0,8192);
}
void CAdvancedCompareView::OnDisplay(NMHDR* header, LRESULT* result) {
    *result = 0; auto* info = reinterpret_cast<NMLVDISPINFO*>(header);
    if (info->item.mask & LVIF_TEXT) wcsncpy_s(info->item.pszText, info->item.cchTextMax, Cell(info->item.iItem,info->item.iSubItem).c_str(),_TRUNCATE);
}
void CAdvancedCompareView::OnResultChanged() {
    if (suppress_result_) return;
    if (manual_resolved_) { manual_resolved_ = false; unresolved_ = model_unresolved_; RebuildRows(); }
    RefreshOutputState();
}
void CAdvancedCompareView::OnFilterChanged() { if (!Busy()) RebuildRows(); rows_.Invalidate(); }
void CAdvancedCompareView::OnRulesChanged() { if (!Busy()) InvalidateInputs(); }
void CAdvancedCompareView::OnSimilarChanged() {
    if (Busy() || !model_ || stale_ || mode_ != 2) return; const auto model = model_; const auto options = line_options_; const bool similar = similar_.GetCheck() != 0; cancelled_ = false; job_kind_ = JobKind::Presentation; SetBusy(true);
    job_ = std::async(std::launch::async,[this,model,options,similar]() { Result result; result.kind = JobKind::Presentation; result.similar = similar; result.presentation = std::make_shared<DiffPresentation>(); if (!BuildDiffPresentation(*model,options,similar,cancelled_,result.presentation.get())) result.error = L"Alignment cancelled."; return result; });
}
void CAdvancedCompareView::SetEncoding(TextEncoding encoding) { if (!Busy()) { for (auto* combo : {&encoding_,&encoding_right_,&encoding_base_}) combo->SetCurSel(static_cast<int>(encoding)); InvalidateInputs(); } }
void CAdvancedCompareView::CapturePreferences(WorkspaceEntry& entry) const {
    entry.json_rules = json_rules_;
    entry.folder_text_rules = folder_text_rules_; entry.context_lines = context_lines_; entry.folder_tree = folder_tree_; entry.table_rules = table_rules_; entry.text_rules = text_rules_; entry.manual_anchors = manual_anchors_;
    if (!GetSafeHwnd()) { if (has_pending_preferences_) { const auto& p = pending_preferences_; entry.advanced_encoding = p.advanced_encoding; entry.advanced_encoding_right = p.advanced_encoding_right; entry.advanced_encoding_base = p.advanced_encoding_base; entry.options = p.options; entry.include = p.include; entry.exclude = p.exclude; entry.folder_criterion = p.folder_criterion; entry.folder_tolerance = p.folder_tolerance; entry.folder_filter = p.folder_filter; entry.folder_sort = p.folder_sort; entry.folder_sort_descending = p.folder_sort_descending; entry.sync_policy = p.sync_policy; entry.sync_deletions = p.sync_deletions; entry.sync_baseline = p.sync_baseline; } return; }
    entry.folder_criterion = static_cast<unsigned>((std::max)(0,folder_criterion_.GetCurSel())); entry.folder_filter = static_cast<unsigned>((std::max)(0,folder_filter_.GetCurSel())); entry.folder_tolerance = static_cast<unsigned>((std::max)(0,folder_tolerance_.GetCurSel())); entry.folder_sort = folder_sort_; entry.folder_sort_descending = folder_sort_descending_;
    entry.sync_policy = static_cast<unsigned>((std::max)(0,sync_policy_.GetCurSel())); entry.sync_deletions = propagate_deletions_.GetCheck() != 0; entry.sync_baseline = baseline_path_;
    entry.advanced_encoding = encoding_.GetCurSel(); entry.advanced_encoding_right = encoding_right_.GetCurSel(); entry.advanced_encoding_base = encoding_base_.GetCurSel(); entry.options = 0;
    const CButton* controls[] = {&ignore_space_,&ignore_case_,&ignore_eol_,&recursive_,&show_equal_,&mirror_,&syntax_,&changes_only_,&conflicts_only_,&similar_};
    for (unsigned i = 0; i < 10; ++i) if (controls[i]->GetCheck()) entry.options |= 1u << i;
    CString include,exclude; include_.GetWindowTextW(include); exclude_.GetWindowTextW(exclude); entry.include = include.GetString(); entry.exclude = exclude.GetString();
}
void CAdvancedCompareView::RestorePreferences(const WorkspaceEntry& entry) {
    json_rules_ = entry.json_rules;
    folder_text_rules_ = entry.folder_text_rules; context_lines_ = entry.context_lines; folder_tree_ = entry.folder_tree; table_rules_ = entry.table_rules; text_rules_ = entry.text_rules; manual_anchors_ = entry.manual_anchors;
    if (!GetSafeHwnd()) { pending_preferences_ = entry; has_pending_preferences_ = true; return; }
    has_pending_preferences_ = false;
    folder_criterion_.SetCurSel(entry.folder_criterion); folder_filter_.SetCurSel(entry.folder_filter); folder_tolerance_.SetCurSel((std::min)(2u,entry.folder_tolerance)); folder_sort_ = entry.folder_sort; folder_sort_descending_ = entry.folder_sort_descending;
    sync_policy_.SetCurSel(entry.sync_policy); propagate_deletions_.SetCheck(entry.sync_deletions); baseline_path_ = entry.sync_baseline;
    encoding_.SetCurSel(entry.advanced_encoding); encoding_right_.SetCurSel(entry.advanced_encoding_right < 0 ? entry.advanced_encoding : entry.advanced_encoding_right); encoding_base_.SetCurSel(entry.advanced_encoding_base < 0 ? entry.advanced_encoding : entry.advanced_encoding_base); CButton* controls[] = {&ignore_space_,&ignore_case_,&ignore_eol_,&recursive_,&show_equal_,&mirror_,&syntax_,&changes_only_,&conflicts_only_,&similar_};
    for (unsigned i = 0; i < 10; ++i) controls[i]->SetCheck(entry.options & (1u << i) ? BST_CHECKED : BST_UNCHECKED);
    include_.SetWindowTextW(entry.include.c_str()); exclude_.SetWindowTextW(entry.exclude.c_str());
}
void CAdvancedCompareView::SetZoom(int percent) {
    if (!GetSafeHwnd()) return; CFont replacement; if (!replacement.CreatePointFont(MulDiv(font_tenths_,percent,100),font_face_.c_str())) return;
    CFont previous; previous.Attach(content_font_.Detach()); content_font_.Attach(replacement.Detach()); rows_.SetFont(&content_font_); output_.SetFont(&content_font_); if (input_left_.GetSafeHwnd()) input_left_.SetFont(&content_font_); if (input_right_.GetSafeHwnd()) input_right_.SetFont(&content_font_); if (source_left_.GetSafeHwnd()) source_left_.SetFont(&content_font_); if (source_right_.GetSafeHwnd()) source_right_.SetFont(&content_font_); if (source_base_.GetSafeHwnd()) source_base_.SetFont(&content_font_); zoom_percent_ = percent; Columns(); rows_.Invalidate();
}
void CAdvancedCompareView::SetAppearance(const std::wstring& face,unsigned font_tenths,unsigned theme) {
    font_face_ = face; font_tenths_ = font_tenths; theme_ = (std::min)(2u,theme); if (!GetSafeHwnd()) return; SetZoom(zoom_percent_); output_.SetTheme(theme_);
    if (input_left_.GetSafeHwnd()) input_left_.SetTheme(theme_);
    if (input_right_.GetSafeHwnd()) input_right_.SetTheme(theme_);
    if (source_left_.GetSafeHwnd()) source_left_.SetTheme(theme_); if (source_right_.GetSafeHwnd()) source_right_.SetTheme(theme_);
    if (source_base_.GetSafeHwnd()) source_base_.SetTheme(theme_);
    const auto back = theme_ == 2 ? GetSysColor(COLOR_WINDOW) : theme_ == 1 ? RGB(63,63,63) : RGB(255,255,255),fore = theme_ == 2 ? GetSysColor(COLOR_WINDOWTEXT) : theme_ == 1 ? RGB(225,228,235) : RGB(30,35,42); rows_.SetBkColor(back); rows_.SetTextBkColor(back); rows_.SetTextColor(fore); rows_.Invalidate();
}
void CAdvancedCompareView::SetTextViewPreferences(bool wrap,bool whitespace,unsigned tab_width,bool use_tabs) {
    wrap_text_ = wrap; whitespace_ = whitespace; tab_width_ = tab_width; use_tabs_ = use_tabs; if (output_.GetSafeHwnd()) { output_.SetWrap(wrap); output_.SetWhitespace(whitespace); output_.SetTabWidth(tab_width,use_tabs); }
    for (auto* input : {&input_left_,&input_right_}) if (input->GetSafeHwnd()) { input->SetWrap(wrap); input->SetWhitespace(whitespace); input->SetTabWidth(tab_width,use_tabs); }
}

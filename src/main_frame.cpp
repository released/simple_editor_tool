#include "main_frame.h"
#include "resource.h"
#include "ui/localization.h"
#include "workspace_frame.h"

#include <Shlwapi.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_WM_DROPFILES()
    ON_WM_CLOSE()
    ON_WM_DESTROY()
    ON_MESSAGE(WM_EDITOR_SAVED_CLOSE, OnSavedClose)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_FILE_RELOAD, OnReload)
    ON_COMMAND_RANGE(ID_REVIEW_DISK,ID_REVIEW_EDITORCONFIG,OnReview)
    ON_COMMAND_RANGE(ID_DOCUMENT_ALIGN_ADD,ID_DOCUMENT_NOTES_CLEAR,OnDocumentFocus)
    ON_COMMAND_RANGE(ID_WORKFLOW_BASE_SOURCE,ID_WORKFLOW_SOURCE_RELOAD,OnExpansion)
    ON_COMMAND(ID_FILE_SAVE, OnSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnSaveAs)
    ON_COMMAND(ID_FILE_COMPARE, OnCompare)
    ON_COMMAND(ID_FILE_COMPARE_EQUAL, OnCompareEqual)
    ON_COMMAND(ID_FILE_COMPARE_CANCEL, OnCompareCancel)
    ON_COMMAND(ID_EDIT_UNDO, OnUndo)
    ON_COMMAND(ID_EDIT_REDO, OnRedo)
    ON_COMMAND(ID_EDIT_INSERT, OnInsert)
    ON_COMMAND(ID_EDIT_DELETE, OnDelete)
    ON_COMMAND_RANGE(ID_HEX_COPY, ID_HEX_READ_ONLY, OnHexCommand)
    ON_COMMAND(ID_EXPORT_BINARY_REPORT, OnExportBinaryReport)
    ON_COMMAND_RANGE(ID_VIEW_WRAP, ID_HEX_LAYOUT, OnEditorCommand)
    ON_COMMAND_RANGE(ID_TOOL_INSPECT_LE, ID_TOOL_BIN_SREC, OnBinaryTool)
    ON_COMMAND(ID_TEXT_CONVERT_COPY, OnConvertCopy)
    ON_COMMAND_RANGE(ID_BINARY_TAKE_RIGHT, ID_BINARY_TAKE_LEFT, OnBinaryMerge)
    ON_COMMAND(ID_COMPARE_REGION, OnComparisonRegion)
    ON_COMMAND(ID_EXPORT_TEXT_REPORT, OnExportTextReport)
    ON_COMMAND(ID_APPLY_TEXT_PATCH, OnApplyPatch)
    ON_COMMAND(ID_APP_ABOUT,OnAboutNative)
    ON_COMMAND(ID_TOOL_TEMPLATE,OnTemplate)
    ON_COMMAND(ID_VIEW_TEXT_PAGER,OnTextPager)
    ON_COMMAND_RANGE(ID_FEATURE_BOOKMARK_ADD,ID_FEATURE_CRC,OnFeature)
    ON_COMMAND_RANGE(ID_WORKFLOW_FOLDER_RULES,ID_WORKFLOW_STRING_EXPORT,OnWorkflow)
    ON_CONTROL(EN_CHANGE, 502, OnTextChanged)
    ON_COMMAND(ID_EDIT_FIND, OnFind)
    ON_COMMAND(ID_EDIT_REPLACE, OnReplace)
    ON_COMMAND(ID_EDIT_REPEAT, OnFindNext)
    ON_COMMAND(ID_EDIT_GOTO, OnGoTo)
    ON_COMMAND(ID_COMPARE_PREVIOUS, OnPreviousDifference)
    ON_COMMAND(ID_COMPARE_NEXT, OnNextDifference)
    ON_COMMAND(ID_COMPARE_BROWSE_LEFT, OnCompareBrowseLeft)
    ON_COMMAND(ID_COMPARE_BROWSE_RIGHT, OnCompareBrowseRight)
    ON_COMMAND(ID_COMPARE_RUN, OnCompareRun)
    ON_COMMAND(ID_COMPARE_SWAP, OnCompareSwap)
    ON_COMMAND(ID_COMPARE_BROWSE_BASE, OnCompareBrowseBase)
    ON_CONTROL(CBN_SELCHANGE, IDC_COMPARE_MODE, OnCompareModeChanged)
    ON_CONTROL(CBN_EDITCHANGE, IDC_COMPARE_LEFT_PATH, OnComparePathChanged)
    ON_CONTROL(CBN_EDITCHANGE, IDC_COMPARE_RIGHT_PATH, OnComparePathChanged)
    ON_CONTROL(CBN_SELCHANGE, IDC_COMPARE_LEFT_PATH, OnComparePathChanged)
    ON_CONTROL(CBN_SELCHANGE, IDC_COMPARE_RIGHT_PATH, OnComparePathChanged)
    ON_CONTROL(CBN_EDITCHANGE, IDC_COMPARE_BASE_PATH, OnComparePathChanged)
    ON_CONTROL(CBN_SELCHANGE, IDC_COMPARE_BASE_PATH, OnComparePathChanged)
    ON_COMMAND_RANGE(ID_ENCODING_AUTO, ID_ENCODING_SYSTEM, OnEncoding)
    ON_WM_TIMER()
    ON_UPDATE_COMMAND_UI(ID_FILE_COMPARE, OnUpdateJobCommand)
    ON_UPDATE_COMMAND_UI(ID_FILE_COMPARE_EQUAL, OnUpdateJobCommand)
    ON_UPDATE_COMMAND_UI(ID_FILE_COMPARE_CANCEL, OnUpdateJobCommand)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI_RANGE(ID_ENCODING_AUTO, ID_ENCODING_SYSTEM, OnUpdateSaveCommand)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_STATUS, OnUpdateZoomCommand)
    ON_NOTIFY(TCN_SELCHANGING, 500, OnTabChanging)
    ON_NOTIFY(TCN_SELCHANGE, 500, OnTabChanged)
    ON_NOTIFY(LVN_GETDISPINFO, 503, OnDifferenceDisplay)
    ON_NOTIFY(NM_DBLCLK, 503, OnDifferenceActivate)
    ON_WM_COPYDATA()
END_MESSAGE_MAP()

CMainFrame::CMainFrame(bool publish_instance, CWorkspaceFrame* workspace) : workspace_(workspace), cancelled_(false), suppress_text_change_(false), text_dirty_(false) {
    Create(nullptr, L"Simple Editor Tool - Native MFC", workspace ? WS_CHILD | WS_CLIPCHILDREN : WS_OVERLAPPEDWINDOW,
           rectDefault, workspace, nullptr);
    SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
    simple_editor::ui::RegisterLocalizationRoot(this); simple_editor::ui::RegisterLocalizationMenu(session_menu_.GetSafeHmenu(),GetSafeHwnd());
    if (publish_instance) ::SetPropW(m_hWnd, L"SimpleEditorTool.Native.MainWindow", reinterpret_cast<HANDLE>(1));
}
CMainFrame::~CMainFrame() { cancelled_ = true; if (job_.valid()) job_.wait(); ReleaseJobLocks(); if (!workspace_) session_menu_.Detach(); }
void CMainFrame::OnDestroy() {
    if (live_inspector_.GetSafeHwnd()) live_inspector_.DestroyWindow();
    if (log_tail_.GetSafeHwnd()) log_tail_.DestroyWindow();
    KillTimer(1); cancelled_ = true;
    if (search_dialog_.GetSafeHwnd()) search_dialog_.DestroyWindow();
    if (search_results_.GetSafeHwnd()) search_results_.DestroyWindow();
    CFrameWnd::OnDestroy();
}
LRESULT CMainFrame::OnSavedClose(WPARAM, LPARAM) {
    if (saved_close_pending_) { saved_close_pending_ = false; OnClose(); }
    return 0;
}
BOOL CMainFrame::PreTranslateMessage(MSG* message) {
    if (active_tab_ == 1) ActiveEditor();
    if (workspace_ && workspace_->DispatchShortcut(message)) return TRUE;
    if (search_dialog_.GetSafeHwnd() && search_dialog_.IsWindowVisible() &&
        (message->hwnd == search_dialog_.GetSafeHwnd() || search_dialog_.IsChild(CWnd::FromHandle(message->hwnd)))) {
        if (search_dialog_.IsDialogMessage(message)) return TRUE;
        return CFrameWnd::PreTranslateMessage(message);
    }
    if (message->message == WM_MOUSEWHEEL && (LOWORD(message->wParam) & MK_CONTROL) &&
        (message->hwnd == GetSafeHwnd() || ::IsChild(GetSafeHwnd(), message->hwnd))) {
        CPoint point(static_cast<short>(LOWORD(message->lParam)), static_cast<short>(HIWORD(message->lParam)));
        ScreenToClient(&point);
        if ((active_tab_ == 2 ? compare_view_rect_ : page_rect_).PtInRect(point)) { ZoomWheel(active_tab_, static_cast<short>(HIWORD(message->wParam))); return TRUE; }
    }
    if (message->message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000)) {
        if (message->wParam == '0' || message->wParam == VK_NUMPAD0) {
            wheel_remainder_[active_tab_] = 0; SetTabZoom(active_tab_, 100); return TRUE;
        }
        if (message->wParam == 'O') { OnFileOpen(); return TRUE; }
        if (message->wParam == 'S') { OnSave(); return TRUE; }
        if (message->wParam == 'R') { OnReload(); return TRUE; }
        if (message->wParam == 'Z') { OnUndo(); return TRUE; }
        if (message->wParam == 'Y') { OnRedo(); return TRUE; }
        if (message->wParam == 'F') { OnFind(); return TRUE; }
        if (message->wParam == 'H') { OnReplace(); return TRUE; }
        if (message->wParam == 'G') { OnGoTo(); return TRUE; }
    }
    if (message->message == WM_KEYDOWN && message->wParam == VK_F3) { OnFindNext(); return TRUE; }
    if (message->message == WM_KEYDOWN && message->wParam == VK_F7) { if (GetKeyState(VK_SHIFT) & 0x8000) OnPreviousDifference(); else OnNextDifference(); return TRUE; }
    if (message->message == WM_KEYDOWN && message->wParam == VK_ESCAPE && IsBusy()) { OnCompareCancel(); return TRUE; }
    return CFrameWnd::PreTranslateMessage(message);
}

int CMainFrame::OnCreate(LPCREATESTRUCT create) {
    if (CFrameWnd::OnCreate(create) == -1) return -1;
    CMenu menu; menu.CreateMenu(); CMenu file; file.CreatePopupMenu();
    file.AppendMenu(MF_STRING, ID_FILE_OPEN, L"&Open files...\tCtrl+O"); file.AppendMenu(MF_STRING, ID_FILE_RELOAD, L"&Reload from disk\tCtrl+R"); file.AppendMenu(MF_STRING, ID_FILE_SAVE, L"&Save\tCtrl+S"); file.AppendMenu(MF_STRING, ID_FILE_SAVE_AS, L"Save &As..."); file.AppendMenu(MF_STRING, ID_FILE_COMPARE, L"Compare - &Insertion aware..."); file.AppendMenu(MF_STRING, ID_FILE_COMPARE_EQUAL, L"Compare - &Equal address..."); file.AppendMenu(MF_STRING, ID_FILE_COMPARE_CANCEL, L"&Cancel current operation\tEsc"); if (workspace_) file.AppendMenu(MF_STRING, ID_FILE_CLOSE, L"Close &tab\tCtrl+W"); file.AppendMenu(MF_SEPARATOR); file.AppendMenu(MF_STRING, ID_APP_EXIT, L"E&xit");
    if (workspace_) workspace_->AppendFileActions(file);
    CMenu edit; edit.CreatePopupMenu(); edit.AppendMenu(MF_STRING, ID_EDIT_UNDO, L"&Undo\tCtrl+Z"); edit.AppendMenu(MF_STRING, ID_EDIT_REDO, L"&Redo\tCtrl+Y"); edit.AppendMenu(MF_STRING, ID_EDIT_INSERT, L"Insert zero byte at caret"); edit.AppendMenu(MF_STRING, ID_EDIT_DELETE, L"Delete byte at caret");
    edit.AppendMenu(MF_SEPARATOR); edit.AppendMenu(MF_STRING, ID_EDIT_FIND, L"&Find...\tCtrl+F"); edit.AppendMenu(MF_STRING, ID_EDIT_REPEAT, L"Find &next\tF3"); edit.AppendMenu(MF_STRING, ID_EDIT_REPLACE, L"&Replace...\tCtrl+H"); edit.AppendMenu(MF_STRING, ID_EDIT_GOTO, L"&Go to address...\tCtrl+G");
    if (workspace_) { edit.AppendMenu(MF_STRING,ID_WORKSPACE_SEARCH_FILES,L"Find in open files / folder...\tCtrl+Shift+F"); edit.AppendMenu(MF_STRING,ID_WORKSPACE_BATCH_REPLACE,L"Preview replacements in folder..."); }
    CMenu encoding; encoding.CreatePopupMenu(); const wchar_t* encodings[] = {L"Auto detect", L"UTF-8", L"UTF-16 LE", L"UTF-16 BE", L"Big5 / CP950", L"System code page"};
    for (int i = 0; i < 6; ++i) encoding.AppendMenu(MF_STRING, ID_ENCODING_AUTO + i, encodings[i]);
    encoding.CheckMenuRadioItem(ID_ENCODING_AUTO, ID_ENCODING_SYSTEM, ID_ENCODING_AUTO, MF_BYCOMMAND);
    encoding.AppendMenu(MF_SEPARATOR); encoding.AppendMenu(MF_STRING,ID_TEXT_CONVERT_COPY,L"Convert output encoding / EOL to a copy...");
    CMenu compare; compare.CreatePopupMenu(); compare.AppendMenu(MF_STRING, ID_COMPARE_PREVIOUS, L"&Previous difference\tShift+F7"); compare.AppendMenu(MF_STRING, ID_COMPARE_NEXT, L"&Next difference\tF7");
    compare.AppendMenu(MF_STRING,ID_EXPORT_BINARY_REPORT,L"Export binary differences...");
    compare.AppendMenu(MF_STRING,ID_COMPARE_REGION,L"Binary ranges / ignored spans...");
    compare.AppendMenu(MF_STRING,ID_EXPORT_TEXT_REPORT,L"Export text report / unified patch..."); edit.AppendMenu(MF_STRING,ID_APPLY_TEXT_PATCH,L"Apply unified patch to text...");
    compare.AppendMenu(MF_STRING,ID_BINARY_TAKE_RIGHT,L"Take selected right bytes into a copy of left..."); compare.AppendMenu(MF_STRING,ID_BINARY_TAKE_LEFT,L"Take selected left bytes into a copy of right...");
    CMenu bytes; bytes.CreatePopupMenu();
    bytes.AppendMenu(MF_STRING,ID_HEX_COPY,L"Copy Hex"); bytes.AppendMenu(MF_STRING,ID_HEX_COPY_TEXT,L"Copy decoded text"); bytes.AppendMenu(MF_STRING,ID_HEX_CUT,L"Cut bytes");
    bytes.AppendMenu(MF_STRING,ID_HEX_PASTE_WRITE,L"Paste overwrite"); bytes.AppendMenu(MF_STRING,ID_HEX_PASTE_INSERT,L"Paste insert"); bytes.AppendMenu(MF_STRING,ID_HEX_PASTE_TEXT,L"Paste UTF-8 text");
    bytes.AppendMenu(MF_STRING,ID_HEX_SELECT_ALL,L"Select all bytes"); bytes.AppendMenu(MF_STRING,ID_HEX_INSERT_MODE,L"Toggle insert / overwrite"); bytes.AppendMenu(MF_STRING,ID_HEX_READ_ONLY,L"Toggle read-only"); edit.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(bytes.Detach()),L"Hex operations");
    compare.AppendMenu(MF_SEPARATOR); compare.AppendMenu(MF_STRING,ID_WORKSPACE_TEXT_COMPARE,L"Text/source comparison..."); compare.AppendMenu(MF_STRING,ID_WORKSPACE_THREE_MERGE,L"Three-way text merge..."); compare.AppendMenu(MF_STRING,ID_WORKSPACE_FOLDERS,L"Folder comparison / sync...");
    compare.AppendMenu(MF_STRING,ID_WORKSPACE_PASTED_TEXT,L"Pasted text comparison...");
    file.AppendMenu(MF_STRING,ID_WORKSPACE_SAVE_SESSION,L"Save workspace..."); file.AppendMenu(MF_STRING,ID_WORKSPACE_LOAD_SESSION,L"Open workspace...");
    compare.AppendMenu(MF_STRING,ID_WORKFLOW_FOLDER_RULES,L"Folder text comparison rules..."); compare.AppendMenu(MF_STRING,ID_WORKFLOW_CONTEXT,L"Unchanged context lines..."); compare.AppendMenu(MF_STRING,ID_WORKFLOW_TREE,L"Toggle folder result tree"); compare.AppendMenu(MF_STRING,ID_WORKFLOW_TREE_EXPAND,L"Expand all result folders"); compare.AppendMenu(MF_STRING,ID_WORKFLOW_TREE_COLLAPSE,L"Collapse all result folders"); compare.AppendMenu(MF_STRING,ID_FEATURE_ANCHORS,L"Manual line alignment..."); compare.AppendMenu(MF_STRING,ID_FEATURE_RENAMES,L"Find renamed / moved content..."); compare.AppendMenu(MF_STRING,ID_FEATURE_TABLE_RULES,L"Table keys and columns..."); compare.AppendMenu(MF_STRING,ID_FEATURE_TEXT_RULES,L"Text comparison rules...");
    menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(file.Detach()), L"&File"); menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(edit.Detach()), L"&Edit"); menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(encoding.Detach()), L"E&ncoding"); menu.AppendMenu(MF_POPUP, reinterpret_cast<UINT_PTR>(compare.Detach()), L"&Compare");
    if (workspace_) workspace_->UpdateMenuShortcuts(menu);
    CMenu view_menu; view_menu.CreatePopupMenu(); view_menu.AppendMenu(MF_STRING,ID_WORKFLOW_PALETTE,L"Search commands...\tCtrl+Shift+P"); view_menu.AppendMenu(MF_STRING,ID_WORKFLOW_BACK,L"Back\tAlt+Left"); view_menu.AppendMenu(MF_STRING,ID_WORKFLOW_FORWARD,L"Forward\tAlt+Right"); view_menu.AppendMenu(MF_STRING,ID_WORKFLOW_CHANGE_PREVIOUS,L"Previous unsaved modification"); view_menu.AppendMenu(MF_STRING,ID_WORKFLOW_CHANGE_NEXT,L"Next unsaved modification"); const wchar_t* labels[] = {L"Word wrap",L"Show whitespace / EOL",L"Dark content",L"Go to line...",L"Toggle bookmark",L"Next bookmark",L"Previous bookmark",L"Clear bookmarks",L"Fold all",L"Expand all",L"Column selection mode",L"Add next occurrence to selection",L"Tab width...",L"Text font...",L"Complete word",L"Hex layout / base address..."}; for (unsigned i = 0; i < _countof(labels); ++i) view_menu.AppendMenu(MF_STRING,ID_VIEW_WRAP + i,labels[i]); view_menu.AppendMenu(MF_STRING,ID_FEATURE_CLONE,L"Toggle shared Text views"); view_menu.AppendMenu(MF_STRING,ID_FEATURE_COMPLETE,L"Complete from document / language"); CMenu lines; lines.CreatePopupMenu(); const wchar_t* line_names[] = {L"Sort lines ascending",L"Sort lines descending",L"Remove duplicate lines",L"Remove blank lines",L"Trim trailing spaces"}; for (unsigned i = 0; i < 5; ++i) lines.AppendMenu(MF_STRING,ID_FEATURE_SORT_ASC + i,line_names[i]); view_menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(lines.Detach()),L"Line operations"); menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(view_menu.Detach()),L"&View");
    CMenu tools; tools.CreatePopupMenu(); tools.AppendMenu(MF_STRING,ID_WORKFLOW_DECODING,L"Hex decoded character column..."); tools.AppendMenu(MF_STRING,ID_WORKFLOW_CHANGES,L"Review unsaved byte modifications..."); tools.AppendMenu(MF_STRING,ID_WORKFLOW_STRINGS,L"Extract binary strings..."); tools.AppendMenu(MF_STRING,ID_WORKFLOW_STRING_EXPORT,L"Export extracted string previews..."); tools.AppendMenu(MF_STRING,ID_WORKFLOW_INTEGERS,L"Selected integer operations..."); const wchar_t* tool_labels[] = {L"Inspect at caret (little endian)...",L"Inspect at caret (big endian)...",L"CRC32 / SHA-256 / entropy...",L"Extract selection / file...",L"Export selection / file as C array...",L"Fill selected bytes...",L"Invert selected bytes",L"Swap 2-byte words",L"Swap 4-byte words",L"Swap 8-byte words",L"Reverse selected bytes",L"Split into files...",L"Join files in chosen order...",L"Validate Intel HEX / address map...",L"Validate S-record / address map...",L"Intel HEX to BIN...",L"S-record to BIN...",L"BIN to Intel HEX...",L"BIN to S-record..."}; for (unsigned i = 0; i < _countof(tool_labels); ++i) tools.AppendMenu(MF_STRING,ID_TOOL_INSPECT_LE + i,tool_labels[i]); tools.AppendMenu(MF_STRING,ID_FEATURE_CRC,L"Named / custom CRC..."); tools.AppendMenu(MF_STRING,ID_FEATURE_INSPECTOR,L"Live numeric inspector..."); tools.AppendMenu(MF_STRING,ID_FEATURE_BOOKMARK_ADD,L"Name selected Hex range..."); tools.AppendMenu(MF_STRING,ID_FEATURE_BOOKMARK_LIST,L"Hex range bookmarks..."); tools.AppendMenu(MF_STRING,ID_FEATURE_TAIL,L"Follow saved log file..."); tools.AppendMenu(MF_STRING,ID_VIEW_TEXT_PAGER,L"Large text pages..."); tools.AppendMenu(MF_STRING,ID_TOOL_TEMPLATE,L"Open binary field template..."); if (workspace_) CWorkspaceFrame::AppendExplorerMenu(tools); menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(tools.Detach()),L"&Tools");
    
    AppendReviewMenu(menu); if (workspace_) simple_editor::ui::AppendLanguageMenu(menu); CMenu help_menu; help_menu.CreatePopupMenu(); help_menu.AppendMenu(MF_STRING,ID_APP_ABOUT,L"About / native component notices..."); menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(help_menu.Detach()),L"&Help");
    session_menu_.Attach(menu.Detach()); if (!workspace_) SetMenu(&session_menu_);
    tabs_.Create(WS_CHILD | WS_VISIBLE | TCS_TABS, CRect(), this, 500); tabs_.InsertItem(0, L"Hex"); tabs_.InsertItem(1, L"Text"); tabs_.InsertItem(2, L"Compare");
    const CString hex_class = AfxRegisterWndClass(CS_DBLCLKS, LoadCursor(nullptr, IDC_IBEAM), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    hex_.Create(hex_class, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL, CRect(), this, 501);
    compare_left_.Create(hex_class, nullptr, WS_CHILD | WS_VSCROLL, CRect(), this, 504);
    compare_right_.Create(hex_class, nullptr, WS_CHILD | WS_VSCROLL, CRect(), this, 505);
    compare_left_label_.Create(L"Left", WS_CHILD | SS_LEFTNOWORDWRAP, CRect(), this, 506);
    compare_right_label_.Create(L"Right", WS_CHILD | SS_LEFTNOWORDWRAP, CRect(), this, 507);
    compare_left_path_.Create(WS_CHILD | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, CRect(), this, IDC_COMPARE_LEFT_PATH);
    compare_right_path_.Create(WS_CHILD | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL, CRect(), this, IDC_COMPARE_RIGHT_PATH);
    compare_browse_left_.Create(L"Browse...", WS_CHILD | BS_PUSHBUTTON, CRect(), this, ID_COMPARE_BROWSE_LEFT);
    compare_browse_right_.Create(L"Browse...", WS_CHILD | BS_PUSHBUTTON, CRect(), this, ID_COMPARE_BROWSE_RIGHT);
    compare_mode_.Create(WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST, CRect(), this, IDC_COMPARE_MODE);
    for (auto value : {L"Hex - insertion aware",L"Hex - equal address",L"Text/source (2-way)",L"Text merge (3-way)",L"Folders / sync",L"CSV fields (read-only)",L"TSV fields (read-only)",L"JSON values (read-only)",L"Pasted text (2-way)"}) compare_mode_.AddString(value); compare_mode_.SetCurSel(0);
    compare_run_.Create(L"Compare", WS_CHILD | BS_PUSHBUTTON, CRect(), this, ID_COMPARE_RUN);
    compare_swap_.Create(L"Swap sides", WS_CHILD | BS_PUSHBUTTON, CRect(), this, ID_COMPARE_SWAP);
    compare_base_label_.Create(L"Base file:",WS_CHILD,CRect(),this,525);
    compare_base_path_.Create(WS_CHILD | WS_VSCROLL | CBS_DROPDOWN | CBS_AUTOHSCROLL,CRect(),this,IDC_COMPARE_BASE_PATH);
    compare_browse_base_.Create(L"Browse...",WS_CHILD | BS_PUSHBUTTON,CRect(),this,ID_COMPARE_BROWSE_BASE);
    InitializeAdvanced();
    compare_left_.SetReadOnly(true); compare_right_.SetReadOnly(true);
    for (CHexView* view : {static_cast<CHexView*>(&hex_),static_cast<CHexView*>(&compare_left_),static_cast<CHexView*>(&compare_right_)}) {
        view->SetSelectionCallback([this]() { HexSelectionChanged(); });
        view->report = [this](const std::wstring& value) { if (status_.GetSafeHwnd()) status_.SetPaneText(0,value.c_str()); };
    }
    compare_left_.SetScrollCallback([this](std::uint64_t row) { SynchronizeCompareScroll(true,row); });
    compare_right_.SetScrollCallback([this](std::uint64_t row) { SynchronizeCompareScroll(false,row); });
    hex_.SetChangeCallback([this]() { DocumentChanged(); });
    const CString splitter_class = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), nullptr);
    vertical_splitter_.Create(splitter_class, nullptr, WS_CHILD, CRect(), this, 508);
    horizontal_splitter_.Create(splitter_class, nullptr, WS_CHILD, CRect(), this, 509); horizontal_splitter_.vertical = false;
    vertical_splitter_.move = [this](CPoint p) { horizontal_ratio_ = (std::clamp)((p.x - compare_view_rect_.left) / static_cast<double>((std::max)(1, compare_view_rect_.Width())), 0.2, 0.8); Layout(); };
    horizontal_splitter_.move = [this](CPoint p) { vertical_ratio_ = (std::clamp)((p.y - compare_view_rect_.top) / static_cast<double>((std::max)(1, compare_view_rect_.Height())), 0.25, 0.85); Layout(); };
    text_.Create(WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN, CRect(), this, 502);
    text_.SetLimitText(32 * 1024 * 1024);
    text_font_.CreatePointFont(100, L"Consolas"); text_.SetFont(&text_font_);
    differences_.Create(WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDATA, CRect(), this, 503); differences_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    differences_.InsertColumn(0, L"#", LVCFMT_RIGHT, 70); differences_.InsertColumn(1, L"Left address", LVCFMT_LEFT, 145); differences_.InsertColumn(2, L"Left", LVCFMT_LEFT, 70); differences_.InsertColumn(3, L"Right address", LVCFMT_LEFT, 145); differences_.InsertColumn(4, L"Right", LVCFMT_LEFT, 70);
    status_.Create(this); UINT indicators[] = { ID_SEPARATOR, ID_VIEW_ZOOM_STATUS }; status_.SetIndicators(indicators, 2);
    status_.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH | SBPS_NOBORDERS, 0);
    InitializeZoom();
    DragAcceptFiles(TRUE); hex_.SetDocument(&document_); hex_.bookmarks = &bookmarks_; hex_.modified_ranges = &modified_ranges_; compare_left_.SetDocument(&compare_left_document_);
    search_dialog_.execute = [this](const SearchRequest& request) { ExecuteSearch(request); };
    CreateSourceNotice(); SetTimer(1, 50, nullptr); Layout(); return 0;
}

void CMainFrame::OnSize(UINT type, int cx, int cy) { CFrameWnd::OnSize(type, cx, cy); Layout(); }
void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* info) { CFrameWnd::OnGetMinMaxInfo(info); info->ptMinTrackSize.x = 640; info->ptMinTrackSize.y = 400; }
void CMainFrame::Layout() {
    if (!tabs_.GetSafeHwnd() || !status_.GetSafeHwnd() || !differences_.GetSafeHwnd()) return; CRect r; GetClientRect(&r); CRect sr; status_.GetWindowRect(&sr); r.bottom = (std::max<LONG>)(0, r.bottom - sr.Height()); tabs_.MoveWindow(r);
    CRect page = r; tabs_.AdjustRect(FALSE, &page); page.DeflateRect(4, 4);
    page.right = (std::max)(page.left, page.right); page.bottom = (std::max)(page.top, page.bottom); page_rect_ = page;
    hex_.MoveWindow(page); CRect text_page = page; LayoutSourceNotice(text_page); if (dual_view_ && text_clone_.GetSafeHwnd()) { CRect left = text_page,right = text_page; left.right = text_page.left + text_page.Width() / 2 - 2; right.left = left.right + 4; text_.MoveWindow(left); text_clone_.MoveWindow(right); } else text_.MoveWindow(text_page);
    compare_mode_.MoveWindow(page.left, page.top, 220, 200); compare_run_.MoveWindow(page.left + 228, page.top, 90, 25); compare_swap_.MoveWindow(page.left + 326, page.top, 100, 25);
    const int middle = page.left + page.Width() / 2;
    compare_left_label_.MoveWindow(page.left, page.top + 30, (std::max<LONG>)(0, middle - page.left - 4), 20);
    compare_right_label_.MoveWindow(middle + 4, page.top + 30, (std::max<LONG>)(0, page.right - middle - 4), 20);
    compare_left_path_.MoveWindow(page.left, page.top + 50, (std::max<LONG>)(0, middle - page.left - 85), 240);
    compare_browse_left_.MoveWindow(middle - 79, page.top + 50, 75, 24);
    compare_right_path_.MoveWindow(middle + 4, page.top + 50, (std::max<LONG>)(0, page.right - middle - 85), 240);
    compare_browse_right_.MoveWindow(page.right - 75, page.top + 50, 75, 24);
    compare_base_label_.MoveWindow(page.left,page.top + 80,65,24);
    compare_base_path_.MoveWindow(page.left + 70,page.top + 78,(std::max)(0,page.Width() - 155),240);
    compare_browse_base_.MoveWindow(page.right - 75,page.top + 78,75,24);
    CRect content = page; content.top = (std::min)(page.bottom, page.top + (displayed_mode_ == 8 ? 32 : displayed_mode_ == 3 ? 110 : 82)); compare_view_rect_ = content;
    if (advanced_.GetSafeHwnd()) advanced_.MoveWindow(content);
    const int compare_height = static_cast<int>(content.Height() * vertical_ratio_); CRect left = content; left.right = content.left + static_cast<int>(content.Width() * horizontal_ratio_) - 3; left.bottom = content.top + compare_height; CRect right = content; right.left = left.right + 6; right.bottom = left.bottom; CRect list = content; list.top = (std::min)(content.bottom, left.bottom + 6);
    compare_left_.MoveWindow(left); compare_right_.MoveWindow(right); differences_.MoveWindow(list);
    vertical_splitter_.MoveWindow(left.right, content.top, 6, (std::max<LONG>)(0, left.bottom - content.top));
    horizontal_splitter_.MoveWindow(content.left, left.bottom, content.Width(), 6);
    Invalidate(TRUE);
}
void CMainFrame::OnTabChanging(NMHDR*, LRESULT* result) { *result = active_tab_ == 1 && !ApplyTextEdits() ? TRUE : FALSE; }
void CMainFrame::OnTabChanged(NMHDR*, LRESULT* result) { ShowPage(tabs_.GetCurSel()); *result = 0; }
void CMainFrame::ShowPage(int tab) {
    if (active_tab_ == 1 && tab != 1 && !ApplyTextEdits()) { tabs_.SetCurSel(1); return; }
    active_tab_ = tab; tabs_.SetCurSel(tab);
    if (text_clone_.GetSafeHwnd()) text_clone_.ShowWindow(tab == 1 && dual_view_ ? SW_SHOW : SW_HIDE);
    UpdateZoomStatus();
    if (tab == 1 && text_stale_) RefreshText();
    const bool binary = tab == 2 && displayed_mode_ < 2;
    hex_.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE); text_.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE); compare_left_label_.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE); compare_right_label_.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE); compare_left_.ShowWindow(binary ? SW_SHOW : SW_HIDE); compare_right_.ShowWindow(binary ? SW_SHOW : SW_HIDE); differences_.ShowWindow(binary ? SW_SHOW : SW_HIDE);
    vertical_splitter_.ShowWindow(binary ? SW_SHOW : SW_HIDE); horizontal_splitter_.ShowWindow(binary ? SW_SHOW : SW_HIDE); if (advanced_.GetSafeHwnd()) advanced_.ShowWindow(AdvancedActive() ? SW_SHOW : SW_HIDE);
    compare_base_label_.ShowWindow(tab == 2 && displayed_mode_ == 3); compare_base_path_.ShowWindow(tab == 2 && displayed_mode_ == 3); compare_browse_base_.ShowWindow(tab == 2 && displayed_mode_ == 3); Invalidate(TRUE);
    for (CWnd* control : {static_cast<CWnd*>(&compare_left_path_), static_cast<CWnd*>(&compare_right_path_), static_cast<CWnd*>(&compare_browse_left_), static_cast<CWnd*>(&compare_browse_right_), static_cast<CWnd*>(&compare_mode_), static_cast<CWnd*>(&compare_run_), static_cast<CWnd*>(&compare_swap_)}) control->ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
    if (displayed_mode_ == 8) for (CWnd* control : {static_cast<CWnd*>(&compare_left_path_),static_cast<CWnd*>(&compare_right_path_),static_cast<CWnd*>(&compare_browse_left_),static_cast<CWnd*>(&compare_browse_right_),static_cast<CWnd*>(&compare_left_label_),static_cast<CWnd*>(&compare_right_label_)}) control->ShowWindow(SW_HIDE);
    Layout();
}

bool CMainFrame::OpenPath(const std::wstring& path) {
    if (IsSaving()) {
        after_save_ = AfterSave::Open; after_save_path_ = path;
        status_.SetPaneText(0, L"The requested file will open after saving succeeds."); return false;
    }
    if (!ConfirmDiscardChanges(AfterSave::Open, path)) return false;
    std::wstring error; if (!document_.Open(path, &error)) { UiMessageBox((L"Cannot open file:\n" + error).c_str(), MB_ICONERROR); return false; }
    ++revision_; modified_ranges_.clear(); modified_revision_ = UINT64_MAX; cancelled_ = true; match_offset_ = UINT64_MAX; match_pattern_.clear(); source_notice_key_.clear(); source_dismissed_key_.clear(); source_notice_visible_ = false; unicode_generation_ = UINT64_MAX;
    ClearComparison(); hex_.SetDocument(&document_); text_dirty_ = false; text_.SetModify(FALSE); text_stale_ = true;
    SetWindowTextW((std::filesystem::path(path).filename().wstring() + L" - Simple Editor Tool").c_str()); status_.SetPaneText(0, (path + L"   " + std::to_wstring(document_.Size()) + L" bytes").c_str());
    ShowPage(0); if (document_.Size() <= 32 * 1024 * 1024) { RefreshText(); if (text_editable_) ShowPage(1); } RequestIndentation(true); if (workspace_) workspace_->RefreshLabels(); return true;
}
void CMainFrame::OnFileOpen() {
    if (workspace_) { workspace_->ChooseFiles(); return; }
    if (IsSaving()) { after_save_ = AfterSave::OpenDialog; after_save_path_.clear(); status_.SetPaneText(0, L"The Open dialog will appear after saving succeeds."); return; }
    CFileDialog dialog(TRUE, nullptr, nullptr, OFN_FILEMUSTEXIST, L"All files (*.*)|*.*||", this);
    if (dialog.DoModal() == IDOK) OpenPath(dialog.GetPathName().GetString());
}
void CMainFrame::OnReload() {
    if (untitled_) { status_.SetPaneText(0,L"Save the untitled document to a user file before reloading."); return; }
    if (AdvancedActive()) { OnCompareRun(); return; }
    if (workspace_ && workspace_->DocumentLocked(document_)) { status_.SetPaneText(0,L"A synchronization/merge operation is using this file. Wait for it to finish before reloading."); return; }
    if (IsSaving()) { after_save_ = AfterSave::Reload; after_save_path_.clear(); status_.SetPaneText(0, L"The file will reload after saving succeeds."); return; }
    if (!document_.IsOpen()) return;
    const auto path = document_.Path();
    if (OpenPath(path)) status_.SetPaneText(0, L"Reloaded from disk.");
}
void CMainFrame::OnDropFiles(HDROP drop) { if (workspace_) { workspace_->OpenDroppedFiles(drop); return; } wchar_t path[MAX_PATH]; if (DragQueryFileW(drop, 0, path, MAX_PATH)) OpenPath(path); DragFinish(drop); }
bool CMainFrame::ConfirmDiscardChanges(AfterSave next, const std::wstring& next_path) {
    if (!document_.IsModified() && !text_dirty_ && !text_.GetModify() && !untitled_dirty_) return true;
    const int answer = UiMessageBox(L"The current file has unsaved changes. Save them now?", MB_YESNOCANCEL | MB_ICONWARNING);
    if (answer == IDCANCEL) return false;
    if (answer == IDYES) { SaveCurrent(next,next_path); return false; }
    return true;
}
void CMainFrame::OnClose() {
    if (workspace_) { workspace_->CloseSession(this); return; }
    if (IsSaving()) { after_save_ = AfterSave::Close; after_save_path_.clear(); status_.SetPaneText(0, L"The window will close after saving succeeds. Esc cancels this request."); return; }
    if (ConfirmDiscardChanges(AfterSave::Close)) CFrameWnd::OnClose();
}
BOOL CMainFrame::OnCopyData(CWnd*, COPYDATASTRUCT* data) { if (data && data->lpData) { OpenPath(static_cast<const wchar_t*>(data->lpData)); ShowWindow(SW_SHOWMAXIMIZED); SetForegroundWindow(); return TRUE; } return FALSE; }


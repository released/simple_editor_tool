#include <afxwin.h>
#include <afxcmn.h>
#include <Shlobj.h>
#include "../src/main_frame.h"
#include "../src/workspace_frame.h"
#include "../src/resource.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include "../src/core/sync_journal.h"
#include <iostream>
#include <atlimage.h>
#include "../src/ui/template_dialog.h"
#include "../src/ui/text_pager.h"
#include "native_exception_trace.h"
#include "drop_data_object.h"
#include "../src/ui/sync_preview.h"
#include "../src/ui/preferences_dialog.h"
#include "../src/ui/text_commands.h"
#include "../src/ui/batch_replace_dialog.h"
#include "../src/ui/feature_dialogs.h"
#include "../src/ui/command_palette.h"
#include "../src/services/explorer_menu.h"
#include "../src/core/ips_patch.h"
#include "../src/core/compare_presets.h"
#include "../src/ui/statistics_dialog.h"
#include "../src/ui/conversion_dialog.h"
#include "../src/ui/localization.h"

int RunWorkspaceProcessTests(const std::filesystem::path& root, const std::filesystem::path& executable);
int RunExplorerProcessTests(const std::filesystem::path& root, const std::filesystem::path& executable);

class NativeUiTest {
public:
#include "native_search_ui.inc"
#include "native_format_ui.inc"
#include "native_presentation_ui.inc"
#include "native_region_ui.inc"
#include "native_patch_ui.inc"
#include "native_drop_ui.inc"
#include "native_folder_ui.inc"
#include "native_cross_search_ui.inc"
#include "native_documents_ui.inc"
#include "native_preferences_ui.inc"
#include "native_recovery_ui.inc"
#include "native_structured_ui.inc"
#include "native_inspection_ui.inc"
#include "native_discovery_ui.inc"
#include "native_discovery_flows_ui.inc"
#include "native_discovery_visual_ui.inc"
#include "native_workflow_ui.inc"
#include "native_explorer_ui.inc"
#include "native_pasted_text_ui.inc"
#include "native_text_workflow_ui.inc"
#include "native_editor_review_ui.inc"
#include "native_workflow_expansion_ui.inc"
#include "native_localization_ui.inc"
#include "native_document_focus_ui.inc"
#include "native_outline_sidebar_ui.inc"
    static int RunCodeEditor(const std::filesystem::path& root) {
        int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
        auto* frame = new CMainFrame(false); AfxGetApp()->m_pMainWnd = frame;
        const auto path = root / L"code.cpp"; Write(path,{'i','n','t',' ','m','a','i','n','(',')',' ','{','\n',' ',' ','r','e','t','u','r','n',' ','1',';','\n','}','\n'}); frame->OpenPath(path.wstring()); frame->ShowPage(1); auto& edit = frame->text_;
        check(edit.Call(SCI_GETMARGINWIDTHN,0) > 0 && edit.Call(SCI_GETLINECOUNT) == 4,L"native editor line gutter and LF display adapter");
        const auto view_revision = frame->revision_; edit.ToggleBookmark(); edit.Call(SCI_FOLDALL,SC_FOLDACTION_EXPAND); MSG idle{}; while (::PeekMessage(&idle,nullptr,0,0,PM_REMOVE)) { ::TranslateMessage(&idle); ::DispatchMessage(&idle); }
        check(frame->revision_ == view_revision && !frame->text_dirty_,L"bookmark/fold and deferred lexer notifications do not dirty a document"); edit.Call(SCI_MARKERDELETEALL,20);
        edit.SetWindowTextW(L"a\x4E2D\U0001F680z\r\nsecond\r\nthird"); edit.SetSel(1,4); int start = 0,end = 0; edit.GetSel(start,end);
        check(start == 1 && end == 4 && edit.Call(SCI_GETSELECTIONEND) == 8,L"UTF-16 selection bridge preserves supplementary Unicode boundaries");
        edit.ReplaceSel(L"changed",TRUE); check(edit.CanUndo() && edit.GetModify(),L"native edit change sets modify and Undo"); edit.Undo(); CString value; edit.GetWindowTextW(value);
        check(value == L"a\x4E2D\U0001F680z\r\nsecond\r\nthird" && edit.CanRedo(),L"native Undo restores exact Unicode and enables Redo"); edit.Redo(); check(edit.Utf8().find("changed") != std::string::npos,L"native Redo restores the edit");
        edit.SetWindowTextW(L"same same\r\nsecond\r\nthird"); edit.GoToLine(2); check(edit.Call(SCI_LINEFROMPOSITION,edit.Call(SCI_GETCURRENTPOS)) == 1,L"Go To Line navigates by document line");
        frame->OnEditorCommand(ID_VIEW_BOOKMARK); check((edit.Call(SCI_MARKERGET,1) & (1 << 20)) != 0,L"bookmark command creates a marker"); edit.GoToLine(1); frame->OnEditorCommand(ID_VIEW_BOOKMARK_NEXT); check(edit.Call(SCI_LINEFROMPOSITION,edit.Call(SCI_GETCURRENTPOS)) == 1,L"next bookmark navigates and wraps"); frame->OnEditorCommand(ID_VIEW_BOOKMARK_CLEAR); check(edit.Call(SCI_MARKERNEXT,0,1 << 20) < 0,L"clear bookmark command removes markers");
        frame->OnEditorCommand(ID_VIEW_WRAP); frame->OnEditorCommand(ID_VIEW_WHITESPACE); check(edit.Call(SCI_GETWRAPMODE) == SC_WRAP_WORD && edit.Call(SCI_GETVIEWWS) == SCWS_VISIBLEALWAYS && edit.Call(SCI_GETVIEWEOL),L"wrap/whitespace commands change presentation");
        edit.SetTabWidth(8,true); check(edit.Call(SCI_GETTABWIDTH) == 8 && edit.Call(SCI_GETUSETABS),L"tab settings apply to indentation and inserted tabs");
        frame->OnEditorCommand(ID_VIEW_COLUMN); check(edit.Call(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE,L"column selection mode is accessible"); frame->OnEditorCommand(ID_VIEW_COLUMN);
        edit.SetSel(0,4); frame->OnEditorCommand(ID_VIEW_MULTI_NEXT); check(edit.Call(SCI_GETSELECTIONS) == 2,L"add next occurrence creates a second editable selection");
        edit.SetWindowTextW(L"int main() {\r\n  return 1;\r\n}\r\n"); edit.ConfigureLanguage(L"test.cpp"); edit.Call(SCI_COLOURISE,0,-1); check((edit.Call(SCI_GETFOLDLEVEL,0) & SC_FOLDLEVELHEADERFLAG) != 0,L"C++ lexer supplies fold headers"); frame->OnEditorCommand(ID_VIEW_FOLD); check(!edit.Call(SCI_GETLINEVISIBLE,1),L"Fold All hides nested lines"); frame->OnEditorCommand(ID_VIEW_UNFOLD); check(edit.Call(SCI_GETLINEVISIBLE,1),L"Expand All restores nested lines");
        frame->OnEditorCommand(ID_VIEW_DARK); check(edit.Call(SCI_STYLEGETBACK,STYLE_DEFAULT) == RGB(63,63,63),L"dark theme updates editor styles");
        edit.SetReadOnly(TRUE); const auto before = edit.Utf8(); edit.ReplaceSel(L"blocked",TRUE); check(edit.Utf8() == before,L"native bridge respects read-only edits"); edit.SetReadOnly(FALSE);
        edit.SetWindowTextW(L"12345"); edit.SetLimitText(5); edit.SetSel(0,5); edit.ReplaceSel(L"abcde",TRUE); check(edit.Utf8() == "abcde",L"replacement limit subtracts replaced selection");
        edit.SetSel(99999,99999); edit.GetSel(start,end); check(start == 5 && end == 5,L"out-of-range UTF-16 selections clamp to EOF");
        frame->text_dirty_ = false; edit.SetModify(FALSE); frame->document_.Close(); frame->OpenPath(path.wstring()); frame->ShowPage(0); frame->hex_.SelectRange(0,4);
        check(frame->StartBinaryTool(ID_TOOL_INVERT,L"") && Wait(frame),L"Hex transformation runs through the product background job");
        auto bytes = Read(frame);
        if (bytes[0] != static_cast<std::uint8_t>('i' ^ 0xFF) || bytes[4] != 'm') { std::wcout << L"Transformation status: " << frame->status_.GetPaneText(0).GetString() << L"; bytes:"; for (std::size_t i = 0; i < (std::min<std::size_t>)(8,bytes.size()); ++i) std::wcout << L" " << static_cast<unsigned>(bytes[i]); std::wcout << L'\n'; }
        check(bytes[0] == static_cast<std::uint8_t>('i' ^ 0xFF) && bytes[4] == 'm',L"product transformation adopts only selected changed bytes"); frame->OnUndo(); check(Read(frame) == ReadDisk(path),L"product Undo restores a range operation");
        frame->hex_.SelectRange(4,4); check(frame->StartBinaryTool(ID_TOOL_EXTRACT,(root / L"selected.bin").wstring()) && Wait(frame) && ReadDisk(root / L"selected.bin") == std::vector<std::uint8_t>({'m','a','i','n'}),L"product extraction writes its selected range asynchronously");
        frame->OnHexCommand(ID_HEX_READ_ONLY); frame->hex_.SelectRange(0,4); check(!frame->StartBinaryTool(ID_TOOL_INVERT,L""),L"range operation cannot bypass user read-only"); frame->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr;
        return failures;
    }
    static int Run(const std::filesystem::path& root) {
        int failures = 0;
        const auto check = [&](bool condition, const wchar_t* name) { if (!condition) { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; } else std::wcout << L"PASS: " << name << L'\n'; };
        auto* frame = new CMainFrame(false);
        AfxGetApp()->m_pMainWnd = frame;
        frame->SetWindowPos(nullptr, 0, 0, 1200, 800, SWP_NOACTIVATE | SWP_NOZORDER);
        check(frame->GetSafeHwnd() && frame->hex_.GetSafeHwnd() && frame->horizontal_splitter_.GetSafeHwnd(), L"MFC frame and child lifecycle");
        check(!::GetPropW(frame->GetSafeHwnd(), L"SimpleEditorTool.Native.MainWindow"), L"test window has no production instance identity");
        const auto file = root / L"editor.txt";
        Write(file, {'a','b','c',' ','a','b','c'});
        check(frame->OpenPath(file.wstring()), L"open test-owned document");

        frame->ShowPage(1); frame->text_.SetSel(0, 3); frame->text_.ReplaceSel(L"xyz", TRUE);
        check(frame->text_.GetModify() != FALSE, L"actual edit control records modified text");
        frame->ShowPage(0);
        check(Read(frame) == std::vector<std::uint8_t>({'x','y','z',' ','a','b','c'}), L"Text to Hex commits current bytes");
        frame->hex_.GoTo(0); frame->hex_.SendMessage(WM_CHAR, '4'); frame->hex_.SendMessage(WM_CHAR, '1');
        frame->ShowPage(1); CString value; frame->text_.GetWindowTextW(value);
        check(value == L"Ayz abc", L"Hex to Text refreshes edited bytes");
        frame->OnSave();
        check(Wait(frame), L"background save completes");
        check(!frame->document_.IsModified() && !frame->text_.GetModify(), L"text and hex atomic save clears modified state");

        frame->hex_.GoTo(0); frame->ShowPage(0);
        SearchRequest request; request.hex = false; request.find = L"abc";
        frame->ExecuteSearch(request); check(Wait(frame), L"background Find completes");
        check(frame->match_offset_ == 4 && frame->hex_.CursorOffset() == 4, L"Find selects matching address");
        frame->OnFindNext(); check(Wait(frame) && frame->match_offset_ == 4, L"Find Next wraps at EOF");
        request.action = SearchAction::Replace; request.replacement = L"12"; frame->ExecuteSearch(request);
        check(Read(frame) == std::vector<std::uint8_t>({'A','y','z',' ','1','2'}), L"Replace applies selected match");
        frame->OnUndo(); check(Read(frame) == std::vector<std::uint8_t>({'A','y','z',' ','a','b','c'}), L"Undo single replacement");
        request.hex = true; request.find = L"61"; request.replacement = L"00 FF"; request.action = SearchAction::ReplaceAll;
        frame->ExecuteSearch(request); check(Wait(frame), L"Hex Replace All completes");
        check(Read(frame) == std::vector<std::uint8_t>({'A','y','z',' ',0,255,'b','c'}), L"Hex Replace All bytes and length");
        frame->OnUndo(); check(Read(frame) == std::vector<std::uint8_t>({'A','y','z',' ','a','b','c'}), L"one Undo restores entire Replace All");
        frame->OnRedo(); frame->OnUndo();

        frame->search_dialog_.Show(true, true, frame);
        frame->search_dialog_.SetDlgItemTextW(IDC_SEARCH_TEXT, L"79 7A");
        frame->search_dialog_.SendMessage(WM_COMMAND, IDOK);
        check(Wait(frame) && frame->hex_.CursorOffset() == 1, L"real Find dialog command dispatch");
        frame->search_dialog_.ShowWindow(SW_HIDE);

        frame->ShowPage(0); goto_frame_ = frame->GetSafeHwnd();
        const auto timer = ::SetTimer(nullptr, 0, 50, AddressTimer);
        frame->OnGoTo(); ::KillTimer(nullptr, timer);
        check(frame->hex_.CursorOffset() == 5, L"Go To dialog validates and navigates hex address");

        frame->OnSave(); check(Wait(frame), L"save completes before comparison"); const auto right = root / L"right.bin";
        Write(right, {'A','Y','z',' ','a','b','X','!'});
        frame->StartCompare(right.wstring(), false);
        check(Wait(frame) && frame->pending_differences_.size() == 3, L"background equal-address comparison UI result");
        check(frame->differences_.GetItemCount() == 3 && frame->difference_index_ == 0, L"virtual list and first difference selection");
        frame->OnNextDifference(); check(frame->compare_left_.CursorOffset() == 6 && frame->compare_right_.CursorOffset() == 6, L"F7 handler selects both addresses");
        frame->OnPreviousDifference(); check(frame->difference_index_ == 0, L"previous difference handler");
        frame->OnPreviousDifference(); check(frame->difference_index_ == 2 && frame->compare_right_.CursorOffset() == 7, L"previous wraps to added byte");
        frame->OnNextDifference(); check(frame->difference_index_ == 0, L"next wraps to first difference");
        NMLVDISPINFO info{}; wchar_t buffer[64]{}; info.item.mask = LVIF_TEXT; info.item.iItem = 1; info.item.iSubItem = 1; info.item.pszText = buffer; info.item.cchTextMax = 64; LRESULT result = 0;
        frame->OnDifferenceDisplay(&info.hdr, &result); check(std::wstring(buffer) == L"0x0000000000000006", L"virtual row formats exact address");

        const auto old_horizontal = frame->horizontal_ratio_;
        frame->vertical_splitter_.SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(2, 20));
        frame->vertical_splitter_.SendMessage(WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(110, 20));
        frame->vertical_splitter_.SendMessage(WM_LBUTTONUP, 0, MAKELPARAM(2, 20));
        check(frame->horizontal_ratio_ > old_horizontal, L"vertical splitter drag resizes left and right panes");
        const auto old_vertical = frame->vertical_ratio_;
        frame->horizontal_splitter_.SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(20, 2));
        frame->horizontal_splitter_.SendMessage(WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(20, 55));
        frame->horizontal_splitter_.SendMessage(WM_LBUTTONUP, 0, MAKELPARAM(20, 2));
        check(frame->vertical_ratio_ > old_vertical, L"horizontal splitter drag resizes panes and list");
        frame->ShowPage(0); frame->ShowPage(2); frame->SetWindowPos(nullptr, 0, 0, 800, 600, SWP_NOACTIVATE | SWP_NOZORDER);
        CRect left_bounds, right_bounds; frame->compare_left_.GetWindowRect(left_bounds); frame->compare_right_.GetWindowRect(right_bounds);
        check(left_bounds.right < right_bounds.left && left_bounds.Width() > 0 && right_bounds.Width() > 0, L"split ratios survive tab switches and resize");

        frame->ShowPage(0); frame->hex_.GoTo(0); frame->hex_.SendMessage(WM_CHAR, '4');
        check(frame->pending_differences_.empty() && frame->differences_.GetItemCount() == 0, L"editing invalidates old comparison addresses");
        frame->OnUndo(); frame->OnSave(); check(Wait(frame), L"save completes before switching fixtures");

        const auto long_file = root / L"scroll.bin";
        Write(long_file, std::vector<std::uint8_t>(1024 * 1024, 'A'));
        check(frame->OpenPath(long_file.wstring()), L"open scroll fixture");
        frame->StartCompare(long_file.wstring(), false); check(Wait(frame), L"large compare finishes");
        frame->compare_left_.SendMessage(WM_VSCROLL, SB_PAGEDOWN);
        check(frame->compare_left_.TopRow() > 0 && frame->compare_left_.TopRow() == frame->compare_right_.TopRow(), L"comparison scroll synchronization");
        frame->compare_right_.SendMessage(WM_KEYDOWN, VK_NEXT);
        check(frame->compare_left_.TopRow() == frame->compare_right_.TopRow(), L"keyboard scrolling also synchronizes");
        const auto many_file = root / L"million-differences.bin"; Write(many_file, std::vector<std::uint8_t>(1024 * 1024, 'B'));
        frame->StartCompare(many_file.wstring(), false);
        check(Wait(frame) && frame->differences_.GetItemCount() == 1000000, L"one million difference rows remain virtual");
        CString summary; frame->status_.GetPaneText(0, summary);
        check(summary.Find(L"first 1,000,000 shown") >= 0, L"truncated comparison is explicitly labelled");
        frame->NavigateDifference(999999);
        check(frame->compare_left_.CursorOffset() == 999999 && frame->compare_right_.CursorOffset() == 999999, L"navigate directly to millionth difference");
        frame->search_dialog_.Show(true, true, frame);
        MSG dialog_key{}; dialog_key.hwnd = frame->search_dialog_.GetDlgItem(IDC_SEARCH_TEXT)->GetSafeHwnd(); dialog_key.message = WM_KEYDOWN; dialog_key.wParam = VK_F7;
        frame->PreTranslateMessage(&dialog_key);
        check(frame->difference_index_ == 999999, L"search field keyboard input does not trigger document navigation");
        frame->search_dialog_.ShowWindow(SW_HIDE);
        request.find = L"41"; request.hex = true; request.action = SearchAction::ReplaceAll; request.replacement = L"42";
        frame->ExecuteSearch(request); frame->OnCompareCancel(); check(Wait(frame) && !frame->document_.IsModified(), L"cancelled Replace All does not apply edits");
        frame->hex_.GoTo(0); request.action = SearchAction::FindNext; frame->ExecuteSearch(request);
        frame->document_.SetByte(0, 'Z'); frame->DocumentChanged();
        check(Wait(frame) && frame->match_offset_ == UINT64_MAX, L"document revision rejects stale search result");
        frame->OnUndo();
        const auto big5_file = root / L"big5.txt"; Write(big5_file, {0xA4,0x41,0x41});
        check(frame->OpenPath(big5_file.wstring()), L"open DBCS boundary fixture");
        frame->OnEncoding(ID_ENCODING_BIG5); frame->hex_.GoTo(0);
        request.hex = false; request.find = L"A"; request.action = SearchAction::FindNext;
        frame->ExecuteSearch(request); check(Wait(frame) && frame->match_offset_ == 2, L"CP950 text search never matches inside a Chinese character");

        const auto reload_file = root / L"reload.txt", reload_replacement = root / L"reload-replacement.txt";
        Write(reload_file, {'A','B','C'}); check(frame->OpenPath(reload_file.wstring()), L"open reload fixture");
        frame->document_.SetByte(1, 'X'); frame->DocumentChanged(); Write(reload_replacement, {'N','E','W'});
        check(ReplaceFileW(reload_file.c_str(), reload_replacement.c_str(), nullptr, 0, nullptr, nullptr) != FALSE, L"replace active editor source externally");
        goto_frame_ = frame->GetSafeHwnd(); confirm_command_ = IDCANCEL;
        auto confirm_timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); frame->OnReload(); ::KillTimer(nullptr, confirm_timer);
        check(frame->document_.IsModified() && Read(frame) == std::vector<std::uint8_t>({'A','X','C'}), L"cancelled reload preserves unsaved editor bytes");
        confirm_command_ = IDNO;
        confirm_timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); frame->OnReload(); ::KillTimer(nullptr, confirm_timer);
        check(!frame->document_.IsModified() && frame->document_.ValidateSource(true) && Read(frame) == std::vector<std::uint8_t>({'N','E','W'}), L"confirmed reload opens external replacement and resets source state");
        frame->ShowPage(1); frame->text_.GetWindowTextW(value); check(value == L"NEW", L"Text page refreshes after reload");
        Write(right, {'X','E','W'}); frame->StartCompare(right.wstring(), false);
        check(Wait(frame) && frame->differences_.GetItemCount() == 1, L"comparison fixture before external update");
        Write(right, {'Z'}); frame->source_check_tick_ = 0; frame->OnTimer(1);
        frame->status_.GetPaneText(0, summary);
        check(frame->differences_.GetItemCount() == 0 && !frame->compare_right_document_.IsOpen() && summary.Find(L"externally") >= 0, L"external right-side update invalidates comparison and reports conflict");
        Write(reload_file, {'n','e','w','!'}); frame->source_check_tick_ = 0; frame->OnTimer(1);
        check(!frame->document_.ValidateSource(false), L"UI detects in-place source changes");
        frame->OnReload(); check(Read(frame) == std::vector<std::uint8_t>({'n','e','w','!'}), L"reload recovers from in-place source changes");
        frame->search_dialog_.DestroyWindow(); frame->KillTimer(1); frame->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr;
        return failures;
    }
    static int RunAsyncSave(const std::filesystem::path& root) {
        int failures = 0;
        const auto check = [&](bool condition, const wchar_t* name) { if (condition) std::wcout << L"PASS: " << name << L'\n'; else { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; } };
        auto* frame = new CMainFrame(false); AfxGetApp()->m_pMainWnd = frame;
        frame->SetWindowPos(nullptr, 0, 0, 1000, 700, SWP_NOACTIVATE | SWP_NOZORDER);
        const auto source = root / L"ui-async.bin", target = root / L"ui-async-as.bin", other = root / L"ui-other.txt", locked = root / L"ui-locked.bin";
        Write(source, std::vector<std::uint8_t>(4 * 1024 * 1024, 'A')); Write(other, {'N','E','W'});
        check(frame->OpenPath(source.wstring()), L"open async UI save fixture");
        frame->ShowPage(0);
        frame->document_.SetByte(0, 'Z'); frame->DocumentChanged(); frame->hex_.GoTo(123);
        check(frame->StartSave(source.wstring()) && frame->IsSaving(), L"Save starts a background job and returns to UI");
        const auto revision = frame->revision_;
        frame->hex_.SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(MK_CONTROL, WHEEL_DELTA), 0);
        check(frame->IsSaving() && frame->zoom_percent_[0] == 110 && frame->revision_ == revision && frame->hex_.CursorOffset() == 123,
              L"Ctrl wheel zoom remains available during save without changing the snapshot or caret");
        frame->hex_.SendMessage(WM_CHAR, 'F'); frame->OnInsert(); frame->OnDelete(); frame->OnUndo(); frame->OnRedo();
        frame->ShowPage(1); frame->text_.SetSel(0, 0); frame->text_.SendMessage(WM_CHAR, 'X'); frame->OnEncoding(ID_ENCODING_BIG5);
        check(frame->revision_ == revision && (frame->text_.GetStyle() & ES_READONLY) && !frame->text_.GetModify(), L"Hex/text/edit commands stay read-only during save");
        frame->Layout(); frame->ShowPage(0); frame->hex_.SendMessage(WM_VSCROLL, SB_PAGEDOWN);
        check(frame->hex_.TopRow() > 0, L"scroll and layout still work during save");
        check(Wait(frame) && !frame->document_.IsModified() && !(frame->text_.GetStyle() & ES_READONLY), L"successful save adopts clean document and restores editing");
        check(frame->hex_.CursorOffset() == 123 && ReadDisk(source)[0] == 'Z', L"save preserves caret and writes edited bytes");

        frame->document_.SetByte(1, 'Y'); frame->DocumentChanged();
        check(frame->StartSave(target.wstring()) && Wait(frame) && frame->document_.Path() == target.wstring(), L"background Save As adopts destination");
        check(ReadDisk(source)[1] == 'A' && ReadDisk(target)[1] == 'Y', L"Save As leaves original source unchanged");
        Write(locked, {'O','L','D'});
        HANDLE file_lock = CreateFileW(locked.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        frame->document_.SetByte(2, 'X'); frame->DocumentChanged(); frame->StartSave(locked.wstring()); frame->OnClose();
        check(Wait(frame) && ::IsWindow(frame->GetSafeHwnd()) && frame->document_.IsModified() && frame->document_.Path() == target.wstring(), L"failed save cancels deferred close and retains document");
        check(ReadDisk(locked) == std::vector<std::uint8_t>({'O','L','D'}), L"failed background save preserves locked destination");
        frame->OnUndo(); frame->OnRedo(); CloseHandle(file_lock);
        frame->OnSave(); check(Wait(frame) && !frame->document_.IsModified(), L"Undo/Redo and retry work after failed background save");

        frame->document_.SetByte(3, 'K'); frame->DocumentChanged(); frame->StartSave(target.wstring()); frame->OnClose(); frame->OnCompareCancel();
        check(Wait(frame) && ::IsWindow(frame->GetSafeHwnd()) && frame->after_save_ == CMainFrame::AfterSave::None, L"cancel request clears deferred close and keeps window open");
        const auto cancellation_disk = ReadDisk(target);
        check(frame->document_.IsModified() ? cancellation_disk[3] != 'K' : cancellation_disk[3] == 'K', L"cancel-or-commit race reports a state matching disk bytes");
        if (frame->document_.IsModified()) frame->OnUndo();

        frame->document_.SetByte(4, 'N'); frame->DocumentChanged(); goto_frame_ = frame->GetSafeHwnd(); confirm_command_ = IDYES;
        auto timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); frame->OpenPath(other.wstring()); ::KillTimer(nullptr, timer);
        check(frame->IsSaving() && frame->document_.Path() == target.wstring(), L"Open waits for unsaved document to finish saving");
        check(Wait(frame) && frame->document_.Path() == other.wstring() && ReadDisk(target)[4] == 'N', L"deferred Open runs only after successful save");
        frame->document_.SetByte(0, 'Q'); frame->DocumentChanged(); frame->StartSave(other.wstring()); frame->OpenPath(source.wstring());
        check(frame->document_.Path() == other.wstring(), L"open request during save does not replace active document early");
        check(Wait(frame) && frame->document_.Path() == source.wstring() && ReadDisk(other)[0] == 'Q', L"open request resumes after the snapshot is committed");

        file_lock = CreateFileW(locked.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        frame->document_.SetByte(0, 'F'); frame->DocumentChanged(); frame->StartSave(locked.wstring()); frame->OpenPath(other.wstring());
        check(Wait(frame) && frame->document_.Path() == source.wstring() && frame->document_.IsModified(), L"failed save cancels deferred Open");
        CloseHandle(file_lock); frame->OnSave(); check(Wait(frame), L"retry source save after deferred Open failure");

        frame->document_.SetByte(5, 'L'); frame->DocumentChanged(); frame->StartSave(source.wstring()); frame->OnClose();
        const bool committed = frame->job_.wait_for(std::chrono::seconds(15)) == std::future_status::ready;
        check(committed, L"wait for completed snapshot before testing late cancellation");
        frame->OnCompareCancel(); check(Wait(frame) && !frame->document_.IsModified() && ReadDisk(source)[5] == 'L', L"late cancellation still adopts the committed save");
        check(::IsWindow(frame->GetSafeHwnd()), L"late cancellation cancels pending close without undoing committed file");

        frame->document_.SetByte(7, 'B'); frame->DocumentChanged(); confirm_command_ = IDYES;
        timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); frame->OnCompareEqual(); ::KillTimer(nullptr, timer);
        check(frame->IsSaving() && frame->after_save_ == CMainFrame::AfterSave::CompareEqual, L"comparison queues the requested mode while saving edits");
        saw_picker_ = false; timer = ::SetTimer(nullptr, 0, 50, PickerCancelTimer);
        check(Wait(frame), L"save-before-compare completes and returns from picker"); ::KillTimer(nullptr, timer);
        check(saw_picker_ && !frame->document_.IsModified() && ReadDisk(source)[7] == 'B', L"comparison file picker appears only after successful save");
        frame->StartSave(source.wstring()); frame->OnFileOpen(); saw_picker_ = false;
        timer = ::SetTimer(nullptr, 0, 50, PickerCancelTimer);
        check(Wait(frame), L"queued Open dialog completes after save"); ::KillTimer(nullptr, timer);
        check(saw_picker_ && !frame->IsBusy(), L"Open during save defers the modal picker safely");
        frame->document_.SetByte(8, 'R'); frame->DocumentChanged(); frame->StartSave(source.wstring()); frame->OnReload();
        check(Wait(frame) && Read(frame)[8] == 'R' && frame->document_.ValidateSource(true), L"Reload during save reopens committed bytes");

        frame->document_.SetByte(6, 'C'); frame->DocumentChanged(); confirm_command_ = IDCANCEL;
        timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); frame->OnClose(); ::KillTimer(nullptr, timer);
        check(!frame->IsBusy() && frame->document_.IsModified(), L"Cancel in unsaved-close prompt starts no save and keeps edits");
        const HWND closing = frame->GetSafeHwnd(); confirm_command_ = IDYES;
        timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); frame->OnClose(); ::KillTimer(nullptr, timer);
        check(frame->IsSaving() && ::IsWindow(closing), L"Save-and-close keeps window alive while save runs");
        check(WaitClosed(frame, closing) && ReadDisk(source)[6] == 'C', L"Save-and-close commits bytes before safe MFC teardown");
        AfxGetApp()->m_pMainWnd = nullptr;
        return failures;
    }
    static int RunZoom(const std::filesystem::path& root) {
        int failures = 0;
        const auto check = [&](bool condition, const wchar_t* name) { if (!condition) { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; } else std::wcout << L"PASS: " << name << L'\n'; };
        auto* frame = new CMainFrame(false); AfxGetApp()->m_pMainWnd = frame;
        frame->SetWindowPos(nullptr, 0, 0, 1200, 800, SWP_NOACTIVATE | SWP_NOZORDER);
        const auto wheel = [](CWnd& control, short delta, UINT flags = MK_CONTROL) {
            control.SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(flags, static_cast<WORD>(delta)), 0);
        };
        const auto status = [&]() { CString text; frame->status_.GetPaneText(1, text); return text; };
        std::vector<std::uint8_t> bytes;
        const std::string line = "0123456789 ABCDEF abcdef\r\n";
        for (int i = 0; i < 500; ++i) bytes.insert(bytes.end(), line.begin(), line.end());
        const auto file = root / L"zoom.txt", right = root / L"zoom-right.txt";
        Write(file, bytes); check(frame->OpenPath(file.wstring()), L"open zoom fixture"); frame->ShowPage(0);
        check(status() == L"Zoom: 100%", L"initial zoom percentage is visible");
        frame->status_.OnUpdateCmdUI(frame, TRUE);
        check(!(frame->status_.GetPaneStyle(1) & SBPS_DISABLED), L"idle command updates keep the zoom indicator enabled");
        wheel(frame->hex_, 30); wheel(frame->hex_, 30);
        check(status() == L"Zoom: 100%", L"partial wheel deltas accumulate without overshooting");
        wheel(frame->hex_, 60);
        check(status() == L"Zoom: 110%", L"one full wheel notch zooms Hex by ten percent");
        wheel(frame->hex_, -120);
        check(status() == L"Zoom: 100%", L"reverse wheel restores zoom");
        CPoint hover = frame->page_rect_.CenterPoint(); frame->ClientToScreen(&hover);
        MSG queued{}; queued.hwnd = frame->tabs_.GetSafeHwnd(); queued.message = WM_MOUSEWHEEL;
        queued.wParam = MAKEWPARAM(0, WHEEL_DELTA); queued.lParam = MAKELPARAM(hover.x, hover.y);
        check(!frame->PreTranslateMessage(&queued) && status() == L"Zoom: 100%", L"ordinary wheel over content is not intercepted as zoom by the frame");
        queued.wParam = MAKEWPARAM(MK_CONTROL, WHEEL_DELTA);
        check(frame->PreTranslateMessage(&queued) && status() == L"Zoom: 110%", L"Ctrl wheel over content zooms even when focus remains on the tab strip");
        frame->hex_.SetTopRow(20); frame->hex_.GoTo(400);
        const auto cursor = frame->hex_.CursorOffset(), top = frame->hex_.TopRow();
        check(frame->SetTabZoom(0, 200) && frame->hex_.CursorOffset() == cursor && frame->hex_.TopRow() == top,
              L"Hex zoom preserves caret and viewport address");
        TEXTMETRIC metrics{};
        {
            CClientDC dc(&frame->hex_); CFont expected; expected.CreatePointFont(200, L"Consolas", &dc);
            auto* old_font = dc.SelectObject(&expected); dc.GetTextMetrics(&metrics); dc.SelectObject(old_font);
        }
        const int x = 8 + 19 * metrics.tmAveCharWidth + 1;
        const int y = 2 * (metrics.tmHeight + 6) + 1;
        frame->hex_.SendMessage(WM_LBUTTONDOWN, 0, MAKELPARAM(x, y));
        check(frame->hex_.CursorOffset() == (top + 2) * 16 + 3, L"zoomed Hex hit testing selects the painted byte"); frame->hex_.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(x,y));
        const auto ordinary_hex = frame->hex_.TopRow(); wheel(frame->hex_, -120, 0);
        check(frame->hex_.TopRow() > ordinary_hex && status() == L"Zoom: 200%", L"ordinary wheel scrolls Hex without changing the zoom percentage");
        const auto scroll_before = frame->hex_.TopRow(); wheel(frame->hex_, -120, MK_SHIFT);
        check(frame->hex_.TopRow() > scroll_before && status() == L"Zoom: 200%", L"Shift wheel scrolls Hex without zooming");
        check(!frame->document_.IsModified() && Read(frame) == bytes, L"Hex zoom never changes file bytes or dirty state");
        frame->hex_.SendMessage(WM_VSCROLL,SB_BOTTOM); const auto bottom = frame->hex_.TopRow();
        unsigned notifications = 0; frame->hex_.SetScrollCallback([&](std::uint64_t) { ++notifications; }); frame->hex_.ValidateRect(nullptr);
        for (int i = 0; i < 30; ++i) wheel(frame->hex_,-120,0);
        frame->hex_.SendMessage(WM_VSCROLL,SB_LINEDOWN); frame->hex_.SendMessage(WM_VSCROLL,SB_PAGEDOWN); frame->hex_.SendMessage(WM_VSCROLL,SB_ENDSCROLL);
        check(frame->hex_.TopRow() == bottom && !frame->hex_.GetUpdateRect(nullptr,FALSE) && !notifications,L"repeated downward scrolling at EOF requests no repaint or scroll callback");
        wheel(frame->hex_,120,0);
        // Hidden native fixtures need not retain a client update region.
        check(frame->hex_.TopRow() < bottom && (!frame->hex_.IsWindowVisible() || frame->hex_.GetUpdateRect(nullptr,FALSE)) && notifications > 0 && status() == L"Zoom: 200%",L"scrolling away from EOF still updates the viewport without changing zoom");
        frame->hex_.SendMessage(WM_VSCROLL,SB_TOP); notifications = 0; frame->hex_.ValidateRect(nullptr);
        for (int i = 0; i < 30; ++i) wheel(frame->hex_,120,0);
        check(frame->hex_.TopRow() == 0 && !frame->hex_.GetUpdateRect(nullptr,FALSE) && !notifications,L"repeated upward scrolling at the first row also requests no repaint");
        frame->hex_.SetScrollCallback({});

        frame->ShowPage(1); check(status() == L"Zoom: 100%", L"Text starts with its own zoom percentage");
        frame->text_.SetSel(0, 3); frame->text_.ReplaceSel(L"xyz", TRUE);
        frame->text_.SetSel(1, 4, TRUE); frame->text_.LineScroll(40);
        CString draft; frame->text_.GetWindowTextW(draft);
        const auto revision = frame->revision_; const int first_line = frame->text_.GetFirstVisibleLine();
        LOGFONT before_font{}; frame->text_.GetFont()->GetLogFont(&before_font);
        wheel(frame->text_, 120);
        LOGFONT after_font{}; frame->text_.GetFont()->GetLogFont(&after_font);
        int start = 0, end = 0; frame->text_.GetSel(start, end); CString after; frame->text_.GetWindowTextW(after);
        check(status() == L"Zoom: 110%" && abs(after_font.lfHeight) > abs(before_font.lfHeight), L"Text wheel changes actual font and percentage");
        check(draft == after && frame->text_.GetModify() && frame->revision_ == revision && start == 1 && end == 4 &&
              frame->text_.GetFirstVisibleLine() == first_line, L"Text zoom preserves draft, selection, scroll and revision");
        frame->text_.Undo(); frame->text_.GetWindowTextW(after);
        check(after.Left(3) == L"012", L"Text Undo history survives font replacement");
        const auto ordinary_text = frame->text_.GetFirstVisibleLine(); wheel(frame->text_, -120, 0);
        check(frame->text_.GetFirstVisibleLine() > ordinary_text && status() == L"Zoom: 110%", L"ordinary wheel scrolls native Text without changing zoom");
        const auto text_scroll = frame->text_.GetFirstVisibleLine(); wheel(frame->text_, -120, MK_SHIFT);
        check(frame->text_.GetFirstVisibleLine() > text_scroll && status() == L"Zoom: 110%", L"Shift wheel retains native Text scrolling");
        frame->ToggleTextClone(); frame->text_clone_.Call(SCI_SETFIRSTVISIBLELINE,0);
        wheel(frame->text_clone_, -120, 0);
        check(frame->text_clone_.GetFirstVisibleLine() > 0 && status() == L"Zoom: 110%", L"ordinary wheel scrolls the shared Text view without zooming");
        wheel(frame->text_clone_, 120);
        check(status() == L"Zoom: 120%", L"Ctrl wheel in the shared Text view updates the owning tab percentage");
        frame->SetTabZoom(1,110); frame->ToggleTextClone();
        frame->ShowPage(0); check(status() == L"Zoom: 200%", L"switching tabs restores that tab's percentage");
        frame->ShowPage(1); check(status() == L"Zoom: 110%", L"Text percentage survives tab switches");

        frame->ShowPage(0); Write(right, std::vector<std::uint8_t>(bytes.size(), 'X'));
        frame->StartCompare(right.wstring(), false);
        check(Wait(frame) && frame->differences_.GetItemCount() > 100, L"comparison zoom fixture completes");
        const int count = frame->differences_.GetItemCount(), selected = frame->differences_.GetNextItem(-1, LVNI_SELECTED);
        wheel(frame->compare_left_, 120); wheel(frame->compare_right_, 120);
        check(status() == L"Zoom: 120%", L"either comparison pane changes the shared Compare zoom");
        const int column_width = frame->differences_.GetColumnWidth(1);
        wheel(frame->differences_, 240);
        check(status() == L"Zoom: 140%" && frame->differences_.GetColumnWidth(1) > column_width,
              L"difference list wheel zooms the page and widens address columns");
        check(frame->differences_.GetItemCount() == count && frame->differences_.GetNextItem(-1, LVNI_SELECTED) == selected,
              L"Compare zoom preserves differences and selected address");
        frame->compare_left_.SendMessage(WM_VSCROLL, SB_PAGEDOWN);
        check(frame->compare_left_.TopRow() == frame->compare_right_.TopRow(), L"comparison scrolling stays synchronized after zoom");
        const auto ordinary_compare = frame->compare_left_.TopRow(); wheel(frame->compare_left_, -120, 0);
        check(frame->compare_left_.TopRow() > ordinary_compare && frame->compare_left_.TopRow() == frame->compare_right_.TopRow() && status() == L"Zoom: 140%", L"ordinary wheel scrolls synchronized comparison panes without zooming");
        frame->compare_left_.SendMessage(WM_VSCROLL,SB_BOTTOM); const auto compare_bottom = frame->compare_left_.TopRow();
        frame->compare_left_.ValidateRect(nullptr); frame->compare_right_.ValidateRect(nullptr);
        for (int i = 0; i < 30; ++i) wheel(frame->compare_left_,-120,0);
        check(frame->compare_left_.TopRow() == compare_bottom && frame->compare_left_.TopRow() == frame->compare_right_.TopRow() && !frame->compare_left_.GetUpdateRect(nullptr,FALSE) && !frame->compare_right_.GetUpdateRect(nullptr,FALSE),L"EOF scrolling does not invalidate either synchronized Hex comparison pane");
        const int ordinary_list = frame->differences_.GetTopIndex(); wheel(frame->differences_, -120, 0);
        check(frame->differences_.GetTopIndex() > ordinary_list && status() == L"Zoom: 140%", L"ordinary wheel scrolls difference results without zooming");
        const int list_top = frame->differences_.GetTopIndex(); wheel(frame->differences_, -120, MK_SHIFT);
        check(frame->differences_.GetTopIndex() > list_top && status() == L"Zoom: 140%", L"Shift wheel retains difference-list scrolling");
        frame->status_.SetPaneText(0, L"Operation progress"); wheel(frame->compare_right_, 120);
        CString progress; frame->status_.GetPaneText(0, progress);
        check(progress == L"Operation progress" && status() == L"Zoom: 150%", L"zoom percentage and operation status remain independent");
        for (int tab = 0; tab < 3; ++tab) {
            frame->ShowPage(tab); frame->SetTabZoom(tab, 50);
            CWnd& content = tab == 0 ? static_cast<CWnd&>(frame->hex_) : tab == 1 ? static_cast<CWnd&>(frame->text_) : static_cast<CWnd&>(frame->differences_);
            wheel(content, -120); check(status() == L"Zoom: 50%", L"zoom lower bound is fifty percent");
            frame->SetTabZoom(tab, 300); wheel(content, 120); check(status() == L"Zoom: 300%", L"zoom upper bound is three hundred percent");
        }
        BYTE keys[256]{}, saved_keys[256]{}; GetKeyboardState(saved_keys); memcpy(keys, saved_keys, sizeof(keys)); keys[VK_CONTROL] = 0x80;
        SetKeyboardState(keys); MSG reset{}; reset.hwnd = frame->differences_.GetSafeHwnd(); reset.message = WM_KEYDOWN; reset.wParam = '0';
        frame->PreTranslateMessage(&reset); SetKeyboardState(saved_keys);
        check(status() == L"Zoom: 100%", L"Ctrl zero resets the active tab to one hundred percent");
        const auto gdi_before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        for (int i = 0; i < 30; ++i) {
            for (int tab = 0; tab < 3; ++tab) { frame->ShowPage(tab); frame->SetTabZoom(tab, i % 2 ? 50 : 300); frame->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN); }
            frame->SetWindowPos(nullptr, 0, 0, i % 2 ? 640 : 1200, i % 2 ? 400 : 800, SWP_NOACTIVATE | SWP_NOZORDER);
        }
        const auto gdi_after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        check(gdi_after <= gdi_before + 4, L"repeated zoom, repaint, resize and tab switching do not leak fonts");
        LOGFONT valid_font{};
        check(frame->text_.GetFont()->GetLogFont(&valid_font) && frame->differences_.GetFont()->GetLogFont(&valid_font),
              L"controls keep live borrowed font handles after repeated zoom");
        frame->SetWindowPos(nullptr,0,0,1200,800,SWP_NOACTIVATE | SWP_NOZORDER);
        frame->compare_mode_.SetCurSel(2); frame->OnCompareModeChanged();
        frame->SetComparisonPaths(file.wstring(),right.wstring(),true); frame->OnCompareRun();
        check(Wait(frame) && frame->advanced_.rows_.GetItemCount() > 100, L"text comparison provides scrollable native results for wheel routing");
        auto& advanced = frame->advanced_; const int comparison_zoom = frame->zoom_percent_[2];
        const int advanced_top = advanced.rows_.GetTopIndex(); wheel(advanced.rows_, -120, 0);
        check(advanced.rows_.GetTopIndex() > advanced_top && frame->zoom_percent_[2] == comparison_zoom, L"ordinary wheel scrolls advanced comparison results without zooming");
        const int output_top = advanced.output_.GetFirstVisibleLine(); wheel(advanced.output_, -120, 0);
        check(advanced.output_.GetFirstVisibleLine() > output_top && frame->zoom_percent_[2] == comparison_zoom, L"ordinary wheel scrolls merge output without zooming");
        wheel(advanced.rows_,120); wheel(advanced.output_,120);
        check(frame->zoom_percent_[2] == comparison_zoom + 20, L"Ctrl wheel on advanced results and merge output uses the shared percentage");
        std::wstring pager_error; CTextPager pager(frame,frame->document_.Snapshot(&pager_error),simple_editor::core::TextEncoding::Utf8); pager.Create(IDD_TEXT_PAGER,frame);
        const auto pager_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (pager.job_.valid() && std::chrono::steady_clock::now() < pager_deadline) { pager.Poll(); ::Sleep(1); }
        MSG pager_wheel{}; pager_wheel.hwnd = pager.editor_.GetSafeHwnd(); pager_wheel.message = WM_MOUSEWHEEL; pager_wheel.wParam = MAKEWPARAM(0,static_cast<WORD>(-WHEEL_DELTA));
        const auto page_text = pager.editor_.Utf8(); const int page_top = pager.editor_.GetFirstVisibleLine(); const bool consumed = pager.PreTranslateMessage(&pager_wheel);
        if (!consumed) pager.editor_.SendMessage(pager_wheel.message,pager_wheel.wParam,pager_wheel.lParam);
        check(!pager.job_.valid() && !consumed && pager.editor_.GetFirstVisibleLine() > page_top && pager.zoom_ == 100, L"ordinary wheel scrolls the read-only text pager without zooming");
        pager_wheel.wParam = MAKEWPARAM(MK_CONTROL,WHEEL_DELTA); const int page_scroll = pager.editor_.GetFirstVisibleLine();
        check(pager.PreTranslateMessage(&pager_wheel) && pager.zoom_ == 110 && pager.editor_.GetFirstVisibleLine() == page_scroll && pager.editor_.Utf8() == page_text, L"Ctrl wheel zooms the text pager without scrolling or replacing its buffer");
        pager.DestroyWindow();
        frame->KillTimer(1); frame->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr;
        return failures;
    }
    static int RunWorkspace(const std::filesystem::path& root) {
        int failures = 0;
        const auto check = [&](bool condition, const wchar_t* name) { if (!condition) { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; } else std::wcout << L"PASS: " << name << L'\n'; };
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        workspace->SetWindowPos(nullptr, 0, 0, 1250, 850, SWP_NOACTIVATE | SWP_NOZORDER);
        check(workspace->GetSafeHwnd() && !::GetPropW(workspace->GetSafeHwnd(), L"SimpleEditorTool.Native.MainWindow"), L"workspace fixture has no production instance identity");
        const auto first = root / L"first.txt", second = root / L"第二份 file.txt", third = root / L"third.bin";
        Write(first, {'A','B','C','\r','\n','D','E','F'}); Write(second, {'B','B','C','\r','\n','D','E','F'}); Write(third, {1,2,3});
        check(workspace->OpenPath(first.wstring()) && workspace->sessions_.size() == 1, L"first file creates an independent editor session");
        auto* editor = workspace->sessions_[0]; editor->ShowPage(1); editor->text_.SetSel(0, 3); editor->text_.ReplaceSel(L"XYZ", TRUE);
        editor->text_.SetSel(1, 2, TRUE); editor->SetTabZoom(1, 150);
        const auto draft_revision = editor->revision_; CString draft; editor->text_.GetWindowTextW(draft);
        BYTE root_keys[256]{}, root_saved_keys[256]{}; GetKeyboardState(root_saved_keys); memcpy(root_keys, root_saved_keys, sizeof(root_keys)); root_keys[VK_CONTROL] = 0x80; SetKeyboardState(root_keys);
        MSG toolbar_reset{}; toolbar_reset.hwnd = workspace->open_button_.GetSafeHwnd(); toolbar_reset.message = WM_KEYDOWN; toolbar_reset.wParam = '0';
        workspace->PreTranslateMessage(&toolbar_reset); SetKeyboardState(root_saved_keys);
        check(editor->zoom_percent_[1] == 100, L"editor shortcuts route from workspace toolbar focus to the active file"); editor->SetTabZoom(1, 150);
        check(editor->text_.GetModify() && editor->text_.CanUndo(), L"original editor has an unsaved draft and native Undo before dropping another file");
        workspace->OpenDroppedFiles(MakeDrop({second.wstring(), third.wstring()}));
        check(workspace->sessions_.size() == 3 && workspace->sessions_[0] == editor, L"one multi-file drop retains the old file and opens every new file");
        auto* right_editor = workspace->sessions_[1];
        workspace->ActivateSession(0); CString retained; editor->text_.GetWindowTextW(retained); int selection_start = 0, selection_end = 0; editor->text_.GetSel(selection_start, selection_end);
        check(retained == draft && editor->text_.GetModify() && editor->text_.CanUndo() && editor->revision_ == draft_revision &&
              editor->zoom_percent_[1] == 150 && selection_start == 1 && selection_end == 2, L"tab switching preserves draft, selection, Undo, zoom and document identity");
        check(workspace->OpenPath((root / L"." / L"first.txt").wstring()) && workspace->sessions_.size() == 3 && workspace->active_session_ == 0,
              L"reopening an existing path activates its tab without replacing its draft");
        workspace->closing_all_ = true; workspace->CloseSession(workspace->sessions_[2]);
        workspace->OpenPath(third.wstring()); workspace->OnContinueClose(0, 0);
        check(!workspace->closing_all_ && workspace->sessions_.size() == 3 && editor->text_.GetModify(),
              L"a new open request cancels a queued workspace exit and retains existing drafts");
        goto_frame_ = workspace->GetSafeHwnd(); const auto error_timer = ::SetTimer(nullptr, 0, 50, ErrorTimer);
        const bool missing = workspace->OpenPath((root / L"not-found.bin").wstring()); ::KillTimer(nullptr, error_timer);
        check(!missing && workspace->sessions_.size() == 3 && editor->text_.GetModify(), L"failed open preserves every existing session and unsaved draft");
        workspace->ActivateSession(1); right_editor->hex_.GoTo(0); right_editor->hex_.SendMessage(WM_CHAR, '5'); right_editor->hex_.SendMessage(WM_CHAR, '2');
        check(Read(right_editor)[0] == 'R' && right_editor->document_.IsModified(), L"right source has independent unsaved Hex edits");
        workspace->SendMessage(WM_COMMAND, ID_WORKSPACE_COMPARE); auto* compare = workspace->sessions_.back();
        check(workspace->sessions_.size() == 4 && compare->comparison_session_ && compare->active_tab_ == 2 && compare->compare_left_path_.GetCount() == 3,
              L"comparison has a separate session and a dropdown of open file paths");
        compare->SetComparisonPaths(first.wstring(), second.wstring(), false);
        saw_picker_ = false; auto picker_timer = ::SetTimer(nullptr, 0, 50, PickerCancelTimer); compare->OnCompareBrowseLeft(); ::KillTimer(nullptr, picker_timer);
        CString selected_left; compare->compare_left_path_.GetWindowTextW(selected_left);
        check(saw_picker_ && selected_left == first.wstring().c_str(), L"left Browse opens the native file explorer and Cancel preserves the path");
        saw_picker_ = false; picker_timer = ::SetTimer(nullptr, 0, 50, PickerCancelTimer); compare->OnCompareBrowseRight(); ::KillTimer(nullptr, picker_timer);
        CString selected_right; compare->compare_right_path_.GetWindowTextW(selected_right);
        check(saw_picker_ && selected_right == second.wstring().c_str(), L"right Browse opens the native file explorer and Cancel preserves the path");
        compare->OnCompareRun(); check(Wait(compare) && !compare->pending_differences_.empty(), L"path-based comparison completes without replacing source tabs");
        std::vector<std::uint8_t> left_bytes, right_bytes; compare->compare_left_document_.Read(0, 3, &left_bytes); compare->compare_right_document_.Read(0, 3, &right_bytes);
        check(left_bytes == std::vector<std::uint8_t>({'X','Y','Z'}) && !right_bytes.empty() && right_bytes[0] == 'R', L"comparison reads unsaved Text and Hex source snapshots rather than stale disk bytes");
        editor->text_.GetWindowTextW(retained);
        check(editor->text_.CanUndo() && editor->text_.GetModify() && retained == draft && Read(editor)[0] == 'A', L"snapshot comparison does not commit or replace the source Text draft and Undo");
        const int retained_count = compare->differences_.GetItemCount();
        Write(root / L"recent.txt", {'r'}); workspace->OpenPath((root / L"recent.txt").wstring()); auto* recent = workspace->sessions_.back();
        recent->search_dialog_.Create(IDD_SEARCH, recent); const HWND owned_search = recent->search_dialog_.GetSafeHwnd();
        workspace->ActivateSession(3);
        check(compare->compare_left_path_.GetCount() == 4 && compare->differences_.GetItemCount() == retained_count,
              L"returning to a comparison refreshes open paths without losing its result");
        workspace->CloseSession(recent); check(owned_search && !::IsWindow(owned_search), L"closing a file releases its modeless search dialog safely");
        const auto compare_revision = compare->revision_; compare->hex_.GoTo(0); compare->hex_.SendMessage(WM_CHAR, '0'); compare->OnInsert();
        SearchRequest replace; replace.hex = true; replace.action = SearchAction::ReplaceAll; replace.find = L"58"; replace.replacement = L"FF"; compare->ExecuteSearch(replace);
        check(compare->revision_ == compare_revision && !compare->IsBusy(), L"comparison snapshots reject editing and replacement commands");
        compare->OnCompareSwap(); compare->compare_left_path_.GetWindowTextW(selected_left); compare->compare_right_path_.GetWindowTextW(selected_right);
        check(selected_left == second.wstring().c_str() && selected_right == first.wstring().c_str() && compare->differences_.GetItemCount() == 0,
              L"Swap exchanges both paths and removes stale differences");
        compare->OnCompareRun(); check(Wait(compare) && compare->pending_differences_[0].left_byte == 'R' && compare->pending_differences_[0].right_byte == 'X', L"recomparison after Swap uses the new direction");
        check(workspace->CloseSession(compare) && workspace->sessions_.size() == 3 && editor->text_.GetModify() && right_editor->document_.IsModified(),
              L"closing only the comparison keeps both unsaved source files open");

        workspace->ActivateSession(0); workspace->SendMessage(WM_COMMAND, ID_FILE_SAVE); const HWND editor_window = editor->GetSafeHwnd();
        check(editor->IsSaving(), L"original session owns the background save before opening another file");
        // Open an actual file while the original save is still owned by its session.
        Write(root / L"fourth.txt", {'4'}); workspace->OpenPath((root / L"fourth.txt").wstring());
        check(workspace->sessions_.size() == 4 && ::IsWindow(editor_window) && Wait(editor) && ReadDisk(first)[0] == 'X',
              L"opening another tab during Save preserves the original save destination and result");
        editor->ShowPage(0); editor->document_.SetByte(0, 'Q'); editor->DocumentChanged(); confirm_command_ = IDYES;
        auto reopen_timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); workspace->CloseSession(editor); ::KillTimer(nullptr, reopen_timer);
        workspace->OpenPath(first.wstring());
        check(Wait(editor) && ::IsWindow(editor_window) && workspace->sessions_.size() == 4 && ReadDisk(first)[0] == 'Q',
              L"reopening a tab cancels its pending save-and-close without cancelling the save");
        editor->saved_close_pending_ = true; editor->PostMessage(WM_EDITOR_SAVED_CLOSE); workspace->OpenPath(first.wstring()); editor->OnSavedClose(0, 0);
        check(::IsWindow(editor_window) && workspace->sessions_.size() == 4, L"reopening a tab also invalidates an already-queued saved-close message");
        auto* retained_active = workspace->sessions_[workspace->active_session_];
        workspace->CloseSession(workspace->sessions_[2]);
        check(workspace->sessions_[workspace->active_session_] == retained_active, L"closing an inactive tab preserves the file the user is currently editing");
        workspace->OpenPath(third.wstring());
        workspace->ActivateSession(1); confirm_command_ = IDCANCEL;
        auto confirm_timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); const bool cancelled = workspace->CloseSession(right_editor); ::KillTimer(nullptr, confirm_timer);
        check(!cancelled && workspace->sessions_.size() == 4 && right_editor->document_.IsModified(), L"Cancel on Close retains that file and all other tabs");
        HANDLE locked = CreateFileW(second.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        confirm_command_ = IDYES; confirm_timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); workspace->OnClose(); ::KillTimer(nullptr, confirm_timer);
        check(Wait(right_editor) && !workspace->closing_all_ && workspace->sessions_.size() == 4 && right_editor->document_.IsModified(),
              L"save failure aborts workspace exit and retains all tabs and edits");
        CloseHandle(locked); confirm_command_ = IDNO;
        confirm_timer = ::SetTimer(nullptr, 0, 50, ConfirmationTimer); const bool closed = workspace->CloseSession(right_editor); ::KillTimer(nullptr, confirm_timer);
        check(closed && workspace->sessions_.size() == 3 && workspace->sessions_[0] == editor, L"explicit discard closes only the chosen file");

        std::vector<wchar_t> forwarded;
        for (const auto& path : {second.wstring(), third.wstring()}) forwarded.insert(forwarded.end(), path.c_str(), path.c_str() + path.size() + 1);
        forwarded.push_back(L'\0'); COPYDATASTRUCT data{2, static_cast<DWORD>(forwarded.size() * sizeof(wchar_t)), forwarded.data()};
        check(workspace->OnCopyData(nullptr, &data), L"multi-path instance forwarding accepts a bounded payload");
        while (!workspace->queued_paths_.empty()) workspace->OnQueuedOpen(0, 0);
        check(workspace->sessions_.size() == 4 && editor->document_.Path() == first.wstring(), L"forwarded paths append or activate tabs without replacing the original");
        wchar_t invalid = L'x'; COPYDATASTRUCT malformed{2, sizeof(invalid), &invalid};
        check(!workspace->OnCopyData(nullptr, &malformed), L"unterminated forwarded paths are rejected");
        workspace->SetWindowPos(nullptr, 0, 0, 720, 480, SWP_NOACTIVATE | SWP_NOZORDER); workspace->NewComparison();
        compare = workspace->sessions_.back(); CRect left_path, left_button, right_path, right_button;
        compare->compare_left_path_.GetWindowRect(left_path); compare->compare_browse_left_.GetWindowRect(left_button);
        compare->compare_right_path_.GetWindowRect(right_path); compare->compare_browse_right_.GetWindowRect(right_button);
        check(left_path.Width() > 0 && left_path.right <= left_button.left && right_path.Width() > 0 && right_path.right <= right_button.left,
              L"comparison path controls remain separate and usable at minimum workspace size");
        const HWND workspace_window = workspace->GetSafeHwnd(); workspace->OnClose();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (::IsWindow(workspace_window) && std::chrono::steady_clock::now() < deadline) {
            MSG message{}; while (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) { ::TranslateMessage(&message); ::DispatchMessage(&message); }
            ::Sleep(1);
        }
        check(!::IsWindow(workspace_window), L"workspace with several clean sessions closes through normal window lifetime");
        AfxGetApp()->m_pMainWnd = nullptr; return failures;
    }
    static int RunAdvanced(const std::filesystem::path& root, const std::filesystem::path& screenshots = {}) {
        int failures = 0; const auto check = [&](bool value, const wchar_t* label) { std::wcout << (value ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!value) ++failures; };
        const auto write_text = [&](const std::filesystem::path& path, const std::string& text) { std::filesystem::create_directories(path.parent_path()); Write(path,{text.begin(),text.end()}); };
        const auto text = [](CEdit& edit) { CString value; edit.GetWindowTextW(value); std::wstring result; for (const auto& line : simple_editor::core::SplitLines(value.GetString())) { result += line.text; if (!line.ending.empty()) result += L"\n"; } return result; };
        const auto bytes_text = [&](const std::filesystem::path& path) { const auto bytes = ReadDisk(path); return std::string(bytes.begin(),bytes.end()); };
        const auto capture = [&](CWnd* window,const wchar_t* name) {
            if (screenshots.empty()) return;
            std::filesystem::create_directories(screenshots); window->ShowWindow(SW_SHOWNOACTIVATE); window->RedrawWindow(nullptr,nullptr,RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            CRect bounds; window->GetWindowRect(bounds); CImage picture; picture.Create(bounds.Width(),bounds.Height(),32);
            const HDC target_dc = picture.GetDC(); const BOOL rendered = ::PrintWindow(window->GetSafeHwnd(),target_dc,0); picture.ReleaseDC();
            check(rendered && SUCCEEDED(picture.Save((screenshots / name).c_str())),L"capture test-owned native comparison window"); window->ShowWindow(SW_HIDE);
        };
        auto* workspace = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = workspace;
        workspace->SetWindowPos(nullptr,0,0,1400,1000,SWP_NOACTIVATE | SWP_NOZORDER); goto_frame_ = workspace->GetSafeHwnd();
        const auto newline_source = root / L"editor-newlines.txt", newline_output = root / L"editor-newlines-result.txt";
        write_text(newline_source,"a\nb\r\nc\r"); auto* newline_editor = new CMainFrame(false); newline_editor->OpenPath(newline_source.wstring()); newline_editor->ShowPage(1);
        check(newline_editor->text_.GetLineCount() == 4,L"native source editor displays mixed LF/CRLF/CR lines");
        newline_editor->text_.SetSel(3,4); newline_editor->text_.ReplaceSel(L"B",TRUE);
        check(newline_editor->StartSave(newline_output.wstring()) && Wait(newline_editor) && bytes_text(newline_output) == "a\nB\r\nc\r" && bytes_text(newline_source) == "a\nb\r\nc\r",L"actual editor Save As preserves mixed newline styles and the source file"); newline_editor->DestroyWindow();
        const auto base = root / L"base.txt", left = root / L"left.cpp", right = root / L"right.cpp";
        write_text(base,"one\ntwo\nthree\n"); write_text(left,"ONE\ntwo\nthree\n"); write_text(right,"one\ntwo\nTHREE\n"); workspace->OpenPath(left.wstring()); auto* editor = workspace->sessions_[0];
        check(!editor->advanced_.GetSafeHwnd(),L"ordinary editor sessions create no advanced control tree");
        workspace->NewComparisonInputs(left.wstring(),right.wstring(),L"",2,true); auto* frame = workspace->sessions_.back(); auto& view = frame->advanced_;
        check(Wait(frame) && view.model_ && view.rows_.GetItemCount() == 3,L"two-way text view aligns source lines in a separate session");
        check(view.output_.GetLineCount() == 4,L"LF source output displays as actual native edit lines");
        editor->ShowPage(1); check(editor->text_.GetLineCount() == 4,L"LF source editor displays separate lines"); editor->ShowPage(0);
        view.rows_.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED); view.Take(simple_editor::core::MergeChoice::Right);
        check(Wait(frame) && text(view.output_) == L"one\ntwo\nthree\n",L"take-right changes only the selected difference block");
        view.Undo(); check(Wait(frame) && text(view.output_) == L"ONE\ntwo\nthree\n",L"merge choice Undo restores the previous result");
        view.Undo(true); check(Wait(frame) && text(view.output_) == L"one\ntwo\nthree\n",L"merge choice Redo reapplies the chosen block");
        const auto output = root / L"two-way-result.txt";
        check(view.SaveTo(output.wstring()) && Wait(frame) && bytes_text(output) == "one\ntwo\nthree\n" && !view.Dirty(),L"merge result saves atomically without changing source files");
        check(bytes_text(left) == "ONE\ntwo\nthree\n" && bytes_text(right) == "one\ntwo\nTHREE\n",L"two-way choices preserve both original files");
        capture(workspace,L"text-compare.png");
        view.rows_.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED); view.Take(simple_editor::core::MergeChoice::Left); view.Cancel();
        check(Wait(frame) && text(view.output_) == L"one\ntwo\nthree\n" && !view.Dirty(),L"cancelled block render restores choices and preserves saved output");
        view.output_.SetSel(0,1); view.output_.ReplaceSel(L"C",TRUE);
        check(view.SaveTo(output.wstring()),L"start merge save for late-cancel boundary");
        const auto save_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (view.job_.valid() && view.job_.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready && std::chrono::steady_clock::now() < save_deadline) {}
        view.close_after_save_ = true; view.Cancel(); view.Tick();
        check(bytes_text(output) == "Cne\ntwo\nthree\n" && !view.Dirty() && !view.PendingClose(),L"late save cancellation adopts committed output and cancels queued close");
        view.InvalidateInputs(); view.OnFilterChanged(); view.Undo(); Wait(frame);
        check(view.stale_ && view.rows_.GetItemCount() == 0,L"filter and Undo never revive results after input paths changed");
        confirm_command_ = IDNO; auto reset_timer = ::SetTimer(nullptr,0,40,ConfirmationTimer); frame->OnCompareRun(); ::KillTimer(nullptr,reset_timer);
        check(Wait(frame) && !view.stale_ && view.rows_.GetItemCount() == 3,L"recompare refreshes invalidated source rows after explicit output discard");
        check(view.SaveTo(output.wstring()) && Wait(frame),L"save refreshed output baseline");
        view.output_.SetSel(0,1); view.output_.ReplaceSel(L"M",TRUE); const auto manual = text(view.output_); view.Take(simple_editor::core::MergeChoice::Left);
        check(!view.Busy() && text(view.output_) == manual,L"block commands never discard manual merge edits"); view.Undo();
        check(!view.manual_ && !view.Dirty(),L"native output Undo returns to the saved generated result");
        check(workspace->LockPaths({left.wstring()},true),L"synchronization can acquire a clean source editor"); const auto original = Read(editor);
        const auto alias = root / L"left-alias.cpp"; const bool linked = CreateHardLinkW(alias.c_str(),left.c_str(),nullptr) != FALSE;
        check(linked && !workspace->LockPaths({alias.wstring()},true,&view),L"file-identity locks reject overlapping hard-link paths");
        workspace->LockPaths({left.wstring()},false,&view);
        check(workspace->DocumentLocked(editor->document_),L"releasing another owner never releases an existing operation lock");
        editor->hex_.GoTo(0); editor->hex_.SendMessage(WM_CHAR,'4'); editor->OnInsert();
        check(Read(editor) == original && !editor->StartSave(left.wstring()),L"locked source editor rejects byte edits and Save"); workspace->LockPaths({left.wstring()},false);
        editor->document_.SetByte(0,'X'); editor->DocumentChanged();
        check(!view.SaveTo(left.wstring()) && bytes_text(left) == "ONE\ntwo\nthree\n",L"merge output cannot overwrite a source editor with unsaved changes"); editor->OnUndo();
        view.output_.SetSel(0,1); view.output_.ReplaceSel(L"M",TRUE); confirm_command_ = IDCANCEL;
        auto timer = ::SetTimer(nullptr,0,40,ConfirmationTimer); const bool closed = workspace->CloseSession(frame); ::KillTimer(nullptr,timer);
        check(!closed && view.Dirty(),L"Cancel closing a merge retains its manual output"); confirm_command_ = IDNO; timer = ::SetTimer(nullptr,0,40,ConfirmationTimer); workspace->CloseSession(frame); ::KillTimer(nullptr,timer);

        workspace->NewComparisonInputs(left.wstring(),right.wstring(),base.wstring(),3,true); frame = workspace->sessions_.back(); auto& three = frame->advanced_;
        check(Wait(frame) && three.unresolved_ == 0 && text(three.output_) == L"ONE\ntwo\nTHREE\n",L"three-way view automatically merges independent changes");
        check(three.SaveTo((root / L"auto-merge.txt").wstring()) && Wait(frame),L"automatic three-way output can be saved separately");
        write_text(right,"RIGHT\ntwo\nthree\n"); frame->OnCompareRun();
        check(Wait(frame) && three.unresolved_ == 1 && text(three.output_).find(L"<<<<<<< LEFT") != std::wstring::npos,L"overlapping three-way changes remain visible conflicts");
        check(!three.SaveTo((root / L"must-not-exist.txt").wstring()) && !std::filesystem::exists(root / L"must-not-exist.txt"),L"unresolved conflicts are not silently saved");
        capture(workspace,L"three-way-conflict.png");
        three.output_.SetWindowTextW(L"reviewed\ntwo\nthree\n"); three.OnResultChanged(); three.OnAction(ID_ADV_MANUAL_RESOLVED);
        check(three.unresolved_ == 0 && three.manual_resolved_ && three.SaveTo((root / L"manual-resolved.txt").wstring()) && Wait(frame) && bytes_text(root / L"manual-resolved.txt") == "reviewed\ntwo\nthree\n",L"reviewed manual conflict resolution saves the exact edited output");
        three.output_.SetSel(0,1); three.output_.ReplaceSel(L"R",TRUE);
        check(three.unresolved_ == 1 && !three.manual_resolved_,L"editing a reviewed result requires conflict acknowledgement again");
        three.dirty_ = false; frame->OnCompareRun(); check(Wait(frame) && three.unresolved_ == 1,L"recompare restores original unresolved conflict after manual review");
        three.rows_.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED); three.Take(simple_editor::core::MergeChoice::Both);
        check(Wait(frame) && three.unresolved_ == 0 && text(three.output_).find(L"ONE\nRIGHT\n") == 0,L"take-both resolves a conflict while retaining both changes");
        three.Undo(); check(Wait(frame) && three.unresolved_ == 1,L"Undo restores an unresolved conflict");
        three.rows_.SetItemState(0,LVIS_SELECTED,LVIS_SELECTED); three.Take(simple_editor::core::MergeChoice::Right);
        check(Wait(frame) && three.unresolved_ == 0 && three.SaveTo((root / L"resolved.txt").wstring()) && Wait(frame) && bytes_text(root / L"resolved.txt") == "RIGHT\ntwo\nthree\n",L"selected conflict resolution saves exact right-side text");
        frame->SetTabZoom(2,180); LOGFONT font{}; check(three.rows_.GetFont()->GetLogFont(&font) && abs(font.lfHeight) > 13,L"advanced comparison content participates in percentage zoom");

        const auto folder_left = root / L"folder-left", folder_right = root / L"folder-right";
        write_text(folder_left / L"changed.txt","L"); write_text(folder_right / L"changed.txt","R"); write_text(folder_left / L"sub" / L"new.bin","N"); write_text(folder_right / L"extra.txt","E");
        workspace->OpenPath((folder_right / L"changed.txt").wstring()); auto* target = workspace->sessions_.back();
        workspace->NewComparisonInputs(folder_left.wstring(),folder_right.wstring(),L"",4,true); auto* folder_frame = workspace->sessions_.back(); auto& folder = folder_frame->advanced_;
        CString exact_folder; folder_frame->compare_right_path_.GetWindowTextW(exact_folder);
        check(exact_folder == folder_right.wstring().c_str(),L"folder paths remain exact when an open file has that directory as a prefix");
        check(Wait(folder_frame) && folder.scan_ && folder.rows_.GetItemCount() == 4,L"folder view recursively lists actual file and directory differences");
        capture(workspace,L"folder-compare.png");
        preview_accept_ = false; timer = ::SetTimer(nullptr,0,40,PreviewTimer); folder.PlanSync(true,true); check(Wait(folder_frame),L"folder preview can be cancelled"); ::KillTimer(nullptr,timer);
        check(bytes_text(folder_right / L"changed.txt") == "R" && !std::filesystem::exists(folder_right / L"sub" / L"new.bin"),L"cancelled preview makes no filesystem changes");
        preview_accept_ = true; timer = ::SetTimer(nullptr,0,40,PreviewTimer); folder.PlanSync(true,true);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (folder.Busy() && folder.job_kind_ != CAdvancedCompareView::JobKind::Sync && std::chrono::steady_clock::now() < deadline) { folder_frame->OnTimer(1); ::Sleep(1); }
        ::KillTimer(nullptr,timer); check(workspace->PathLocked((folder_right / L"changed.txt").wstring()),L"confirmed sync holds path locks until its completion is adopted");
        target->OnInsert(); check(!target->document_.IsModified(),L"an open target editor cannot mutate during synchronization");
        check(Wait(folder_frame) && bytes_text(folder_right / L"changed.txt") == "L" && bytes_text(folder_right / L"sub" / L"new.bin") == "N" && bytes_text(folder_right / L"extra.txt") == "E",L"confirmed update sync copies nested content while retaining extras");
        check(!workspace->PathLocked((folder_right / L"changed.txt").wstring()),L"sync completion releases editor locks"); if (target->document_.IsModified()) target->OnUndo(); target->OnReload();
        folder.mirror_.SetCheck(BST_CHECKED); timer = ::SetTimer(nullptr,0,40,PreviewTimer); folder.PlanSync(true,true); check(Wait(folder_frame),L"mirror preview and operation finish"); ::KillTimer(nullptr,timer);
        bool backup_found = false;
        if (std::filesystem::exists(folder_right / L".SimpleEditorTool.backups")) for (const auto& item : std::filesystem::recursive_directory_iterator(folder_right / L".SimpleEditorTool.backups")) if (item.path().filename() == L"extra.txt" && bytes_text(item.path()) == "E") backup_found = true;
        check(!std::filesystem::exists(folder_right / L"extra.txt") && backup_found,L"mirror retains target-only file content in a recovery backup");
        int file_row = -1; for (int i = 0; i < folder.rows_.GetItemCount(); ++i) if (folder.Cell(i,0) == L"changed.txt") file_row = i;
        NMITEMACTIVATE activate{}; activate.iItem = file_row; LRESULT handled = 0; const auto previous_count = workspace->sessions_.size(); folder.OnDoubleClick(&activate.hdr,&handled);
        check(workspace->sessions_.size() == previous_count + 1 && workspace->sessions_.back()->displayed_mode_ == 2 && Wait(workspace->sessions_.back()),L"double-clicking a folder file pair opens an independent text comparison");
        editor->SetTabZoom(1,160); const auto entries = workspace->CaptureWorkspace(); std::wstring error; const auto session_file = root / L"workspace.ini";
        check(simple_editor::core::SaveWorkspaceFile(session_file,entries,&error),L"workspace paths and options save to a portable INI");
        std::vector<simple_editor::core::WorkspaceEntry> loaded; check(simple_editor::core::LoadWorkspaceFile(session_file,&loaded,&error) && loaded.size() == entries.size(),L"workspace INI parses all session categories");
        auto* restored = new CWorkspaceFrame(false); AfxGetApp()->m_pMainWnd = restored; restored->RestoreWorkspace(loaded);
        check(restored->sessions_.size() == entries.size() && restored->sessions_[0]->zoom_percent_[1] == 160,L"workspace restore recreates file, merge and folder sessions with view settings");
        restored->OnClose(); const HWND restored_window = restored->GetSafeHwnd();
        const auto finish = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (::IsWindow(restored_window) && std::chrono::steady_clock::now() < finish) { MSG message{}; while (::PeekMessage(&message,nullptr,0,0,PM_REMOVE)) { ::TranslateMessage(&message); ::DispatchMessage(&message); } ::Sleep(1); }
        AfxGetApp()->m_pMainWnd = workspace; editor->ShowPage(1); editor->text_.SetSel(0,3); editor->text_.ReplaceSel(L"draft",TRUE); const auto count_before = workspace->sessions_.size(); workspace->RestoreWorkspace(loaded);
        check(workspace->sessions_.size() == count_before && editor->text_.GetModify() && text(editor->text_).find(L"draft") == 0,L"loading a workspace preserves existing drafts and avoids duplicate sessions");
        confirm_command_ = IDNO; timer = ::SetTimer(nullptr,0,40,ConfirmationTimer); const HWND window = workspace->GetSafeHwnd(); workspace->OnClose();
        const auto close_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (::IsWindow(window) && std::chrono::steady_clock::now() < close_deadline) { MSG message{}; while (::PeekMessage(&message,nullptr,0,0,PM_REMOVE)) { ::TranslateMessage(&message); ::DispatchMessage(&message); } ::Sleep(1); }
        ::KillTimer(nullptr,timer); check(!::IsWindow(window),L"advanced sessions close through normal MFC ownership after explicit discard"); AfxGetApp()->m_pMainWnd = nullptr; return failures;
    }
    static int RunHexTools(const std::filesystem::path& root) {
        int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
        std::vector<std::uint8_t> original(64); for (unsigned i = 0; i < original.size(); ++i) original[i] = static_cast<std::uint8_t>(i);
        const auto path = root / L"hex-tools.bin", right = root / L"right.bin"; Write(path,original);
        auto* frame = new CMainFrame(false); AfxGetApp()->m_pMainWnd = frame; frame->SetWindowPos(nullptr,0,0,1400,900,SWP_NOACTIVATE | SWP_NOZORDER); frame->OpenPath(path.wstring()); frame->ShowPage(0);
        CHexView::ClipboardContent private_clipboard;
        frame->hex_.clipboard_access = [&](bool write,CHexView::ClipboardContent& content) { if (write) private_clipboard = content; else { const bool text = content.want_text; content = private_clipboard; content.want_text = text; } return true; };
        std::wcout << L"INFO: Clipboard operations use an isolated per-control store; native OS clipboard transport is not exercised.\n";
        TEXTMETRIC metrics{}; { CClientDC dc(&frame->hex_); CFont font; font.CreatePointFont(100,L"Consolas",&dc); auto* old = dc.SelectObject(&font); dc.GetTextMetrics(&metrics); dc.SelectObject(old); }
        const auto point = [&](unsigned offset) { const auto column = offset % 16; return CPoint(8 + static_cast<int>(10 + column * 3 + (column >= 8 ? 1 : 0)) * metrics.tmAveCharWidth + 1,static_cast<int>(offset / 16) * (metrics.tmHeight + 3) + 1); };
        const auto drag = [&](unsigned start,unsigned end) { const auto a = point(start), b = point(end); frame->hex_.SendMessage(WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(a.x,a.y)); frame->hex_.SendMessage(WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(b.x,b.y)); frame->hex_.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(b.x,b.y)); };
        drag(2,20); check(frame->hex_.SelectionStart() == 2 && frame->hex_.SelectionLength() == 19,L"mouse selection spans Hex rows and includes both endpoints");
        drag(20,2); check(frame->hex_.SelectionStart() == 2 && frame->hex_.SelectionLength() == 19,L"reverse Hex drag retains the same selected range");
        check(frame->hex_.Clipboard(ID_HEX_COPY),L"copy selection writes Hex text and exact binary clipboard formats");
        check(private_clipboard.bytes == std::vector<std::uint8_t>(original.begin() + 2,original.begin() + 21) && private_clipboard.text.find(L"02 03 04") == 0,L"clipboard payload preserves exact selected bytes and formatted Hex text");
        frame->hex_.GoTo(40); check(frame->hex_.Clipboard(ID_HEX_PASTE_WRITE),L"paste overwrite accepts the binary clipboard");
        auto overwritten = original; std::copy(original.begin() + 2,original.begin() + 21,overwritten.begin() + 40);
        check(Read(frame) == overwritten && frame->document_.Size() == 64,L"overwrite paste preserves unaffected bytes and file length"); frame->OnUndo(); check(Read(frame) == original,L"one Undo restores the entire pasted byte range");
        frame->hex_.SelectRange(0,2); check(frame->hex_.Clipboard(ID_HEX_PASTE_INSERT) && frame->document_.Size() == 81,L"insert paste replaces selection and grows the document once"); frame->OnUndo(); check(Read(frame) == original,L"insert paste Undo restores original length and bytes");
        frame->hex_.SelectRange(2,19); check(frame->hex_.Clipboard(ID_HEX_CUT) && frame->document_.Size() == 45,L"Cut removes exactly the selected byte range"); frame->OnUndo(); check(Read(frame) == original,L"Cut has one document Undo action");
        private_clipboard.has_bytes = false; private_clipboard.text = L"0xAA,0xBB;\\xCC"; frame->hex_.GoTo(0);
        check(frame->hex_.Clipboard(ID_HEX_PASTE_WRITE) && Read(frame)[0] == 0xAA && Read(frame)[2] == 0xCC,L"ordinary clipboard text accepts common Hex literal formats"); frame->OnUndo();
        private_clipboard.text = L"not hex"; frame->hex_.GoTo(0); check(!frame->hex_.Clipboard(ID_HEX_PASTE_WRITE) && Read(frame) == original,L"malformed clipboard text cannot modify binary data");
        private_clipboard.text = L"\u4e2d"; check(frame->hex_.Clipboard(ID_HEX_PASTE_TEXT) && Read(frame)[0] == 0xE4 && Read(frame)[2] == 0xAD,L"explicit UTF-8 paste encodes Unicode text"); frame->OnUndo();
        BYTE keyboard_before[256]{}, keyboard[256]{}; GetKeyboardState(keyboard_before); memcpy(keyboard,keyboard_before,sizeof(keyboard)); keyboard[VK_SHIFT] = 0x80;
        frame->hex_.GoTo(3); SetKeyboardState(keyboard); frame->hex_.SendMessage(WM_KEYDOWN,VK_DOWN); SetKeyboardState(keyboard_before);
        check(frame->hex_.SelectionStart() == 3 && frame->hex_.SelectionLength() == 16,L"Shift+arrow selection spans byte rows");
        frame->SetSaveReadOnly(true); frame->hex_.SelectRange(0,2); check(frame->hex_.Clipboard(ID_HEX_COPY) && !frame->hex_.Clipboard(ID_HEX_PASTE_WRITE) && !frame->hex_.DeleteSelection(),L"read-only Hex allows copying and blocks all byte mutations"); frame->SetSaveReadOnly(false);
        frame->hex_.GoTo(0); frame->hex_.SendMessage(WM_KEYDOWN,VK_BACK); check(Read(frame) == original,L"Backspace at offset zero preserves the first byte");
        const int ascii_x = 8 + (61 + 5) * metrics.tmAveCharWidth + 1;
        frame->hex_.SendMessage(WM_LBUTTONDOWN,0,MAKELPARAM(ascii_x,1)); frame->hex_.SendMessage(WM_LBUTTONUP,0,MAKELPARAM(ascii_x,1)); frame->hex_.SendMessage(WM_CHAR,'Z');
        check(frame->hex_.AsciiMode() && Read(frame)[5] == 'Z',L"ASCII-column hit testing and typing write the clicked byte"); frame->OnUndo();
        frame->hex_.GoTo(0); frame->hex_.SendMessage(WM_CHAR,0xD83D); frame->hex_.SendMessage(WM_CHAR,0xDE00);
        const auto emoji = Read(frame); check(emoji[0] == 0xF0 && emoji[1] == 0x9F && emoji[2] == 0x98 && emoji[3] == 0x80,L"ASCII/UTF-8 input combines a Unicode surrogate pair"); frame->OnUndo();
        frame->hex_.GoTo(0); frame->hex_.SendMessage(WM_KEYDOWN,VK_INSERT); frame->hex_.SendMessage(WM_CHAR,'A');
        check(frame->hex_.InsertMode() && frame->document_.Size() == 65 && Read(frame)[0] == 'A',L"Insert mode changes input behavior without a fixed zero-byte command"); frame->OnUndo(); frame->hex_.SetInsertMode(false);
        frame->hex_.GoTo(7); const auto cursor = frame->hex_.CursorOffset();
        check(frame->hex_.SetLayout(32,4,0x100000000ULL) && frame->hex_.CursorOffset() == cursor && Read(frame) == original,L"Hex row width/grouping/base address preserve document bytes and caret");
        check(!frame->hex_.SetLayout(17,4,0) && !frame->hex_.SetLayout(16,8,UINT64_MAX),L"invalid Hex layouts and overflowing base addresses are rejected"); frame->hex_.SetLayout(16,8,0);
        Write(right,original); { std::fstream changed(right,std::ios::in | std::ios::out | std::ios::binary); changed.seekp(12); changed.put(static_cast<char>(254)); }
        frame->StartCompare(right.wstring(),false); check(Wait(frame) && frame->pending_differences_.size() == 1,L"binary comparison is retained before report export");
        const auto report = root / L"ui-report.csv"; check(frame->StartBinaryReport(report.wstring(),0) && Wait(frame),L"actual MFC report command completes asynchronously");
        const auto report_bytes = ReadDisk(report); const std::string csv(report_bytes.begin(),report_bytes.end());
        check(csv.find("changed,0x000000000000000C,0C,0x000000000000000C,FE") != std::string::npos && ReadDisk(path) == original,L"UI export writes exact addresses/data without changing source files");
        frame->DestroyWindow(); AfxGetApp()->m_pMainWnd = nullptr; return failures;
    }
private:
    inline static bool preview_accept_ = false;
    static void CALLBACK PreviewTimer(HWND,UINT,UINT_PTR,DWORD) { const HWND dialog = TestDialog(); if (dialog && ::GetDlgItem(dialog,IDC_SYNC_OPERATIONS)) ::PostMessageW(dialog,WM_COMMAND,preview_accept_ ? IDOK : IDCANCEL,0); }
    static HDROP MakeDrop(const std::vector<std::wstring>& paths) {
        std::size_t count = 1; for (const auto& path : paths) count += path.size() + 1;
        HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(DROPFILES) + count * sizeof(wchar_t));
        auto* header = static_cast<DROPFILES*>(GlobalLock(memory)); header->pFiles = sizeof(DROPFILES); header->fWide = TRUE;
        auto* destination = reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(header) + sizeof(DROPFILES));
        for (const auto& path : paths) { memcpy(destination, path.c_str(), (path.size() + 1) * sizeof(wchar_t)); destination += path.size() + 1; }
        GlobalUnlock(memory); return reinterpret_cast<HDROP>(memory);
    }
    static void CALLBACK ErrorTimer(HWND, UINT, UINT_PTR, DWORD) {
        const HWND dialog = TestDialog();
        if (dialog && !::GetDlgItem(dialog, IDYES)) ::PostMessageW(dialog, WM_CLOSE, 0, 0);
    }
    inline static HWND goto_frame_ = nullptr;
    inline static int confirm_command_ = IDCANCEL;
    inline static bool saw_picker_ = false;
    static HWND TestDialog() {
        HWND found = nullptr;
        ::EnumThreadWindows(GetCurrentThreadId(), [](HWND window, LPARAM value) -> BOOL {
            wchar_t name[32]{}; ::GetClassNameW(window, name, 32);
            if (wcscmp(name, L"#32770") == 0 && ::IsWindowVisible(window)) { *reinterpret_cast<HWND*>(value) = window; return FALSE; }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        return found;
    }
    static void CALLBACK PickerCancelTimer(HWND, UINT, UINT_PTR, DWORD) {
        const HWND dialog = TestDialog();
        if (dialog && ::GetDlgItem(dialog, IDCANCEL)) { saw_picker_ = true; ::PostMessageW(dialog, WM_CLOSE, 0, 0); }
    }
    static void CALLBACK ConfirmationTimer(HWND, UINT, UINT_PTR, DWORD) {
        const HWND dialog = TestDialog();
        if (dialog != goto_frame_ && ::GetDlgItem(dialog, IDYES) && ::GetDlgItem(dialog, confirm_command_)) ::PostMessageW(dialog, WM_COMMAND, confirm_command_, 0);
    }
    static void CALLBACK AddressTimer(HWND, UINT, UINT_PTR, DWORD) {
        const HWND dialog = ::GetLastActivePopup(goto_frame_);
        if (dialog != goto_frame_ && ::GetDlgItem(dialog, IDC_ADDRESS)) { ::SetDlgItemTextW(dialog, IDC_ADDRESS, L"0x5"); ::PostMessageW(dialog, WM_COMMAND, IDOK, 0); }
    }
    static bool Wait(CMainFrame* frame) {
        const HWND window = frame->GetSafeHwnd();
        const auto alive = [&]() { return ::IsWindow(window) && CWnd::FromHandlePermanent(window) == frame; };
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (alive() && frame->IsBusy() && std::chrono::steady_clock::now() < deadline) {
            MSG message{}; while (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) { ::TranslateMessage(&message); ::DispatchMessage(&message); if (!alive()) return true; }
            frame->OnTimer(1); ::Sleep(1);
        }
        return !alive() || !frame->IsBusy();
    }
    static bool WaitClosed(CMainFrame* frame, HWND window) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (::IsWindow(window) && std::chrono::steady_clock::now() < deadline) {
            MSG message{}; while (::PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) { ::TranslateMessage(&message); ::DispatchMessage(&message); if (!::IsWindow(window)) return true; }
            if (CWnd::FromHandlePermanent(window) != frame) return false;
            frame->OnTimer(1); ::Sleep(1);
        }
        return !::IsWindow(window);
    }
    static std::vector<std::uint8_t> ReadDisk(const std::filesystem::path& path) { std::ifstream file(path, std::ios::binary); return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()}; }
    static std::vector<std::uint8_t> Read(CMainFrame* frame) { std::vector<std::uint8_t> bytes; frame->document_.Read(0, static_cast<std::size_t>(frame->document_.Size()), &bytes); return bytes; }
    static void Write(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) { std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); }
};

CWinApp test_app;
int wmain(int argc, wchar_t** argv) {
    InitializeNativeExceptionTrace();
    std::wcout << std::unitbuf; std::wcerr << std::unitbuf;
    const std::wstring mode = argc > 1 ? argv[1] : L"";
    if (!AfxWinInit(GetModuleHandle(nullptr), nullptr, GetCommandLineW(), SW_HIDE)) return 2;
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_WIN95_CLASSES}; InitCommonControlsEx(&controls);
    wchar_t temp[MAX_PATH]{}, name[MAX_PATH]{}; GetTempPathW(MAX_PATH, temp); GetTempFileNameW(temp, L"set", 0, name); DeleteFileW(name); CreateDirectoryW(name, nullptr); { std::ofstream isolated(std::filesystem::path(name) / L".editorconfig",std::ios::binary); isolated << "root = true\n"; }
    int failures = 0;
    if (mode == L"--workspace-process") {
        if (argc != 3) { std::wcerr << L"Usage: NativeUiTests.exe --workspace-process <formal-exe>\n"; failures = 1; }
        else failures += RunWorkspaceProcessTests(name, std::filesystem::absolute(argv[2]));
    }
    else if (mode == L"--outline-sidebar") failures += NativeUiTest::RunOutlineSidebar(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--document-focus") failures += NativeUiTest::RunDocumentFocus(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--localization") failures += NativeUiTest::RunLocalization(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--workflow-expansion") failures += NativeUiTest::RunWorkflowExpansion(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--workflows") failures += NativeUiTest::RunWorkflows(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--discovery-completion") failures += NativeUiTest::RunDiscoveryCompletion(name);
    else if (mode == L"--discovery-visual") failures += NativeUiTest::RunDiscoveryVisual(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--discovery-flows") failures += NativeUiTest::RunDiscoveryFlows(name);
    else if (mode == L"--discovery") failures += NativeUiTest::RunDiscovery(name);
    else if (mode == L"--progressive-compare") failures += NativeUiTest::RunProgressiveCompare(name);
    else if (mode == L"--inspection") failures += NativeUiTest::RunInspection(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--structured") failures += NativeUiTest::RunStructured(name);
    else if (mode == L"--recovery") failures += NativeUiTest::RunRecovery(name);
    else if (mode == L"--preferences") failures += NativeUiTest::RunPreferences(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--documents") failures += NativeUiTest::RunDocuments(name);
    else if (mode == L"--cross-search") failures += NativeUiTest::RunCrossSearch(name);
    else if (mode == L"--folder-tools") failures += NativeUiTest::RunFolderTools(name);
    else if (mode == L"--drop-targets") failures += NativeUiTest::RunDropTargets(name);
    else if (mode == L"--text-reports") failures += NativeUiTest::RunTextReports(name);
    else if (mode == L"--regions") failures += NativeUiTest::RunRegions(name);
    else if (mode == L"--presentation") failures += NativeUiTest::RunPresentation(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--format-tools") failures += NativeUiTest::RunFormatTools(name);
    else if (mode == L"--search-tools") failures += NativeUiTest::RunSearchTools(name);
    else if (mode == L"--code-editor") failures += NativeUiTest::RunCodeEditor(name);
    else if (mode == L"--hex-tools") failures += NativeUiTest::RunHexTools(name);
    else if (mode == L"--editor-review") failures += NativeUiTest::RunEditorReview(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--text-workflow") failures += NativeUiTest::RunTextWorkflow(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--pasted-text") failures += NativeUiTest::RunPastedText(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--advanced") failures += NativeUiTest::RunAdvanced(name,argc > 2 ? std::filesystem::absolute(argv[2]) : std::filesystem::path());
    else if (mode == L"--explorer") failures += NativeUiTest::RunExplorer(name);
    else if (mode == L"--explorer-process" && argc > 2) failures += RunExplorerProcessTests(name,std::filesystem::absolute(argv[2]));
    else if (mode == L"--enable-explorer" && argc > 2) { std::wstring error; if (!simple_editor::shell::SetUserExplorerMenu(std::filesystem::absolute(argv[2]).wstring(),true,&error)) { std::wcerr << error << L'\n'; ++failures; } }
    else if (mode == L"--workspace") failures += NativeUiTest::RunWorkspace(name);
    else if (mode == L"--zoom") failures += NativeUiTest::RunZoom(name);
    else {
        if (mode != L"--async-save") { failures += NativeUiTest::Run(name); failures += NativeUiTest::RunZoom(name); }
        failures += NativeUiTest::RunAsyncSave(name);
    }
    simple_editor::ui::ShutdownLocalization(); CCodeEditor::ReleaseResources();
    std::error_code ignored; std::filesystem::remove_all(name, ignored);
    std::wcout << (failures ? L"FAIL: native MFC UI tests\n" : L"PASS: native MFC UI tests\n"); return failures ? 1 : 0;
}

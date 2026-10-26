#include "workspace_frame.h"
#include "main_frame.h"
#include "resource.h"
#include "ui/preferences_dialog.h"
#include "ui/localization.h"
#include <algorithm>
#include <chrono>
using namespace simple_editor::core;
namespace {
const UINT kCommands[] = {ID_FILE_NEW,ID_FILE_OPEN,ID_FILE_SAVE,ID_EDIT_FIND,ID_EDIT_REPLACE,ID_WORKSPACE_SEARCH_FILES,ID_WORKSPACE_REOPEN};
}
void CWorkspaceFrame::UpdateMenuShortcuts(CMenu& menu) const {
    if (!menu.GetSafeHmenu()) return;
    for (int position = 0; position < menu.GetMenuItemCount(); ++position) {
        if (auto* child = menu.GetSubMenu(position)) { UpdateMenuShortcuts(*child); continue; }
        const auto id = menu.GetMenuItemID(position); for (std::size_t i = 0; i < preferences_.shortcuts.size(); ++i) if (id == kCommands[i]) { CString previous; menu.GetMenuString(position,previous,MF_BYPOSITION); std::wstring text = previous.GetString(); const auto tab = text.find(L'\t'); if (tab != std::wstring::npos) text.erase(tab); if (preferences_.shortcuts[i].key) text += L"\t" + FormatShortcut(preferences_.shortcuts[i]); menu.ModifyMenu(position,MF_BYPOSITION | MF_STRING,id,text.c_str()); break; }
    }
}
bool CWorkspaceFrame::LoadConfiguration(const std::filesystem::path& path,std::wstring* error) {
    preferences_path_ = path; AppPreferences value; if (!LoadPreferences(path,&value,error)) return false;
    return ChangePreferences(value,false);
}
bool CWorkspaceFrame::ChangePreferences(const AppPreferences& value,bool persist) {
    std::wstring error; if (!ValidatePreferences(value,&error)) return false; preferences_ = value; recent_files_.assign(value.recent.begin(),value.recent.end());
    if (simple_editor::ui::LocalizationEnabled()) simple_editor::ui::SetUiLanguage(value.language);
    for (auto* frame : sessions_) ApplyPreferencesToFrame(frame); if (GetSafeHwnd()) RefreshRecentMenus();
    if (GetSafeHwnd() && (preferences_.auto_restore || preferences_.recover_drafts)) InitializeRecovery();
    if (persist) { preferences_dirty_ = true; StartPreferenceWrite(); } return true;
}
void CWorkspaceFrame::ApplyPreferencesToFrame(CMainFrame* frame) {
    const bool previous = applying_preferences_; applying_preferences_ = true;
    frame->text_face_ = preferences_.font; frame->content_font_tenths_ = preferences_.font_tenths; frame->content_theme_ = preferences_.theme; frame->dark_content_ = preferences_.theme == 1;
    for (CHexView* view : {static_cast<CHexView*>(&frame->hex_),static_cast<CHexView*>(&frame->compare_left_),static_cast<CHexView*>(&frame->compare_right_)}) view->SetAppearance(preferences_.font,preferences_.font_tenths,preferences_.theme);
    if (frame->text_clone_.GetSafeHwnd()) { frame->text_clone_.SetTheme(preferences_.theme); frame->text_clone_.SetWrap(preferences_.wrap); frame->text_clone_.SetWhitespace(preferences_.whitespace); frame->text_clone_.SetTabWidth(preferences_.tab_width,preferences_.use_tabs); }
    frame->text_.SetTheme(preferences_.theme); frame->text_.SetWrap(preferences_.wrap); frame->text_.SetWhitespace(preferences_.whitespace); frame->text_.SetTabWidth(preferences_.tab_width,preferences_.use_tabs);
    const auto back = preferences_.theme == 2 ? GetSysColor(COLOR_WINDOW) : preferences_.theme == 1 ? RGB(63,63,63) : RGB(255,255,255),fore = preferences_.theme == 2 ? GetSysColor(COLOR_WINDOWTEXT) : preferences_.theme == 1 ? RGB(225,228,235) : RGB(30,35,42);
    frame->differences_.SetBkColor(back); frame->differences_.SetTextBkColor(back); frame->differences_.SetTextColor(fore);
    frame->advanced_.SetAppearance(preferences_.font,preferences_.font_tenths,preferences_.theme);
    if (frame->indentation_manual_) for (auto* editor : {&frame->text_,&frame->text_clone_}) if (editor->GetSafeHwnd()) { const auto& settings = frame->manual_indentation_; editor->SetTabWidth(settings.tab_width,settings.use_tabs); editor->Call(SCI_SETINDENT,settings.indent_width); }
    frame->RequestIndentation(false); if (!frame->comparison_session_ && frame->document_.IsOpen()) frame->ConfigureFileSyntax();
    frame->advanced_.SetTextViewPreferences(preferences_.wrap,preferences_.whitespace,preferences_.tab_width,preferences_.use_tabs);
    CClientDC dc(frame); frame->comparison_base_font_.lfHeight = -MulDiv(preferences_.font_tenths,dc.GetDeviceCaps(LOGPIXELSY),720); frame->comparison_base_font_.lfWidth = 0; wcsncpy_s(frame->comparison_base_font_.lfFaceName,preferences_.font.c_str(),_TRUNCATE);
    if (auto* output = frame->advanced_.OutputEditor()) { output->SetWrap(preferences_.wrap); output->SetWhitespace(preferences_.whitespace); output->SetTabWidth(preferences_.tab_width,preferences_.use_tabs); }
    for (int i = 0; i < 3; ++i) { frame->zoom_percent_[i] = -1; frame->SetTabZoom(i,preferences_.zoom[i]); } applying_preferences_ = previous;
}
void CWorkspaceFrame::RememberView(CMainFrame* frame) {
    if (applying_preferences_ || !frame || !frame->GetSafeHwnd()) return;
    preferences_.font = frame->text_face_; preferences_.font_tenths = frame->content_font_tenths_; preferences_.theme = frame->content_theme_; for (int i = 0; i < 3; ++i) if (frame->zoom_percent_[i] >= 50) preferences_.zoom[i] = frame->zoom_percent_[i];
    if (auto* editor = frame->ActiveEditor()) {
        preferences_.wrap = editor->Call(SCI_GETWRAPMODE) != SC_WRAP_NONE; preferences_.whitespace = editor->Call(SCI_GETVIEWWS) != SCWS_INVISIBLE;
        const bool document_indentation = frame->IsFileEditor(editor) && (frame->indentation_manual_ || frame->indentation_result_.applied);
        if (!document_indentation) { preferences_.tab_width = static_cast<unsigned>(editor->Call(SCI_GETTABWIDTH)); preferences_.use_tabs = editor->Call(SCI_GETUSETABS) != 0; }
    }
    if (!preferences_path_.empty()) preferences_dirty_ = true;
}
void CWorkspaceFrame::OnPreferences() {
    if (workspace_dialog_) return; auto current = preferences_; current.recent.assign(recent_files_.begin(),recent_files_.end()); CPreferencesDialog dialog(this,current); workspace_dialog_ = true; const auto accepted = dialog.DoModal(); workspace_dialog_ = false; if (accepted == IDOK) ChangePreferences(dialog.value,true);
}
bool CWorkspaceFrame::StartPreferenceWrite() {
    if (preferences_io_.valid()) return false; preferences_dirty_ = false; if (preferences_path_.empty()) return false;
    auto value = preferences_; value.recent.assign(recent_files_.begin(),recent_files_.end()); const auto path = preferences_path_;
    if (!LockPaths({path.wstring()},true,&preferences_io_)) { UiMessageBox(L"Portable settings have unsaved edits or are busy in an editor. Preferences remain active in memory.",MB_ICONWARNING); return false; }
    try { preferences_io_ = std::async(std::launch::async,[path,value]() { std::wstring error; if (!SavePreferences(path,value,&error) && error.empty()) error = L"Could not save portable settings."; return error; }); }
    catch (...) { LockPaths({path.wstring()},false,&preferences_io_); UiMessageBox(L"Could not start saving portable preferences.",MB_ICONWARNING); return false; } SetTimer(3,50,nullptr); return true;
}
void CWorkspaceFrame::TickPreferences() {
    if (!preferences_io_.valid() || preferences_io_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
    std::wstring error; try { error = preferences_io_.get(); } catch (...) { error = L"Portable preferences could not be saved."; } LockPaths({},false,&preferences_io_); KillTimer(3);
    if (!error.empty()) UiMessageBox(error.c_str(),MB_ICONWARNING); const bool close = preferences_close_pending_; preferences_close_pending_ = false; if (close) OnClose(); else if (preferences_dirty_) StartPreferenceWrite();
}
void CWorkspaceFrame::OnLanguage(UINT command) {
    const std::wstring language = command == ID_LANGUAGE_TRADITIONAL ? L"tw" : L"en";
    if (!simple_editor::ui::SetUiLanguage(language)) { UiMessageBox(L"Language files are unavailable. Restore lang_en.ini and lang_tw.ini beside the EXE, then restart.",MB_ICONWARNING); return; }
    preferences_.language = language; preferences_dirty_ = true; if (!preferences_io_.valid()) StartPreferenceWrite();
    for (auto* frame : sessions_) { if (frame->advanced_.GetSafeHwnd()) frame->advanced_.Invalidate(FALSE); frame->Layout(); }
    DrawMenuBar();
}
bool CWorkspaceFrame::DispatchShortcut(MSG* message) {
    if ((message->message != WM_KEYDOWN && message->message != WM_SYSKEYDOWN) || !GetSafeHwnd() || (message->hwnd != GetSafeHwnd() && !::IsChild(GetSafeHwnd(),message->hwnd))) return false;
    const unsigned modifiers = (GetKeyState(VK_CONTROL) & 0x8000 ? 1 : 0) | (GetKeyState(VK_SHIFT) & 0x8000 ? 2 : 0) | (GetKeyState(VK_MENU) & 0x8000 ? 4 : 0);
    for (std::size_t i = 0; i < preferences_.shortcuts.size(); ++i) if (preferences_.shortcuts[i].key == message->wParam && preferences_.shortcuts[i].modifiers == modifiers) { SendMessage(WM_COMMAND,kCommands[i]); return true; }
    const AppPreferences defaults; for (const auto& binding : defaults.shortcuts) if (binding.key == message->wParam && binding.modifiers == modifiers) return true; return false;
}

#include "main_frame.h"
#include "resource.h"
#include "workspace_frame.h"
#include "core/search.h"
#include <sstream>

CCodeEditor* CMainFrame::ActiveEditor() { if (AdvancedActive()) return advanced_.SearchEditor(); if (active_tab_ != 1) return nullptr; if (::GetFocus() == text_clone_.GetSafeHwnd() && dual_view_) clone_active_ = true; else if (::GetFocus() == text_.GetSafeHwnd()) clone_active_ = false; return dual_view_ && clone_active_ ? &text_clone_ : &text_; }
void CMainFrame::OnAboutNative() { if (IsBusy()) return; CNativeNoticesDialog dialog(this); path_dialog_ = true; dialog.DoModal(); path_dialog_ = false; }
void CMainFrame::OnEditorCommand(UINT command) {
    if (command == ID_HEX_LAYOUT) {
        auto* view = ActiveHex(); if (!view || IsBusy()) return;
        wchar_t initial[96]{}; swprintf_s(initial,L"%u,%u,0x%llX",view->BytesPerRow(),view->Grouping(),view->BaseAddress());
        CValueDialog dialog(this,L"Hex layout",L"Bytes per row (8/16/32), byte group (1/2/4/8), display base address (hex). Example: 16,8,0x80000000",initial);
        unsigned rows = 16, group = 8; std::uint64_t base = 0;
        dialog.validate = [&](const std::wstring& value,std::wstring* error) { const auto first = value.find(L','), second = first == std::wstring::npos ? first : value.find(L',',first + 1); if (second == std::wstring::npos) return false;
            std::uint64_t row_value = 0,group_value = 0; if (!simple_editor::core::ParseAddress(L"d:" + value.substr(0,first),&row_value) || !simple_editor::core::ParseAddress(L"d:" + value.substr(first + 1,second - first - 1),&group_value) || row_value > 32 || group_value > 8) return false; rows = static_cast<unsigned>(row_value); group = static_cast<unsigned>(group_value);
            if (!simple_editor::core::ParseAddress(value.substr(second + 1),&base) || (rows != 8 && rows != 16 && rows != 32) || (group != 1 && group != 2 && group != 4 && group != 8) || base > UINT64_MAX - view->DocumentSize()) { *error = L"Choose valid row/group values and a base that does not overflow the address space."; return false; } return true; };
        path_dialog_ = true; const auto selected = dialog.DoModal(); path_dialog_ = false;
        if (selected == IDOK) { view->SetLayout(rows,group,base); if (active_tab_ == 2) { auto* other = view == &compare_left_ ? static_cast<CHexView*>(&compare_right_) : static_cast<CHexView*>(&compare_left_); other->SetLayout(rows,group,other->BaseAddress()); } } return;
    }
    auto* editor = ActiveEditor(); if (!editor || !editor->GetSafeHwnd()) { status_.SetPaneText(0,L"Choose the Text editor or a merged output for text-view commands."); return; }
    switch (command) {
    case ID_VIEW_WRAP: editor->SetWrap(editor->Call(SCI_GETWRAPMODE) == SC_WRAP_NONE); break;
    case ID_VIEW_WHITESPACE: editor->SetWhitespace(editor->Call(SCI_GETVIEWWS) == SCWS_INVISIBLE); break;
    case ID_VIEW_DARK:
        dark_content_ = !dark_content_; content_theme_ = dark_content_ ? 1 : 0; text_.SetTheme(content_theme_); if (text_clone_.GetSafeHwnd()) text_clone_.SetTheme(content_theme_); advanced_.SetAppearance(text_face_,content_font_tenths_,content_theme_);
        for (CHexView* view : {static_cast<CHexView*>(&hex_),static_cast<CHexView*>(&compare_left_),static_cast<CHexView*>(&compare_right_)}) view->SetAppearance(text_face_,content_font_tenths_,content_theme_); break;
    case ID_VIEW_BOOKMARK: editor->ToggleBookmark(); break;
    case ID_VIEW_BOOKMARK_NEXT: editor->NextBookmark(); break; case ID_VIEW_BOOKMARK_PREVIOUS: editor->NextBookmark(true); break;
    case ID_VIEW_BOOKMARK_CLEAR: editor->Call(SCI_MARKERDELETEALL,20); break;
    case ID_VIEW_FOLD: editor->Call(SCI_FOLDALL,SC_FOLDACTION_CONTRACT); break; case ID_VIEW_UNFOLD: editor->Call(SCI_FOLDALL,SC_FOLDACTION_EXPAND); break;
    case ID_VIEW_COLUMN: editor->Call(SCI_SETSELECTIONMODE,editor->Call(SCI_GETSELECTIONMODE) == SC_SEL_RECTANGLE ? SC_SEL_STREAM : SC_SEL_RECTANGLE); break;
    case ID_VIEW_MULTI_NEXT: editor->Call(SCI_SETTARGETRANGE,0,editor->Call(SCI_GETLENGTH)); editor->Call(SCI_MULTIPLESELECTADDNEXT); break;
    case ID_VIEW_COMPLETE: editor->Call(SCI_AUTOCSHOW,0,reinterpret_cast<LPARAM>("auto bool break case catch class const continue def double else enum false float for from function if import int let namespace new nullptr private public return static string struct switch true try var void while")); break;
    case ID_VIEW_GOTO_LINE: {
        if (IsBusy()) return; const auto count = editor->GetLineCount();
        CValueDialog dialog(this,L"Go to line",L"Line number (decimal, starting at 1). Maximum: " + std::to_wstring(count),L"1");
        std::uint64_t line = 1; dialog.validate = [&](const std::wstring& value,std::wstring*) { if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos) return false; try { line = std::stoull(value); return line && line <= static_cast<std::uint64_t>(count); } catch (...) { return false; } };
        path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer == IDOK) editor->GoToLine(line); break;
    }
    case ID_VIEW_TABS: {
        if (IsBusy()) return; CValueDialog dialog(this,L"Tab settings",L"Tab width 1-16. Add the word tabs to insert Tab characters, otherwise spaces are used. Example: 4 spaces",L"4 spaces");
        unsigned width = 4; bool tabs = false; dialog.validate = [&](const std::wstring& value,std::wstring*) { std::wistringstream input(value); std::wstring mode; if (!(input >> width) || width < 1 || width > 16) return false; input >> mode; tabs = mode == L"tabs"; return mode.empty() || mode == L"tabs" || mode == L"spaces"; };
        path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer == IDOK) { editor->SetTabWidth(width,tabs); if (IsFileEditor(editor)) { indentation_manual_ = true; manual_indentation_ = {width,width,tabs}; } } break;
    }
    case ID_VIEW_FONT: {
        if (IsBusy() || !IsFileEditor(editor)) { status_.SetPaneText(0,L"Use comparison zoom for the merged-output font; text font selection applies to the file editor."); return; }
        LOGFONT font{}; text_.GetFont()->GetLogFont(&font); CFontDialog dialog(&font,CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT,nullptr,this);
        path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false;
        if (answer == IDOK) { if (dialog.GetSize() < 60 || dialog.GetSize() > 320) { status_.SetPaneText(0,L"Choose a base font size from 6 to 32 pt; use content zoom for larger display."); return; } dialog.GetCurrentFont(&font); text_face_ = font.lfFaceName; content_font_tenths_ = dialog.GetSize(); const auto percent = zoom_percent_[1]; zoom_percent_[1] = -1; SetTabZoom(1,percent); } break;
    }
    }
    if (workspace_) workspace_->RememberView(this);
    if (!IsBusy() && editor == ActiveEditor()) status_.SetPaneText(0,editor->Status().c_str());
}

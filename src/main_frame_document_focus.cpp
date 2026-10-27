#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include "ui/localization.h"
#include <algorithm>

void CMainFrame::AppendDocumentCommands(CMenu& menu) {
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_CLIP_COMPARE,L"Compare selection with clipboard...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_ALIGN_ADD,L"Align selected source lines");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_ALIGN_MANAGE,L"Manage alignment points...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_FOLDER_REFRESH,L"Refresh selected folder rows");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_SYNTAX,L"Choose syntax mode...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_ASSOCIATIONS,L"Syntax extension associations...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_FUNCTIONS,L"Function sidebar / refresh");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_PREVIOUS_FILE,L"Previous differing file");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_NEXT_FILE,L"Next differing file");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_PARENT,L"Return to parent folder comparison");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_NOTE,L"Annotate selected difference...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_NOTES_LOAD,L"Load review notes...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_NOTES_SAVE,L"Save review notes...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_NOTES_REPORT,L"Export annotated HTML...");
    menu.AppendMenu(MF_STRING,ID_DOCUMENT_NOTES_CLEAR,L"Clear review notes...");
    menu.AppendMenu(MF_SEPARATOR);
}
void CMainFrame::OnDocumentFocus(UINT command) {
    if (command >= ID_DOCUMENT_NOTE && command <= ID_DOCUMENT_NOTES_CLEAR) { if (AdvancedActive()) advanced_.NotesCommand(command); return; }
    if (command >= ID_DOCUMENT_PREVIOUS_FILE && command <= ID_DOCUMENT_PARENT) { if (workspace_) workspace_->NavigateFolderPair(this,command == ID_DOCUMENT_PARENT ? 0 : command == ID_DOCUMENT_PREVIOUS_FILE ? -1 : 1); return; }
    if (command == ID_DOCUMENT_SYNTAX) { ChooseSyntax(); return; }
    if (command == ID_DOCUMENT_ASSOCIATIONS) { ChooseSyntaxAssociations(); return; }
    if (command == ID_DOCUMENT_FUNCTIONS) { StartFunctionOutline(); return; }
    if (command == ID_DOCUMENT_FOLDER_REFRESH) { if (AdvancedActive()) advanced_.RefreshSelectedFolders(); return; }
    if (command == ID_DOCUMENT_ALIGN_ADD || command == ID_DOCUMENT_ALIGN_MANAGE) {
        if (!AdvancedActive()) { status_.SetPaneText(0,L"Choose a two-way file comparison first."); return; }
        if (command == ID_DOCUMENT_ALIGN_ADD) advanced_.AddSelectedAnchor(); else advanced_.ManageAnchors(); return;
    }
    if (command == ID_DOCUMENT_CLIP_COMPARE) {
        auto* editor = ActiveEditor(); if (IsBusy() || !workspace_ || !editor) return;
        const auto count = editor->Call(SCI_GETSELTEXT);
        if (count <= 1 || count > 16 * 1024 * 1024 + 1) { status_.SetPaneText(0,L"Select up to 16 MiB of Text before comparing with the clipboard."); return; }
        std::string selected(static_cast<std::size_t>(count),0); if (editor->Call(SCI_GETSELTEXT,0,reinterpret_cast<LPARAM>(selected.data())) != count) return; selected.resize(static_cast<std::size_t>(count - 1));
        std::wstring clipboard; bool valid = false;
        if (::OpenClipboard(GetSafeHwnd())) {
            const auto memory = GetClipboardData(CF_UNICODETEXT); const auto size = memory ? GlobalSize(memory) : 0;
            const auto* text = memory && size >= 2 && size <= 32 * 1024 * 1024 + 2 ? static_cast<const wchar_t*>(GlobalLock(memory)) : nullptr;
            if (text) { const auto* last = std::find(text,text + size / 2,L'\0'); if (last != text + size / 2) { clipboard.assign(text,last); valid = true; } GlobalUnlock(memory); } CloseClipboard();
        }
        std::wstring error; if (!valid) error = L"The clipboard needs Unicode text within the comparison limit.";
        if (!valid || !workspace_->NewPastedPair(CCodeEditor::FromUtf8(selected),clipboard,&error)) { UiMessageBox(error.c_str(),MB_ICONWARNING); return; }
    }
}

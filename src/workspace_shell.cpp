#include "workspace_frame.h"
#include "ui/localization.h"
#include "resource.h"
#include "services/explorer_menu.h"

void CWorkspaceFrame::AppendExplorerMenu(CMenu& parent) {
    CMenu menu; menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING,ID_EXPLORER_ADD,L"Add file context menu");
    menu.AppendMenu(MF_STRING,ID_EXPLORER_REMOVE,L"Remove file context menu");
    parent.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(menu.Detach()),L"Windows Explorer");
}
void CWorkspaceFrame::OnExplorerMenu(UINT command) {
    if (workspace_dialog_ || closing_all_ || destroying_) return;
    workspace_dialog_ = true; std::wstring error;
    const bool add = command == ID_EXPLORER_ADD;
    const bool ok = simple_editor::shell::SetUserExplorerMenu(simple_editor::shell::CurrentExecutable(),add,&error);
    UiMessageBox(ok ? add ? L"Explorer file menu added for the current user.\nRight-click a file and choose \x4EE5 Simple Editor Tool \x958B\x555F.\nOn Windows 11, use Show more options if needed.\nIf you move the EXE, add the menu again from the new location." : L"The Simple Editor Tool file context menu was removed for the current user." : error.c_str(),ok ? MB_OK | MB_ICONINFORMATION : MB_OK | MB_ICONERROR);
    workspace_dialog_ = false;
}
void CWorkspaceFrame::OnUpdateExplorerMenu(CCmdUI* command) {
    simple_editor::shell::ExplorerMenuState state; std::wstring error;
    const bool known = simple_editor::shell::ReadUserExplorerMenu(&state,&error);
    command->Enable(!workspace_dialog_ && !closing_all_ && !destroying_ && (command->m_nID == ID_EXPLORER_ADD || (known && state.owned)));
    if (command->m_nID == ID_EXPLORER_ADD) command->SetCheck(known && state.owned && _wcsicmp(state.executable.c_str(),simple_editor::shell::CurrentExecutable().c_str()) == 0);
}

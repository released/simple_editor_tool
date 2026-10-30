#pragma once
#include <afxwin.h>
#include <Shellapi.h>
#include <Shlobj.h>

namespace simple_editor::ui {
inline void DropPointToScreen(HDROP drop, HWND source) {
    POINT point{};
    if (!DragQueryPoint(drop,&point)) return;
    if (!::ClientToScreen(source,&point) || GlobalSize(drop) < sizeof(DROPFILES)) return;
    auto* files = static_cast<DROPFILES*>(GlobalLock(drop));
    if (!files) return;
    files->pt = point; files->fNC = TRUE; GlobalUnlock(drop);
}
}

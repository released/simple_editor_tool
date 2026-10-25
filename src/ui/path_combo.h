#pragma once
#include <afxwin.h>
#include <string>

inline void SetPathComboText(CComboBox& box, const std::wstring& path) {
    // WM_SETTEXT on a combo may select a longer prefix match. Set its edit directly.
    box.SetCurSel(-1); COMBOBOXINFO info{}; info.cbSize = sizeof(info);
    if (::GetComboBoxInfo(box.GetSafeHwnd(),&info) && info.hwndItem) ::SetWindowTextW(info.hwndItem,path.c_str());
    else box.SetWindowTextW(path.c_str());
}

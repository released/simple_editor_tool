#include "localization_internal.h"
#include "../resource.h"
#include <algorithm>
namespace simple_editor::ui {
namespace {
std::wstring WindowText(HWND window) { const auto count = GetWindowTextLengthW(window); if (count < 0 || count > 32768) return {}; std::wstring text(static_cast<std::size_t>(count) + 1,0); GetWindowTextW(window,text.data(),count + 1); text.resize(count); return text; }
bool TextRole(LocalizedRole role) { return role == LocalizedRole::Text || role == LocalizedRole::Caption; }
bool StatusEdit(HWND window) { if (!(GetWindowLongPtrW(window,GWL_STYLE) & ES_READONLY)) return false; const int id = GetDlgCtrlID(window); return id == 567 || id == 1914 || id == 2603 || id == 2811 || id == 2301 || id == IDC_SEARCH_STATUS || id == IDC_SYNC_SUMMARY; }
std::wstring ItemText(HWND window,LocalizedRole role,int index) {
    wchar_t text[16385]{};
    if (role == LocalizedRole::Combo) { const auto count = SendMessageW(window,CB_GETLBTEXTLEN,index,0); if (count >= 0 && count <= 16384) SendMessageW(window,CB_GETLBTEXT,index,reinterpret_cast<LPARAM>(text)); }
    else if (role == LocalizedRole::Header) { HDITEMW item{}; item.mask = HDI_TEXT; item.pszText = text; item.cchTextMax = 16385; SendMessageW(window,HDM_GETITEMW,index,reinterpret_cast<LPARAM>(&item)); }
    else { TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = text; item.cchTextMax = 16385; SendMessageW(window,TCM_GETITEMW,index,reinterpret_cast<LPARAM>(&item)); }
    return text;
}
}
void AttachLocalizedWindow(HWND window) {
    auto& state = LocaleState(); if (!state.enabled || !IsWindow(window) || state.windows.count(window)) return;
    wchar_t name[64]{}; GetClassNameW(window,name,64); const auto style = GetWindowLongPtrW(window,GWL_STYLE); LocalizedWindow item;
    if (state.roots.count(window)) item.role = LocalizedRole::Caption;
    else if (_wcsicmp(name,L"Static") == 0 && (style & SS_TYPEMASK) != SS_ICON && (style & SS_TYPEMASK) != SS_BITMAP && (style & SS_TYPEMASK) != SS_OWNERDRAW) item.role = LocalizedRole::Text;
    else if (_wcsicmp(name,L"Button") == 0 && !(style & (BS_BITMAP | BS_ICON))) item.role = LocalizedRole::Text;
    else if (_wcsicmp(name,L"Edit") == 0 && StatusEdit(window)) item.role = LocalizedRole::Text;
    else if (_wcsicmp(name,L"ComboBox") == 0 && (style & 3) == CBS_DROPDOWNLIST && !(style & (CBS_SORT | CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) item.role = LocalizedRole::Combo;
    else if (_wcsicmp(name,WC_HEADERW) == 0) item.role = LocalizedRole::Header;
    else if (_wcsicmp(name,WC_TABCONTROLW) == 0 && GetDlgCtrlID(window) == 500) item.role = LocalizedRole::Pages;
    else if (_wcsicmp(name,STATUSCLASSNAMEW) == 0) item.role = LocalizedRole::Status;
    else return;
    if (TextRole(item.role)) item.source = WindowText(window);
    else if (item.role != LocalizedRole::Status) { const auto count = SendMessageW(window,item.role == LocalizedRole::Combo ? CB_GETCOUNT : item.role == LocalizedRole::Header ? HDM_GETITEMCOUNT : TCM_GETITEMCOUNT,0,0); if (count < 0 || count > 512) return; for (int i = 0; i < count; ++i) item.items.push_back(ItemText(window,item.role,i)); }
    else { const auto count = SendMessageW(window,SB_GETPARTS,0,0); for (int i = 0; i < count && i < 32; ++i) { const auto length = SendMessageW(window,SB_GETTEXTLENGTHW,i,0); if ((HIWORD(length) & SBT_OWNERDRAW) || LOWORD(length) > 32768) continue; std::wstring text(LOWORD(length) + 1,0); SendMessageW(window,SB_GETTEXTW,i,reinterpret_cast<LPARAM>(text.data())); text.resize(LOWORD(length)); item.panes[i | (HIWORD(length) & ~SBT_OWNERDRAW)] = std::move(text); } }
    if (!SetWindowSubclass(window,LocalizedControlProc,0x53454C31,0)) return;
    state.windows.emplace(window,std::move(item)); RefreshLocalizedWindow(window);
}
void RefreshLocalizedWindow(HWND window) {
    auto& state = LocaleState(); const auto found = state.windows.find(window); if (found == state.windows.end()) return; const auto copy = found->second; const bool previous = state.rendering; state.rendering = true;
    if (TextRole(copy.role)) { const auto text = Localize(copy.source); SetWindowTextW(window,text.c_str()); }
    else if (copy.role == LocalizedRole::Status) for (const auto& pane : copy.panes) { const auto text = Localize(pane.second); SendMessageW(window,SB_SETTEXTW,pane.first,reinterpret_cast<LPARAM>(text.c_str())); }
    else if (copy.role == LocalizedRole::Combo) {
        const auto selection = SendMessageW(window,CB_GETCURSEL,0,0); std::vector<LPARAM> data;
        for (std::size_t i = 0; i < copy.items.size(); ++i) data.push_back(SendMessageW(window,CB_GETITEMDATA,i,0));
        SendMessageW(window,WM_SETREDRAW,FALSE,0); SendMessageW(window,CB_RESETCONTENT,0,0);
        for (std::size_t i = 0; i < copy.items.size(); ++i) { const auto text = Localize(copy.items[i]); const auto index = SendMessageW(window,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str())); if (index >= 0) SendMessageW(window,CB_SETITEMDATA,index,data[i]); }
        SendMessageW(window,CB_SETCURSEL,selection,0); SendMessageW(window,WM_SETREDRAW,TRUE,0); InvalidateRect(window,nullptr,FALSE);
    } else for (std::size_t i = 0; i < copy.items.size(); ++i) { const auto text = Localize(copy.items[i]); if (copy.role == LocalizedRole::Header) { HDITEMW item{}; item.mask = HDI_TEXT; item.pszText = const_cast<wchar_t*>(text.c_str()); SendMessageW(window,HDM_SETITEMW,i,reinterpret_cast<LPARAM>(&item)); } else { TCITEMW item{}; item.mask = TCIF_TEXT; item.pszText = const_cast<wchar_t*>(text.c_str()); SendMessageW(window,TCM_SETITEMW,i,reinterpret_cast<LPARAM>(&item)); } }
    state.rendering = previous;
}
LRESULT CALLBACK LocalizedControlProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR,DWORD_PTR) {
    auto& state = LocaleState(); auto found = state.windows.find(window);
    if (message == WM_NCDESTROY) { RemoveWindowSubclass(window,LocalizedControlProc,0x53454C31); state.windows.erase(window); state.roots.erase(window); std::set<HMENU> retired; for (auto menu = state.menus.begin(); menu != state.menus.end();) { if (menu->second == window || !IsMenu(menu->first)) { retired.insert(menu->first); menu = state.menus.erase(menu); } else ++menu; } for (auto label = state.menu_items.begin(); label != state.menu_items.end();) { if (retired.count(label->first.first) || !IsMenu(label->first.first)) label = state.menu_items.erase(label); else ++label; } return DefSubclassProc(window,message,wparam,lparam); }
    if (found == state.windows.end() || state.rendering) return DefSubclassProc(window,message,wparam,lparam);
    auto& item = found->second;
    if (TextRole(item.role) && message == WM_SETTEXT) { item.source = lparam ? reinterpret_cast<const wchar_t*>(lparam) : L""; const auto text = Localize(item.source); return DefSubclassProc(window,message,wparam,reinterpret_cast<LPARAM>(text.c_str())); }
    if (item.role == LocalizedRole::Status && message == SB_SETTEXTW && !(wparam & SBT_OWNERDRAW)) { item.panes[wparam] = lparam ? reinterpret_cast<const wchar_t*>(lparam) : L""; const auto text = Localize(item.panes[wparam]); return DefSubclassProc(window,message,wparam,reinterpret_cast<LPARAM>(text.c_str())); }
    if (item.role == LocalizedRole::Combo) {
        if (message == CB_RESETCONTENT) item.items.clear();
        if (message == CB_DELETESTRING && wparam < item.items.size()) item.items.erase(item.items.begin() + wparam);
        if ((message == CB_ADDSTRING || message == CB_INSERTSTRING) && lparam) { const std::wstring source = reinterpret_cast<const wchar_t*>(lparam); const auto text = Localize(source); const auto index = DefSubclassProc(window,message,wparam,reinterpret_cast<LPARAM>(text.c_str())); if (index >= 0 && static_cast<std::size_t>(index) <= item.items.size()) item.items.insert(item.items.begin() + index,source); return index; }
    } else if (item.role == LocalizedRole::Header && (message == HDM_INSERTITEMW || message == HDM_SETITEMW) && lparam) {
        auto value = *reinterpret_cast<const HDITEMW*>(lparam); if ((value.mask & HDI_TEXT) && value.pszText && value.pszText != LPSTR_TEXTCALLBACKW) { const std::wstring source = value.pszText; const auto text = Localize(source); value.pszText = const_cast<wchar_t*>(text.c_str()); const auto result = DefSubclassProc(window,message,wparam,reinterpret_cast<LPARAM>(&value)); if (message == HDM_INSERTITEMW && result >= 0 && static_cast<std::size_t>(result) <= item.items.size()) item.items.insert(item.items.begin() + result,source); else if (result && wparam < item.items.size()) item.items[wparam] = source; return result; }
    } else if (item.role == LocalizedRole::Header && message == HDM_DELETEITEM && wparam < item.items.size()) item.items.erase(item.items.begin() + wparam);
    else if (item.role == LocalizedRole::Pages && (message == TCM_INSERTITEMW || message == TCM_SETITEMW) && lparam) {
        auto value = *reinterpret_cast<const TCITEMW*>(lparam); if ((value.mask & TCIF_TEXT) && value.pszText && value.pszText != LPSTR_TEXTCALLBACKW) { const std::wstring source = value.pszText; const auto text = Localize(source); value.pszText = const_cast<wchar_t*>(text.c_str()); const auto result = DefSubclassProc(window,message,wparam,reinterpret_cast<LPARAM>(&value)); if (message == TCM_INSERTITEMW && result >= 0 && static_cast<std::size_t>(result) <= item.items.size()) item.items.insert(item.items.begin() + result,source); else if (result && wparam < item.items.size()) item.items[wparam] = source; return result; }
    }
    return DefSubclassProc(window,message,wparam,lparam);
}
}

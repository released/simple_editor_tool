#pragma once
#include "localization.h"
#include "../core/language_catalog.h"
#include <map>
#include <set>
#include <commctrl.h>
namespace simple_editor::ui {
enum class LocalizedRole { Caption,Text,Combo,Header,Pages,Status };
struct LocalizedWindow {
    LocalizedRole role = LocalizedRole::Text;
    std::wstring source;
    std::vector<std::wstring> items;
    std::map<WPARAM,std::wstring> panes;
};
struct LocalizedMenuItem { std::wstring source,rendered; };
struct LocalizationState {
    core::LanguageCatalog catalog;
    bool enabled = false,rendering = false;
    HHOOK hook = nullptr;
    DWORD thread = 0;
    std::set<HWND> roots;
    std::map<HWND,LocalizedWindow> windows;
    std::map<HMENU,HWND> menus;
    std::map<std::pair<HMENU,UINT_PTR>,LocalizedMenuItem> menu_items;
};
LocalizationState& LocaleState();
void AttachLocalizedWindow(HWND);
void RefreshLocalizedWindow(HWND);
void RefreshAllLocalizedUi();
LRESULT CALLBACK LocalizedControlProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
}

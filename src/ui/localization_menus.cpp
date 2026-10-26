#include "localization_internal.h"
#include "../resource.h"
#include <algorithm>
namespace simple_editor::ui {
void LocalizeMenu(HMENU menu) {
    auto& state = LocaleState(); if (!state.enabled || !menu || !IsMenu(menu) || state.rendering) return;
    const auto registered = state.menus.find(menu); const HWND owner = registered == state.menus.end() ? GetAncestor(GetActiveWindow(),GA_ROOT) : registered->second;
    if (owner) state.menus[menu] = owner;
    state.rendering = true;
    const auto visit = [&](const auto& self,HMENU current,unsigned depth) -> void {
        if (!IsMenu(current) || depth > 10) return; if (owner) state.menus[current] = owner;
        const int count = GetMenuItemCount(current);
        for (int i = 0; i < count; ++i) { wchar_t text[32769]{}; MENUITEMINFOW info{sizeof(info)}; info.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_FTYPE; info.dwTypeData = text; info.cch = 32768; if (!GetMenuItemInfoW(current,i,TRUE,&info) || info.fType & (MFT_SEPARATOR | MFT_OWNERDRAW)) continue;
            if (info.hSubMenu) self(self,info.hSubMenu,depth + 1);
            if (info.wID >= ID_WORKSPACE_RECENT_FIRST && info.wID <= ID_WORKSPACE_RECENT_LAST) continue;
            auto& label = state.menu_items[{current,info.hSubMenu ? reinterpret_cast<UINT_PTR>(info.hSubMenu) : static_cast<UINT_PTR>(info.wID)}]; const std::wstring actual = text;
            if (label.source.empty()) label.source = actual;
            else if (actual != label.rendered && actual != label.source) { const auto actual_tab = actual.find(L'\t'),rendered_tab = label.rendered.find(L'\t'); if (actual.substr(0,actual_tab) == label.rendered.substr(0,rendered_tab)) label.source = label.source.substr(0,label.source.find(L'\t')) + (actual_tab == std::wstring::npos ? L"" : actual.substr(actual_tab)); else label.source = actual; }
            const auto tab = label.source.find(L'\t'); const auto caption = label.source.substr(0,tab); label.rendered = Localize(caption) + (tab == std::wstring::npos ? L"" : label.source.substr(tab));
            MENUITEMINFOW update{sizeof(update)}; update.fMask = MIIM_STRING; update.dwTypeData = const_cast<wchar_t*>(label.rendered.c_str()); SetMenuItemInfoW(current,i,TRUE,&update);
        }
        CheckMenuRadioItem(current,ID_LANGUAGE_ENGLISH,ID_LANGUAGE_TRADITIONAL,UiLanguage() == L"tw" ? ID_LANGUAGE_TRADITIONAL : ID_LANGUAGE_ENGLISH,MF_BYCOMMAND);
    }; visit(visit,menu,0); state.rendering = false;
}
void RegisterLocalizationMenu(HMENU menu,HWND owner) { if (!LocalizationEnabled() || !menu) return; LocaleState().menus[menu] = owner; LocalizeMenu(menu); }
void AppendLanguageMenu(CMenu& menu) { CMenu language; language.CreatePopupMenu(); language.AppendMenu(MF_STRING,ID_LANGUAGE_ENGLISH,L"English"); language.AppendMenu(MF_STRING,ID_LANGUAGE_TRADITIONAL,L"繁體中文"); menu.AppendMenu(MF_POPUP,reinterpret_cast<UINT_PTR>(language.Detach()),L"Language / 語系"); }
}

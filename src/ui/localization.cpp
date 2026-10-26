#include "localization_internal.h"
#include "../resource.h"
#include <afxdlgs.h>
#include <commctrl.h>
namespace simple_editor::ui {
LocalizationState& LocaleState() { static LocalizationState value; return value; }
std::wstring Localize(const std::wstring& source) { const auto& state = LocaleState(); return state.enabled ? state.catalog.Translate(source) : source; }
bool LocalizationEnabled() { return LocaleState().enabled; }
const std::wstring& UiLanguage() { return LocaleState().catalog.Selected(); }
namespace {
LRESULT CALLBACK ObserveUi(int code,WPARAM wparam,LPARAM lparam) {
    auto& state = LocaleState();
    if (code >= 0 && !state.rendering) {
        const auto* message = reinterpret_cast<CWPRETSTRUCT*>(lparam);
        if (message->message == WM_INITDIALOG) {
            auto* window = CWnd::FromHandlePermanent(message->hwnd);
            if (window && window->IsKindOf(RUNTIME_CLASS(CDialog)) && !window->IsKindOf(RUNTIME_CLASS(CFileDialog)) && !window->IsKindOf(RUNTIME_CLASS(CFontDialog)) && !window->IsKindOf(RUNTIME_CLASS(CColorDialog))) RegisterLocalizationRoot(window);
        } else if ((message->message == WM_INITMENUPOPUP || message->message == WM_INITMENU) && (state.roots.count(message->hwnd) || state.roots.count(GetAncestor(message->hwnd,GA_ROOT)))) LocalizeMenu(reinterpret_cast<HMENU>(message->wParam));
    }
    return CallNextHookEx(state.hook,code,wparam,lparam);
}
BOOL CALLBACK AttachChild(HWND window,LPARAM) { AttachLocalizedWindow(window); return TRUE; }
}
bool InitializeLocalization(const std::filesystem::path& path,const std::wstring& language,std::wstring* error) {
    auto& state = LocaleState(); if (state.enabled) return SetUiLanguage(language);
    state.thread = GetCurrentThreadId(); state.catalog = core::LanguageCatalog{}; const bool loaded = state.catalog.Load(path,error); state.catalog.Select(loaded ? language : L"en"); state.enabled = true;
    state.hook = SetWindowsHookExW(WH_CALLWNDPROCRET,ObserveUi,nullptr,state.thread);
    if (!state.hook) { state.enabled = false; if (error) *error = L"Could not initialize interface localization."; return false; }
    return loaded;
}
void RegisterLocalizationRoot(CWnd* window) {
    auto& state = LocaleState(); if (!state.enabled || state.rendering || !window || !window->GetSafeHwnd() || GetCurrentThreadId() != state.thread) return;
    state.roots.insert(window->GetSafeHwnd()); AttachLocalizedWindow(window->GetSafeHwnd()); EnumChildWindows(window->GetSafeHwnd(),AttachChild,0); LocalizeMenu(::GetMenu(window->GetSafeHwnd()));
}
void RefreshAllLocalizedUi() {
    auto& state = LocaleState(); if (!state.enabled) return;
    std::vector<HWND> windows; for (const auto& item : state.windows) windows.push_back(item.first);
    for (const auto window : windows) if (IsWindow(window)) RefreshLocalizedWindow(window);
    for (auto item = state.menus.begin(); item != state.menus.end();) { if (!IsMenu(item->first) || !IsWindow(item->second)) item = state.menus.erase(item); else { LocalizeMenu(item->first); ++item; } }
    for (const auto root : state.roots) if (IsWindow(root)) { DrawMenuBar(root); RedrawWindow(root,nullptr,nullptr,RDW_INVALIDATE | RDW_ALLCHILDREN); }
}
bool SetUiLanguage(const std::wstring& language) {
    auto& state = LocaleState(); if (!state.enabled || GetCurrentThreadId() != state.thread || (language == L"tw" && !state.catalog.Entries()) || !state.catalog.Select(language)) return false;
    RefreshAllLocalizedUi(); return true;
}
void ShutdownLocalization() {
    auto& state = LocaleState(); if (!state.enabled) return;
    state.rendering = true; if (state.hook) UnhookWindowsHookEx(state.hook); state.hook = nullptr;
    for (const auto& item : state.windows) if (IsWindow(item.first)) RemoveWindowSubclass(item.first,LocalizedControlProc,0x53454C31);
    state.windows.clear(); state.roots.clear(); state.menus.clear(); state.menu_items.clear(); state.enabled = false; state.rendering = false;
}
}
int UiMessageBox(LPCTSTR text,UINT type,UINT help) { const auto rendered = simple_editor::ui::Localize(text ? text : L""); return AfxMessageBox(rendered.c_str(),type,help); }

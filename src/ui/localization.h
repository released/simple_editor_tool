#pragma once
#include <afxwin.h>
#include <filesystem>
#include <string>
namespace simple_editor::ui {
bool InitializeLocalization(const std::filesystem::path&,const std::wstring& language,std::wstring* error);
void ShutdownLocalization();
bool LocalizationEnabled();
const std::wstring& UiLanguage();
bool SetUiLanguage(const std::wstring&);
std::wstring Localize(const std::wstring&);
void RegisterLocalizationRoot(CWnd*);
void RegisterLocalizationMenu(HMENU,HWND owner);
void LocalizeMenu(HMENU);
void AppendLanguageMenu(CMenu&);
}
int UiMessageBox(LPCTSTR text,UINT type = MB_OK,UINT help = 0);

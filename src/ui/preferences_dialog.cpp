#include "preferences_dialog.h"
#include "../resource.h"
using namespace simple_editor::core;
CPreferencesDialog::CPreferencesDialog(CWnd* owner,const AppPreferences& initial) : CDialog(IDD_PREFERENCES,owner),value(initial) {}
void CPreferencesDialog::FontLabel() { SetDlgItemTextW(2001,(value.font + L", " + std::to_wstring(value.font_tenths / 10) + L"." + std::to_wstring(value.font_tenths % 10) + L" pt").c_str()); }
BOOL CPreferencesDialog::OnInitDialog() {
    CDialog::OnInitDialog(); FontLabel(); theme_.SubclassDlgItem(2002,this); for (auto text : {L"Light content",L"Dark content",L"System contrast colors"}) theme_.AddString(text); theme_.SetCurSel(value.theme);
    CheckDlgButton(2003,value.wrap); CheckDlgButton(2004,value.whitespace); CheckDlgButton(2005,value.use_tabs); SetDlgItemInt(2006,value.tab_width,FALSE);
    CheckDlgButton(2007,value.auto_restore); CheckDlgButton(2008,value.recover_drafts); SetDlgItemInt(2009,value.snapshot_seconds,FALSE);
    for (int i = 0; i < 3; ++i) SetDlgItemInt(2010 + i,value.zoom[i],FALSE);
    for (std::size_t i = 0; i < value.shortcuts.size(); ++i) SetDlgItemTextW(2020 + static_cast<int>(i),FormatShortcut(value.shortcuts[i]).c_str()); return TRUE;
}
BOOL CPreferencesDialog::OnCommand(WPARAM wparam,LPARAM lparam) {
    if (LOWORD(wparam) == 2000) {
        LOGFONT font{}; CClientDC dc(this); font.lfHeight = -MulDiv(value.font_tenths,dc.GetDeviceCaps(LOGPIXELSY),720); wcsncpy_s(font.lfFaceName,value.font.c_str(),_TRUNCATE);
        CFontDialog dialog(&font,CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_INITTOLOGFONTSTRUCT,nullptr,this); if (dialog.DoModal() == IDOK) { dialog.GetCurrentFont(&font); value.font = font.lfFaceName; value.font_tenths = static_cast<unsigned>(dialog.GetSize()); FontLabel(); } return TRUE;
    }
    return CDialog::OnCommand(wparam,lparam);
}
void CPreferencesDialog::OnOK() {
    value.theme = static_cast<unsigned>(theme_.GetCurSel()); value.wrap = IsDlgButtonChecked(2003) != 0; value.whitespace = IsDlgButtonChecked(2004) != 0; value.use_tabs = IsDlgButtonChecked(2005) != 0; value.tab_width = GetDlgItemInt(2006,nullptr,FALSE);
    value.auto_restore = IsDlgButtonChecked(2007) != 0; value.recover_drafts = IsDlgButtonChecked(2008) != 0; value.snapshot_seconds = GetDlgItemInt(2009,nullptr,FALSE);
    for (int i = 0; i < 3; ++i) value.zoom[i] = static_cast<int>(GetDlgItemInt(2010 + i,nullptr,FALSE));
    std::wstring error; for (std::size_t i = 0; i < value.shortcuts.size(); ++i) { CString text; GetDlgItemTextW(2020 + static_cast<int>(i),text); if (!ParseShortcut(text.GetString(),&value.shortcuts[i],&error)) { AfxMessageBox(error.c_str(),MB_ICONWARNING); return; } }
    if (!ValidatePreferences(value,&error)) { AfxMessageBox(error.c_str(),MB_ICONWARNING); return; } CDialog::OnOK();
}

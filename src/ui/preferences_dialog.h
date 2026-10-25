#pragma once
#include <afxdlgs.h>
#include "../core/portable_preferences.h"
class CPreferencesDialog : public CDialog {
public:
    CPreferencesDialog(CWnd* owner,const simple_editor::core::AppPreferences& value);
    simple_editor::core::AppPreferences value;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
private:
    void FontLabel();
    CComboBox theme_;
};

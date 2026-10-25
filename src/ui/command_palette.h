#pragma once
#include <afxcmn.h>
#include <functional>
#include <vector>
#include <string>
struct PaletteCommand { UINT id = 0; std::wstring name; bool enabled = true; };
std::vector<PaletteCommand> CollectPaletteCommands(CMenu* menu);
class CCommandPalette : public CDialog {
public:
    CCommandPalette(CWnd* owner,std::vector<PaletteCommand> commands);
    UINT selected = 0;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
private:
    void Filter();
    CListCtrl list_; std::vector<PaletteCommand> commands_; std::vector<std::size_t> shown_;
};

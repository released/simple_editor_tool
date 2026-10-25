#include "command_palette.h"
#include "../resource.h"
#include <set>
#include <algorithm>
#include <climits>
#include <cwctype>
namespace {
std::wstring Fold(std::wstring value) { for (auto& ch : value) ch = static_cast<wchar_t>(towlower(ch)); return value; }
}
std::vector<PaletteCommand> CollectPaletteCommands(CMenu* menu) {
    std::vector<PaletteCommand> result; std::set<UINT> seen; std::function<void(CMenu*,std::wstring,unsigned)> visit = [&](CMenu* current,std::wstring prefix,unsigned depth) { if (!current || depth > 8 || result.size() >= 512) return; for (int i = 0; i < current->GetMenuItemCount(); ++i) { CString label; current->GetMenuString(i,label,MF_BYPOSITION); std::wstring name = label.GetString(); name.erase(std::remove(name.begin(),name.end(),L'&'),name.end()); if (name.empty()) continue; const auto path = prefix.empty() ? name : prefix + L" / " + name; if (auto* sub = current->GetSubMenu(i)) visit(sub,path,depth + 1); else { const auto id = current->GetMenuItemID(i); if (id && id != UINT_MAX && seen.insert(id).second && result.size() < 512) result.push_back({id,path,(current->GetMenuState(i,MF_BYPOSITION) & (MF_DISABLED | MF_GRAYED)) == 0}); } } }; visit(menu,L"",0); return result;
}
CCommandPalette::CCommandPalette(CWnd* owner,std::vector<PaletteCommand> commands) : CDialog(IDD_COMMAND_PALETTE,owner),commands_(std::move(commands)) {}
BOOL CCommandPalette::OnInitDialog() { CDialog::OnInitDialog(); list_.SubclassDlgItem(2901,this); list_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER); CRect area; list_.GetClientRect(area); list_.InsertColumn(0,L"Command / shortcut",LVCFMT_LEFT,(std::max)(200,area.Width() - 100)); list_.InsertColumn(1,L"Available",LVCFMT_LEFT,90); static_cast<CEdit*>(GetDlgItem(2900))->SetLimitText(256); Filter(); return TRUE; }
void CCommandPalette::Filter() { CString text; GetDlgItemTextW(2900,text); const auto query = Fold(text.GetString()); list_.DeleteAllItems(); shown_.clear(); for (std::size_t i = 0; i < commands_.size(); ++i) if (Fold(commands_[i].name).find(query) != std::wstring::npos) { const auto row = list_.InsertItem(static_cast<int>(shown_.size()),commands_[i].name.c_str()); list_.SetItemText(row,1,commands_[i].enabled ? L"Yes" : L"Unavailable"); shown_.push_back(i); } if (!shown_.empty()) list_.SetItemState(0,LVIS_SELECTED | LVIS_FOCUSED,LVIS_SELECTED | LVIS_FOCUSED); SetDlgItemTextW(2902,(std::to_wstring(shown_.size()) + L" commands. Context and busy/read-only restrictions still apply.").c_str()); }
BOOL CCommandPalette::OnCommand(WPARAM wp,LPARAM lp) { if (LOWORD(wp) == 2900 && HIWORD(wp) == EN_CHANGE && list_.GetSafeHwnd()) { Filter(); return TRUE; } return CDialog::OnCommand(wp,lp); }
void CCommandPalette::OnOK() { const auto row = list_.GetNextItem(-1,LVNI_SELECTED); if (row < 0 || static_cast<std::size_t>(row) >= shown_.size()) return; const auto& command = commands_[shown_[row]]; if (!command.enabled) { SetDlgItemTextW(2902,L"This command is unavailable in the current page/document state."); return; } selected = command.id; CDialog::OnOK(); }

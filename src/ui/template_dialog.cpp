#include "template_dialog.h"
#include "../resource.h"
#include <map>
using namespace simple_editor::core;
BEGIN_MESSAGE_MAP(CTemplateDialog,CDialog)
    ON_NOTIFY(TVN_SELCHANGED,2200,OnSelection)
END_MESSAGE_MAP()
CTemplateDialog::CTemplateDialog(CWnd* owner,const BinaryTemplateResult& fields) : CDialog(IDD_BINARY_TEMPLATE,owner),fields_(fields) {}
BOOL CTemplateDialog::OnInitDialog() {
    CDialog::OnInitDialog(); tree_.SubclassDlgItem(2200,this); tree_.SetRedraw(FALSE); std::map<std::wstring,HTREEITEM> groups;
    for (std::size_t i = 0; i < fields_.fields.size(); ++i) {
        const auto& field = fields_.fields[i]; HTREEITEM parent = TVI_ROOT; std::size_t begin = 0, dot = 0;
        while ((dot = field.path.find(L'.',begin)) != std::wstring::npos) {
            const auto key = field.path.substr(0,dot); auto found = groups.find(key);
            if (found == groups.end()) { auto node = tree_.InsertItem(field.path.substr(begin,dot - begin).c_str(),parent); tree_.SetItemData(node,0); groups.emplace(key,node); parent = node; } else parent = found->second; begin = dot + 1;
        }
        const auto label = field.path.substr(begin) + L" : " + field.type + (field.valid ? L"" : L" [outside file]"); const auto node = tree_.InsertItem(label.c_str(),parent); tree_.SetItemData(node,i + 1);
    }
    tree_.SetRedraw(TRUE); if (auto node = tree_.GetRootItem()) { tree_.Expand(node,TVE_EXPAND); tree_.SelectItem(node); } GetDlgItem(IDOK)->EnableWindow(FALSE); return TRUE;
}
void CTemplateDialog::OnSelection(NMHDR*,LRESULT* result) {
    *result = 0; selected = -1; const auto node = tree_.GetSelectedItem(); const auto data = node ? tree_.GetItemData(node) : 0;
    if (!data || data > fields_.fields.size()) { SetDlgItemTextW(2201,L"Select a field to inspect its physical byte range."); GetDlgItem(IDOK)->EnableWindow(FALSE); return; }
    selected = static_cast<int>(data - 1); const auto& field = fields_.fields[selected]; wchar_t offset[32]{}; swprintf_s(offset,L"0x%016llX",field.offset);
    const auto text = field.path + L"\r\nType: " + field.type + L"\r\nPhysical offset: " + offset + L"\r\nBytes: " + std::to_wstring(field.length) + L"\r\n\r\n" + field.value;
    SetDlgItemTextW(2201,text.c_str()); GetDlgItem(IDOK)->EnableWindow(field.valid);
}
void CTemplateDialog::OnOK() { if (selected >= 0 && static_cast<std::size_t>(selected) < fields_.fields.size() && fields_.fields[selected].valid) CDialog::OnOK(); }

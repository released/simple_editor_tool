#include "review_dialogs.h"
#include "../resource.h"
#include <algorithm>
BEGIN_MESSAGE_MAP(CMarkdownOutlineDialog,CDialog)
    ON_NOTIFY(NM_DBLCLK,IDC_OUTLINE_TREE,OnActivate)
END_MESSAGE_MAP()
CMarkdownOutlineDialog::CMarkdownOutlineDialog(CWnd* owner,const std::vector<simple_editor::core::MarkdownHeading>& headings) : CDialog(IDD_MARKDOWN_OUTLINE,owner),headings_(headings) {}
BOOL CMarkdownOutlineDialog::OnInitDialog() {
    CDialog::OnInitDialog(); if (function_list) { SetWindowTextW(L"C/C++ function outline"); SetDlgItemTextW(IDOK,L"Go to definition"); } tree_.SubclassDlgItem(IDC_OUTLINE_TREE,this); HTREEITEM parents[7]{};
    for (std::size_t i = 0; i < headings_.size(); ++i) {
        const auto level = (std::clamp)(headings_[i].level,1u,6u); auto parent = level - 1; while (parent && !parents[parent]) --parent;
        const auto item = tree_.InsertItem(headings_[i].title.c_str(),parent ? parents[parent] : TVI_ROOT,TVI_LAST); tree_.SetItemData(item,i + 1);
        parents[level] = item; for (auto deeper = level + 1; deeper < 7; ++deeper) parents[deeper] = nullptr;
        if (parent && level <= 3) tree_.Expand(parents[parent],TVE_EXPAND);
    }
    if (tree_.GetRootItem()) tree_.SelectItem(tree_.GetRootItem()); return TRUE;
}
void CMarkdownOutlineDialog::OnOK() { const auto item = tree_.GetSelectedItem(); const auto index = item ? tree_.GetItemData(item) : 0; if (!index || index > headings_.size()) return; line = headings_[index - 1].line; CDialog::OnOK(); }
void CMarkdownOutlineDialog::OnActivate(NMHDR*,LRESULT* result) { *result = 0; OnOK(); }

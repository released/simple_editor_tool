#pragma once
#include <afxcmn.h>
#include "../core/text_review.h"
class CMarkdownOutlineDialog : public CDialog {
public:
    CMarkdownOutlineDialog(CWnd* owner,const std::vector<simple_editor::core::MarkdownHeading>& headings);
    std::size_t line = 0;
    bool function_list = false;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    afx_msg void OnActivate(NMHDR*,LRESULT* result);
    DECLARE_MESSAGE_MAP()
private:
    std::vector<simple_editor::core::MarkdownHeading> headings_;
    CTreeCtrl tree_;
};

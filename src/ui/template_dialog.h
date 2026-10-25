#pragma once
#include <afxcmn.h>
#include "../core/binary_template.h"
class CTemplateDialog : public CDialog {
public:
    CTemplateDialog(CWnd* owner,const simple_editor::core::BinaryTemplateResult& fields);
    int selected = -1;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    afx_msg void OnSelection(NMHDR*,LRESULT*);
    DECLARE_MESSAGE_MAP()
private:
    const simple_editor::core::BinaryTemplateResult& fields_;
    CTreeCtrl tree_;
};

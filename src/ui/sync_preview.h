#pragma once
#include <afxcmn.h>
#include "../core/folder_policy.h"
class CSyncPreview : public CDialog {
public:
    CSyncPreview(simple_editor::core::SyncBundle bundle,CWnd* owner);
    simple_editor::core::SyncBundle Selected() const;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnNotify(WPARAM,LPARAM,LRESULT*) override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
private:
    void SkipSelected();
    struct Row { std::size_t plan,index; bool conflict,skip = false; };
    simple_editor::core::SyncBundle bundle_;
    std::vector<Row> rows_;
    CListCtrl list_;
    CButton skip_,all_;
};

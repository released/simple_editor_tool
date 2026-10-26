#pragma once
#include <afxwin.h>
#include "../core/review_notes.h"
class CReviewNoteDialog : public CDialog {
public:
    CReviewNoteDialog(CWnd* parent,const simple_editor::core::ReviewNote&,const std::wstring& location);
    simple_editor::core::ReviewNote note;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
private:
    std::wstring location_;
};

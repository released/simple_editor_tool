#pragma once
#include <afxcmn.h>
#include <functional>
#include "native_search.h"
class CSearchResultsDialog : public CDialog {
public:
    void Show(CWnd* owner,std::vector<NativeSearchMatch> matches,const std::wstring& title);
    std::function<void(const NativeSearchMatch&)> navigate;
    std::size_t Count() const { return matches_.size(); }
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override { ShowWindow(SW_HIDE); }
    afx_msg void OnDisplay(NMHDR*,LRESULT* result);
    afx_msg void OnActivate(NMHDR*,LRESULT* result);
    afx_msg void OnSize(UINT type,int cx,int cy);
    DECLARE_MESSAGE_MAP()
private:
    void Layout(int cx,int cy);
    CListCtrl list_;
    std::vector<NativeSearchMatch> matches_;
};

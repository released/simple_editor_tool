#pragma once
#include <afxcmn.h>
#include <functional>
#include <string>
#include <vector>
class CFeatureInput : public CDialog {
public:
    CFeatureInput(CWnd* owner,std::wstring title,std::wstring prompt,std::wstring initial = L"");
    std::wstring value;
    std::function<bool(const std::wstring&,std::wstring*)> validate;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
private: std::wstring title_,prompt_;
};
class CReviewList : public CDialog {
public:
    CReviewList(CWnd* owner,std::wstring title,std::vector<std::wstring> columns,std::vector<std::vector<std::wstring>> rows);
    std::vector<std::size_t> selected;
    std::wstring accept_label;
    bool allow_empty = false;
    bool single_select = false;
    std::function<void(std::size_t)> details;
    std::function<bool(std::size_t)> remove;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
    afx_msg void OnDisplay(NMHDR*,LRESULT*);
    afx_msg void OnActivate(NMHDR*,LRESULT*);
    DECLARE_MESSAGE_MAP()
private:
    std::wstring title_; std::vector<std::wstring> columns_; std::vector<std::vector<std::wstring>> rows_; CListCtrl list_;
};

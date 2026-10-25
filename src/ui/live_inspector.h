#pragma once
#include <afxcmn.h>
#include "../core/binary_analysis.h"
#include <functional>
struct LiveInspection { std::vector<simple_editor::core::InspectorValue> values; std::uint64_t offset = 0,revision = 0; bool writable = false; std::wstring error; };
class CLiveInspector : public CDialog {
public:
    void Show(CWnd* owner);
    std::function<LiveInspection(bool)> inspect;
    std::function<bool(const LiveInspection&,const std::wstring&,const std::wstring&,bool,std::wstring*)> write;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    void OnCancel() override;
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnDestroy();
    afx_msg void OnSelect(NMHDR*,LRESULT*);
    DECLARE_MESSAGE_MAP()
private:
    void Tick(); CListCtrl list_; CComboBox endian_; LiveInspection current_,editing_; std::wstring type_; bool editing_little_ = true;
};

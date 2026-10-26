#pragma once
#include "code_editor.h"
#include "../core/data_conversion.h"
#include <future>
#include "inspection_layout.h"
class CConversionDialog : public CDialog {
public:
    CConversionDialog(CWnd* owner,bool may_apply);
    simple_editor::core::DataConversion result;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    BOOL OnCommand(WPARAM,LPARAM) override;
    void OnOK() override;
    void OnCancel() override;
    afx_msg void OnTimer(UINT_PTR);
    afx_msg void OnSize(UINT,int,int);
    DECLARE_MESSAGE_MAP()
private:
    void Preview();
    void Copy();
    bool Current() const;
    void UpdateActions();
    CCodeEditor input_,output_;
    CComboBox input_format_,output_format_;
    CFont font_;
    struct Converted { simple_editor::core::DataConversion value; std::wstring error; };
    std::future<Converted> job_;
    std::atomic_bool cancelled_{false};
    std::uint64_t generation_ = UINT64_MAX;
    int input_kind_ = -1,output_kind_ = -1;
    bool may_apply_,valid_ = false;
    CInspectionLayout layout_;
};

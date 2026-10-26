#pragma once
#include <afxwin.h>
#include "../core/binary_analysis.h"
#include "inspection_layout.h"
class CBinaryPlot : public CWnd {
public:
    const simple_editor::core::BinaryAnalysis* data = nullptr;
    bool entropy = false;
    int selected = -1;
    std::function<void(unsigned)> choose;
    friend class NativeUiTest;
protected:
    afx_msg void OnPaint();
    afx_msg void OnClick(UINT,CPoint);
    afx_msg BOOL OnErase(CDC*) { return TRUE; }
    DECLARE_MESSAGE_MAP()
};
class CStatisticsDialog : public CDialog {
public:
    CStatisticsDialog(CWnd*,const simple_editor::core::BinaryAnalysis&,std::uint64_t offset);
    std::uint64_t selected_offset = 0,selected_length = 0;
    friend class NativeUiTest;
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    afx_msg void OnSize(UINT,int,int);
    DECLARE_MESSAGE_MAP()
private:
    void ChooseBlock(unsigned index);
    const simple_editor::core::BinaryAnalysis& data_;
    std::uint64_t offset_;
    CBinaryPlot histogram_,entropy_;
    CInspectionLayout layout_;
};

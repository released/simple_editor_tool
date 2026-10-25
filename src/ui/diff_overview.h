#pragma once
#include <afxwin.h>
#include <functional>
#include <vector>
class CDiffOverview : public CWnd {
public:
    BOOL Create(CWnd* parent,UINT id);
    void SetBands(std::vector<unsigned char> bands);
    void SetViewport(std::size_t first,std::size_t count);
    std::function<void(std::size_t)> navigate;
protected:
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT flags,CPoint point);
    afx_msg void OnSize(UINT type,int cx,int cy);
    DECLARE_MESSAGE_MAP()
private:
    std::vector<unsigned char> bands_, pixels_;
    std::size_t first_ = 0, count_ = 0;
};

#pragma once
#include <afxwin.h>
#include <functional>

class CSplitterBar : public CWnd {
public:
    bool vertical = true;
    std::function<void(CPoint)> move;
protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT);
    afx_msg void OnLButtonDown(UINT, CPoint);
    afx_msg void OnMouseMove(UINT, CPoint);
    afx_msg void OnLButtonUp(UINT, CPoint);
    afx_msg void OnCancelMode();
    DECLARE_MESSAGE_MAP()
};

#pragma once
#include <afxcmn.h>
#include <functional>
class CWorkspaceTabs : public CTabCtrl {
public:
    std::function<void(int)> close;
    std::function<void(int,CPoint)> context;
    std::function<void(int,int)> reorder;
protected:
    LRESULT WindowProc(UINT message,WPARAM wparam,LPARAM lparam) override;
private:
    int Hit(CPoint point) { TCHITTESTINFO info{}; info.pt = point; return HitTest(&info); }
    int pressed_ = -1;
    bool dragging_ = false;
    CPoint press_point_;
};

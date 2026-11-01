#pragma once
#include <afxcmn.h>
#include <functional>
#include <string>
class CWorkspaceTabs : public CTabCtrl {
public:
    struct LockState { bool available = false, locked = false, enabled = false; };
    std::function<void(int)> close;
    std::function<void(int)> toggle_lock;
    std::function<LockState(int)> lock_state;
    std::function<void(int,CPoint)> context;
    std::function<void(int,int)> reorder;
    void RefreshActions();
    friend class NativeUiTest;
protected:
    void DrawItem(LPDRAWITEMSTRUCT item) override;
    LRESULT WindowProc(UINT message,WPARAM wparam,LPARAM lparam) override;
private:
    enum class Action { None, Lock, Close };
    int Scale(int pixels) const;
    CRect ActionBounds(int index,Action action) const;
    Action ActionAt(int index,CPoint point) const;
    std::wstring ActionText(int index,Action action) const;
    void Hover(int index,Action action);
    int Hit(CPoint point) { TCHITTESTINFO info{}; info.pt = point; return HitTest(&info); }
    int pressed_ = -1,hovered_ = -1;
    Action pressed_action_ = Action::None,hover_action_ = Action::None;
    bool dragging_ = false;
    bool tracking_ = false;
    CPoint press_point_;
    CToolTipCtrl tooltip_;
    std::wstring tooltip_text_;
};

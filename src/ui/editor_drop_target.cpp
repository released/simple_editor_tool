#include "code_editor.h"
#include "../core/text_codec.h"
#include <Ole2.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <algorithm>
#include <new>

namespace {
class EditorDropTarget final : public IDropTarget {
public:
    explicit EditorDropTarget(CCodeEditor* owner) : owner_(owner) {}
    void Detach() { owner_ = nullptr; }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** object) override { if (!object) return E_POINTER; *object = nullptr; if (iid != IID_IUnknown && iid != IID_IDropTarget) return E_NOINTERFACE; *object = static_cast<IDropTarget*>(this); AddRef(); return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override { const auto count = InterlockedDecrement(&references_); if (!count) delete this; return count; }
    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* data,DWORD,POINTL,DWORD* effect) override { if (!effect) return E_POINTER; allowed_ = *effect; files_ = Supports(data,CF_HDROP); text_ = Supports(data,CF_UNICODETEXT); return Effect(effect); }
    HRESULT STDMETHODCALLTYPE DragOver(DWORD,POINTL,DWORD* effect) override { return Effect(effect); }
    HRESULT STDMETHODCALLTYPE DragLeave() override { files_ = text_ = false; return S_OK; }
    HRESULT STDMETHODCALLTYPE Drop(IDataObject* data,DWORD,POINTL location,DWORD* effect) override {
        if (!effect) return E_POINTER; const auto allowed = *effect; *effect = DROPEFFECT_NONE; if (!(allowed & DROPEFFECT_COPY)) return S_OK;
        if (!owner_ || !owner_->GetSafeHwnd() || !data) return S_OK;
        const bool files = Supports(data,CF_HDROP); if (!files && (owner_->Call(SCI_GETREADONLY) || !Supports(data,CF_UNICODETEXT))) return S_OK;
        FORMATETC format{static_cast<CLIPFORMAT>(files ? CF_HDROP : CF_UNICODETEXT),nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL}; STGMEDIUM medium{};
        if (FAILED(data->GetData(&format,&medium))) return S_OK;
        struct ReleaseMedium { STGMEDIUM& value; ~ReleaseMedium() { ReleaseStgMedium(&value); } } release{medium};
        if (medium.tymed != TYMED_HGLOBAL || !medium.hGlobal) return S_OK; const auto size = GlobalSize(medium.hGlobal); if (!size || size > 64 * 1024 * 1024) { Error(L"Dropped data is empty or exceeds 64 MiB."); return S_OK; }
        const auto* bytes = static_cast<const unsigned char*>(GlobalLock(medium.hGlobal)); if (!bytes) return S_OK;
        if (files) {
            const auto* header = reinterpret_cast<const DROPFILES*>(bytes); bool valid = size >= sizeof(DROPFILES) && header->pFiles >= sizeof(DROPFILES) && header->pFiles < size;
            if (valid && header->fWide) { valid = (header->pFiles % alignof(wchar_t)) == 0; bool terminated = false; if (valid) { const auto* names = reinterpret_cast<const wchar_t*>(bytes + header->pFiles); const auto count = (size - header->pFiles) / sizeof(wchar_t); for (std::size_t i = 1; i < count; ++i) if (!names[i - 1] && !names[i]) { terminated = true; break; } } valid = valid && terminated; }
            else if (valid) { bool terminated = false; for (auto i = static_cast<SIZE_T>(header->pFiles) + 1; i < size; ++i) if (!bytes[i - 1] && !bytes[i]) { terminated = true; break; } valid = terminated; }
            if (!valid) { GlobalUnlock(medium.hGlobal); Error(L"Invalid dropped file list."); return S_OK; }
            const auto copy = GlobalAlloc(GMEM_MOVEABLE,size); auto* target = copy ? GlobalLock(copy) : nullptr;
            if (!target) { if (copy) GlobalFree(copy); GlobalUnlock(medium.hGlobal); return S_OK; }
            memcpy(target,bytes,size); GlobalUnlock(copy); GlobalUnlock(medium.hGlobal);
            owner_->SendMessage(WM_DROPFILES,reinterpret_cast<WPARAM>(copy),0); *effect = DROPEFFECT_COPY; return S_OK;
        }
        const auto* begin = reinterpret_cast<const wchar_t*>(bytes); const auto* end = std::find(begin,begin + size / sizeof(wchar_t),L'\0');
        if (end == begin + size / sizeof(wchar_t)) { GlobalUnlock(medium.hGlobal); Error(L"Dropped text is not terminated."); return S_OK; }
        const std::wstring text(begin,end); GlobalUnlock(medium.hGlobal); std::wstring error; if (!simple_editor::core::ValidateText(text,&error)) { Error(error); return S_OK; }
        CPoint point(location.x,location.y); owner_->ScreenToClient(&point); auto position = owner_->Call(SCI_POSITIONFROMPOINTCLOSE,point.x,point.y); if (position < 0) position = owner_->Call(SCI_GETLENGTH);
        owner_->Call(SCI_SETSEL,position,position); owner_->ReplaceSel(text.c_str(),TRUE); *effect = DROPEFFECT_COPY; return S_OK;
    }
private:
    bool Supports(IDataObject* data,CLIPFORMAT type) const { if (!data) return false; FORMATETC format{type,nullptr,DVASPECT_CONTENT,-1,TYMED_HGLOBAL}; return data->QueryGetData(&format) == S_OK; }
    HRESULT Effect(DWORD* effect) const { if (!effect) return E_POINTER; *effect = (allowed_ & DROPEFFECT_COPY) && owner_ && owner_->GetSafeHwnd() && (files_ || (text_ && !owner_->Call(SCI_GETREADONLY))) ? DROPEFFECT_COPY : DROPEFFECT_NONE; return S_OK; }
    void Error(const std::wstring& text) { if (owner_) if (auto* frame = owner_->GetParentFrame()) frame->SetMessageText(text.c_str()); }
    LONG references_ = 1;
    CCodeEditor* owner_ = nullptr;
    bool files_ = false,text_ = false;
    DWORD allowed_ = DROPEFFECT_NONE;
};
}
CCodeEditor::~CCodeEditor() { DetachDropTarget(); }
void CCodeEditor::InitializeDropTarget() {
    RevokeDragDrop(GetSafeHwnd()); auto* target = new (std::nothrow) EditorDropTarget(this);
    if (target && SUCCEEDED(RegisterDragDrop(GetSafeHwnd(),target))) { drop_target_ = target; return; }
    if (target) target->Release(); ::DragAcceptFiles(GetSafeHwnd(),TRUE);
}
void CCodeEditor::DetachDropTarget() { if (!drop_target_) return; static_cast<EditorDropTarget*>(drop_target_)->Detach(); if (GetSafeHwnd()) RevokeDragDrop(GetSafeHwnd()); drop_target_->Release(); drop_target_ = nullptr; }

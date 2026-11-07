#include "markdown_preview.h"
#include "localization.h"
#include "ui_font.h"
#include "flowchart_picture.h"
#include <Richedit.h>
#include <Richole.h>
#include <tom.h>
#include <atlcomcli.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace {
constexpr wchar_t preview_storage[] = L"SimpleEditor.MarkdownStorage";
class PreviewStorage final : public IRichEditOleCallback {
public:
    bool own_stream = false;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** value) override {
        if (!value) return E_POINTER; *value = nullptr;
        if (id != IID_IUnknown && id != IID_IRichEditOleCallback) return E_NOINTERFACE;
        *value = static_cast<IRichEditOleCallback*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
    ULONG STDMETHODCALLTYPE Release() override { const auto count = InterlockedDecrement(&references_); if (!count) delete this; return static_cast<ULONG>(count); }
    HRESULT STDMETHODCALLTYPE GetNewStorage(LPSTORAGE* storage) override {
        if (!storage) return E_POINTER; *storage = nullptr; if (!own_stream) return E_ACCESSDENIED;
        CComPtr<ILockBytes> bytes; auto hr = CreateILockBytesOnHGlobal(nullptr,TRUE,&bytes);
        if (SUCCEEDED(hr)) hr = StgCreateDocfileOnILockBytes(bytes,STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,0,storage);
        return hr;
    }
    HRESULT STDMETHODCALLTYPE GetInPlaceContext(LPOLEINPLACEFRAME*,LPOLEINPLACEUIWINDOW*,LPOLEINPLACEFRAMEINFO) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ShowContainerUI(BOOL) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE QueryInsertObject(LPCLSID,LPSTORAGE,LONG) override { return own_stream ? S_OK : E_ACCESSDENIED; }
    HRESULT STDMETHODCALLTYPE DeleteObject(LPOLEOBJECT) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE QueryAcceptData(LPDATAOBJECT,CLIPFORMAT*,DWORD,BOOL,HGLOBAL) override { return E_ACCESSDENIED; }
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetClipboardData(CHARRANGE*,DWORD,LPDATAOBJECT* data) override { if (data) *data = nullptr; return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDragDropEffect(BOOL,DWORD,LPDWORD effect) override { if (effect) *effect = DROPEFFECT_NONE; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetContextMenu(WORD,LPOLEOBJECT,CHARRANGE*,HMENU* menu) override { if (menu) *menu = nullptr; return S_OK; }
private:
    LONG references_ = 1;
};
struct Stream { const std::string& bytes; std::size_t position = 0; };
DWORD CALLBACK ReadRtf(DWORD_PTR cookie,LPBYTE target,LONG size,LONG* count) {
    auto& stream = *reinterpret_cast<Stream*>(cookie);
    *count = static_cast<LONG>((std::min<std::size_t>)(static_cast<std::size_t>((std::max)(0L,size)),stream.bytes.size() - stream.position));
    if (*count) std::memcpy(target,stream.bytes.data() + stream.position,*count);
    stream.position += *count; return 0;
}
bool AtBottom(CWnd& window) {
    SCROLLINFO info{sizeof(info),SIF_RANGE | SIF_PAGE | SIF_POS};
    return window.GetScrollInfo(SB_VERT,&info) && info.nPage && info.nPos >= (std::max)(0,info.nMax - static_cast<int>(info.nPage) + 1);
}
bool ScrollCharacter(HWND window,LONG character,bool end = false) {
    CComPtr<IUnknown> ole; CComPtr<ITextDocument> document; CComPtr<ITextRange> range;
    if (!::SendMessageW(window,EM_GETOLEINTERFACE,0,reinterpret_cast<LPARAM>(&ole)) || !ole ||
        FAILED(ole->QueryInterface(__uuidof(ITextDocument),reinterpret_cast<void**>(&document))) || !document ||
        FAILED(document->Range((std::max)(0L,character),(std::max)(0L,character),&range)) || !range) return false;
    if (character < 0) { LONG length = 0; if (FAILED(range->GetStoryLength(&length)) || FAILED(range->SetRange((std::max)(0L,length - 1),(std::max)(0L,length - 1)))) return false; }
    return range->ScrollIntoView(end ? tomEnd : tomStart) == S_OK;
}
bool EndVisible(CWnd& window,LONG character) {
    RECT bounds{}; POINTL point{-1,-1}; window.SendMessage(EM_GETRECT,0,reinterpret_cast<LPARAM>(&bounds));
    window.SendMessage(EM_POSFROMCHAR,reinterpret_cast<WPARAM>(&point),character);
    return point.y >= bounds.top && point.y < bounds.bottom;
}
constexpr UINT_PTR preview_subclass = 0x4D445343;
constexpr wchar_t wheel_remainder[] = L"SimpleEditor.MarkdownWheelRemainder";
constexpr wchar_t picture_scroll_step[] = L"SimpleEditor.MarkdownPictureScrollStep";
void ScrollPicturePixels(HWND window,long long delta) {
    POINT position{}; ::SendMessageW(window,EM_GETSCROLLPOS,0,reinterpret_cast<LPARAM>(&position));
    SCROLLINFO info{sizeof(info),SIF_RANGE | SIF_PAGE | SIF_POS};
    if (GetScrollInfo(window,SB_VERT,&info)) position.y = info.nPos;
    const auto maximum = (std::max)(0,info.nMax - static_cast<int>(info.nPage) + 1);
    position.y = static_cast<LONG>((std::clamp<long long>)(static_cast<long long>(position.y) + delta,0,maximum));
    ::SendMessageW(window,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&position));
}
LRESULT CALLBACK PreviewScrollProc(HWND window,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR,DWORD_PTR) {
    if (message == WM_NCDESTROY) {
        if (auto* storage = reinterpret_cast<PreviewStorage*>(RemovePropW(window,preview_storage))) storage->Release();
        RemovePropW(window,wheel_remainder); RemovePropW(window,picture_scroll_step); RemoveWindowSubclass(window,PreviewScrollProc,preview_subclass);
    }
    if (message == WM_MOUSEWHEEL) {
        if (LOWORD(wparam) & MK_CONTROL) {
            if (auto* frame = CWnd::FromHandlePermanent(::GetParent(::GetParent(window)))) {
                MSG event{}; event.hwnd = window; event.message = message; event.wParam = wparam; event.lParam = lparam; frame->PreTranslateMessage(&event);
            }
            return 0;
        }
        const int accumulated = static_cast<int>(reinterpret_cast<INT_PTR>(GetPropW(window,wheel_remainder))) + static_cast<short>(HIWORD(wparam));
        const int steps = accumulated / WHEEL_DELTA;
        SetPropW(window,wheel_remainder,reinterpret_cast<HANDLE>(static_cast<INT_PTR>(accumulated % WHEEL_DELTA)));
        if (steps) {
            UINT lines = 3; SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0,&lines,0);
            if (const auto pixels = reinterpret_cast<INT_PTR>(GetPropW(window,picture_scroll_step))) {
                RECT viewport{}; GetClientRect(window,&viewport);
                ScrollPicturePixels(window,-static_cast<long long>(steps) * (lines == WHEEL_PAGESCROLL ? viewport.bottom : pixels * lines));
                return 0;
            }
            if (lines == WHEEL_PAGESCROLL) { for (int n = 0; n < std::abs(steps); ++n) ::SendMessageW(window,EM_SCROLL,steps > 0 ? SB_PAGEUP : SB_PAGEDOWN,0); }
            else ::SendMessageW(window,EM_LINESCROLL,0,static_cast<LPARAM>((std::clamp<long long>)(-static_cast<long long>(steps) * lines,INT_MIN,INT_MAX)));
        }
        return 0;
    }
    if (message == WM_VSCROLL && !lparam) {
        const auto command = LOWORD(wparam);
        if (command == SB_TOP || command == SB_BOTTOM) {
            if (ScrollCharacter(window,command == SB_TOP ? 0L : -1L,command == SB_BOTTOM)) return 0;
        }
        if (command == SB_LINEUP || command == SB_LINEDOWN || command == SB_PAGEUP || command == SB_PAGEDOWN) {
            if (const auto pixels = reinterpret_cast<INT_PTR>(GetPropW(window,picture_scroll_step))) {
                RECT viewport{}; GetClientRect(window,&viewport); const auto step = command == SB_PAGEUP || command == SB_PAGEDOWN ? viewport.bottom : pixels;
                ScrollPicturePixels(window,command == SB_LINEUP || command == SB_PAGEUP ? -step : step);
            } else ::SendMessageW(window,EM_SCROLL,command,0);
            return 0;
        }
    }
    return DefSubclassProc(window,message,wparam,lparam);
}
}
BEGIN_MESSAGE_MAP(CMarkdownPreview,CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()
bool CMarkdownPreview::Create(CWnd* owner) {
    const auto cls = AfxRegisterWndClass(0,LoadCursor(nullptr,IDC_ARROW),reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),nullptr);
    return CWnd::Create(cls,L"",WS_CHILD | WS_CLIPCHILDREN,CRect(),owner,6210) != FALSE;
}
int CMarkdownPreview::OnCreate(LPCREATESTRUCT create) {
    if (CWnd::OnCreate(create) == -1) return -1;
    if (!ole_ready_) ole_ready_ = SUCCEEDED(OleInitialize(nullptr));
    library_ = LoadLibraryExW(L"Msftedit.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library_ || !caption_.Create(L"Markdown preview",WS_CHILD | WS_VISIBLE,CRect(),this,6211) ||
        !rich_.CreateEx(WS_EX_CLIENTEDGE,MSFTEDIT_CLASS,L"",WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,CRect(),this,6212)) return -1;
    if (!SetWindowSubclass(rich_.GetSafeHwnd(),PreviewScrollProc,preview_subclass,0)) return -1;
    if (ole_ready_) {
        if (auto* storage = new(std::nothrow) PreviewStorage) {
            if (!SetPropW(rich_.GetSafeHwnd(),preview_storage,storage) || !rich_.SendMessage(EM_SETOLECALLBACK,0,reinterpret_cast<LPARAM>(storage))) {
                RemovePropW(rich_.GetSafeHwnd(),preview_storage); storage->Release();
            }
        }
    }
    if (!simple_editor::ui::CreateUiFont(font_,*this)) return -1;
    caption_.SetFont(&font_);
    rich_.SendMessage(EM_EXLIMITTEXT,0,262144);
    rich_.SendMessage(EM_SETTARGETDEVICE,0,0);
    rich_.SendMessage(EM_AUTOURLDETECT,FALSE,0);
    return 0;
}
CMarkdownPreview::~CMarkdownPreview() {
    Stop(); if (::IsWindow(GetSafeHwnd())) DestroyWindow();
    if (::IsWindow(rich_.GetSafeHwnd())) rich_.DestroyWindow();
    if (library_) FreeLibrary(library_);
    if (ole_ready_) OleUninitialize();
}
void CMarkdownPreview::Stop() { cancelled_ = true; if (job_.valid()) { job_.wait(); try { job_.get(); } catch (...) {} } }
void CMarkdownPreview::OnDestroy() { Stop(); CWnd::OnDestroy(); }
void CMarkdownPreview::OnSize(UINT type,int width,int height) {
    CWnd::OnSize(type,width,height);
    if (!rich_.GetSafeHwnd()) return;
    const int top = (std::min)(26,(std::max)(0,height));
    caption_.MoveWindow(6,2,(std::max)(0,width - 12),(std::max)(0,top - 2));
    rich_.MoveWindow(0,top,(std::max)(0,width),(std::max)(0,height - top));
}
bool CMarkdownPreview::RouteKey(MSG* message) {
    if (!rich_.GetSafeHwnd() || message->hwnd != rich_.GetSafeHwnd() || message->message != WM_KEYDOWN || !(GetKeyState(VK_CONTROL) & 0x8000)) return false;
    if (message->wParam == 'C') { rich_.SendMessage(WM_COPY); return true; }
    if (message->wParam == 'A') { rich_.SendMessage(EM_SETSEL,0,-1); return true; }
    return message->wParam == 'Z' || message->wParam == 'Y' || message->wParam == 'X' || message->wParam == 'V';
}
bool CMarkdownPreview::Current(const CCodeEditor& editor,std::uint64_t revision) const { return generation_ == editor.Generation() && revision_ == revision && applied_ == requested_ && !applied_.empty(); }
void CMarkdownPreview::Tick(CCodeEditor& editor,std::uint64_t revision,const std::wstring& path,bool visible,unsigned theme,int font_tenths,int zoom) {
    if (!rich_.GetSafeHwnd() || (!visible && !job_.valid())) return;
    const auto caption = simple_editor::ui::Localize(L"Markdown preview");
    if (caption_text_ != caption) { caption_text_ = caption; caption_.SetWindowTextW(caption.c_str()); }
    CRect bounds; rich_.GetClientRect(bounds); CClientDC dc(this);
    const auto generation = editor.Generation();
    const auto tag = path + L":" + std::to_wstring(revision) + L":" + std::to_wstring(generation) + L":" + std::to_wstring(theme) + L":" + std::to_wstring(font_tenths) + L":" + std::to_wstring(zoom) + L":" + std::to_wstring(bounds.Width()) + L":" + simple_editor::ui::UiLanguage();
    if (tag != requested_) { requested_ = tag; due_ = GetTickCount64() + 250; if (job_.valid()) cancelled_ = true; }
    if (!visible) { if (job_.valid()) cancelled_ = true; }
    if (job_.valid() && job_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        try { auto result = job_.get(); if (visible && running_ == requested_ && !cancelled_ && !result.rtf.empty()) {
            Present(result,theme);
            if (result.rtf.find("{\\pict\\emfblip") != std::string::npos) SetPropW(rich_.GetSafeHwnd(),picture_scroll_step,reinterpret_cast<HANDLE>(static_cast<INT_PTR>((std::max)(1,MulDiv(font_tenths * zoom,dc.GetDeviceCaps(LOGPIXELSY),72000) + 4))));
            else RemovePropW(rich_.GetSafeHwnd(),picture_scroll_step);
            applied_ = running_; generation_ = generation; revision_ = revision;
        } }
        catch (...) { scroll_anchors_.clear(); text_positions_.clear(); sync_ready_ = false; caption_.SetWindowTextW(simple_editor::ui::Localize(L"Markdown preview unavailable; source editing remains available.").c_str()); applied_ = running_; }
    }
    if (!visible || job_.valid() || requested_ == applied_ || GetTickCount64() < due_) return;
    const auto size = static_cast<std::size_t>((std::min<LRESULT>)(262144,(std::max<LRESULT>)(0,editor.Call(SCI_GETLENGTH))));
    std::string input(size + 1,'\0'); Sci_TextRange range{{0,static_cast<Sci_PositionCR>(size)},input.data()};
    editor.Call(SCI_GETTEXTRANGE,0,reinterpret_cast<LPARAM>(&range)); input.resize(size);
    // A bounded byte prefix can end within a UTF-8 sequence.
    for (unsigned retry = 0; retry < 4 && !input.empty() && !MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,input.data(),static_cast<int>(input.size()),nullptr,0); ++retry) input.pop_back();
    const auto text = CCodeEditor::FromUtf8(input);
    const bool dark = theme == 1 || (theme == 2 && GetRValue(GetSysColor(COLOR_WINDOW)) < 128);
    const auto half_points = static_cast<unsigned>((std::max)(12,font_tenths * zoom / 500));
    const int width = MulDiv((std::max)(80,bounds.Width() - 16),1440,dc.GetDeviceCaps(LOGPIXELSX));
    const auto notice = simple_editor::ui::Localize(L"Preview limited to the first 64 Ki characters / 4,096 lines; source is unchanged.");
    const auto diagram_notice = simple_editor::ui::Localize(L"Mermaid diagram unavailable (unsupported syntax or preview limit); source shown below.");
    const auto diagrams_ready = ole_ready_ && GetPropW(rich_.GetSafeHwnd(),preview_storage);
    cancelled_ = false; running_ = requested_;
    try { job_ = std::async(std::launch::async,[this,text,dark,half_points,width,notice,diagram_notice,diagrams_ready] {
        return simple_editor::core::RenderMarkdownPreview(text,dark,half_points,width,notice,cancelled_,
            diagrams_ready ? simple_editor::core::MarkdownDiagramRenderer(simple_editor::ui::RenderFlowchartPicture) : simple_editor::core::MarkdownDiagramRenderer{},diagram_notice);
    }); }
    catch (...) { caption_.SetWindowTextW(simple_editor::ui::Localize(L"Markdown preview unavailable; source editing remains available.").c_str()); applied_ = requested_; }
}
void CMarkdownPreview::Present(const simple_editor::core::MarkdownPreview& result,unsigned theme) {
    POINT scroll{}; CHARRANGE selection{}; rich_.SendMessage(EM_GETSCROLLPOS,0,reinterpret_cast<LPARAM>(&scroll)); rich_.SendMessage(EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&selection));
    rich_.SetRedraw(FALSE);
    Stream source{result.rtf}; EDITSTREAM stream{reinterpret_cast<DWORD_PTR>(&source),0,ReadRtf};
    rich_.SendMessage(EM_SETBKGNDCOLOR,0,theme == 1 ? RGB(63,63,63) : GetSysColor(COLOR_WINDOW));
    auto* storage = reinterpret_cast<PreviewStorage*>(GetPropW(rich_.GetSafeHwnd(),preview_storage));
    if (storage) storage->own_stream = true;
    rich_.SendMessage(EM_STREAMIN,SF_RTF,reinterpret_cast<LPARAM>(&stream));
    if (storage) storage->own_stream = false;
    rich_.SendMessage(EM_SETREADONLY,TRUE,0); rich_.SendMessage(EM_EMPTYUNDOBUFFER);
    rich_.SendMessage(EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&selection)); rich_.SendMessage(EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&scroll));
    if (!stream.dwError) BuildScrollMap(result);
    const bool missing_picture = result.rtf.find("{\\pict\\emfblip") != std::string::npos && scroll_anchors_.empty();
    rich_.SetRedraw(TRUE); rich_.Invalidate(FALSE);
    if (stream.dwError || missing_picture) throw std::runtime_error("Markdown preview stream failed");
}
void CMarkdownPreview::BuildScrollMap(const simple_editor::core::MarkdownPreview& result) {
    sync_ready_ = false; scroll_anchors_.clear(); text_positions_.clear();
    source_lines_ = result.source_lines; truncated_ = result.truncated;
    if (result.scroll_text.empty()) return;
    // RAWTEXT keeps native character positions, including table delimiters.
    std::wstring raw(262145,0); GETTEXTEX request{static_cast<DWORD>(raw.size() * sizeof(wchar_t)),GT_RAWTEXT,1200,nullptr,nullptr};
    const auto copied = rich_.SendMessage(EM_GETTEXTEX,reinterpret_cast<WPARAM>(&request),reinterpret_cast<LPARAM>(raw.data()));
    if (copied < 0 || static_cast<std::size_t>(copied) >= raw.size()) return;
    raw.resize(static_cast<std::size_t>(copied));
    std::vector<LONG> positions; positions.reserve(result.scroll_text.size() + 1); std::size_t key = 0;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (key < result.scroll_text.size() && raw[i] == result.scroll_text[key]) { positions.push_back(static_cast<LONG>(i)); ++key; }
        else if (raw[i] > 32 && raw[i] != 0xFFF9 && raw[i] != 0xFFFB) return;
    }
    if (key != result.scroll_text.size()) return;
    positions.push_back(static_cast<LONG>(raw.size()));
    std::vector<ScrollAnchor> anchors; anchors.reserve(result.anchors.size());
    for (const auto& anchor : result.anchors) {
        if (anchor.text_offset >= positions.size() || anchor.source_line >= source_lines_ ||
            (!anchors.empty() && (anchor.source_line <= anchors.back().source_line || positions[anchor.text_offset] < anchors.back().character))) return;
        anchors.push_back({anchor.source_line,positions[anchor.text_offset]});
    }
    scroll_anchors_ = std::move(anchors); text_positions_ = std::move(positions);
}
void CMarkdownPreview::SynchronizeScroll(CCodeEditor& source,std::uint64_t revision) {
    if (!Current(source,revision) || scroll_anchors_.empty() || !rich_.IsWindowVisible()) return;
    const auto source_top = source.Call(SCI_GETFIRSTVISIBLELINE),preview_top = rich_.SendMessage(EM_GETFIRSTVISIBLELINE);
    const bool source_changed = source_top != source_top_,preview_changed = preview_top != preview_top_;
    if (sync_ready_ && !source_changed && !preview_changed) return;
    const auto rich_line = [&](LONG character) { return rich_.SendMessage(EM_EXLINEFROMCHAR,0,character); };
    if (!sync_ready_ || (source_changed && !(preview_changed && ::GetFocus() == rich_.GetSafeHwnd()))) {
        if (!source_top) { if (preview_top) rich_.SendMessage(WM_VSCROLL,SB_TOP); }
        else if (AtBottom(source) || static_cast<std::size_t>(source.Call(SCI_DOCLINEFROMVISIBLE,source_top)) >= source_lines_) {
            ScrollCharacter(rich_.GetSafeHwnd(),-1,true);
        } else {
            const auto line = static_cast<std::size_t>((std::max<LRESULT>)(0,source.Call(SCI_DOCLINEFROMVISIBLE,source_top)));
            auto at = std::upper_bound(scroll_anchors_.begin(),scroll_anchors_.end(),line,[](std::size_t value,const auto& item) { return value < item.source_line; });
            if (at != scroll_anchors_.begin()) --at;
            auto character = at->character;
            const auto base = source.Call(SCI_VISIBLEFROMDOCLINE,line),wrap = (std::max<LRESULT>)(1,source.Call(SCI_WRAPCOUNT,line));
            const auto next = std::find_if(at + 1,scroll_anchors_.end(),[&](const auto& item) { return item.character > at->character; });
            if (base >= 0 && source_top > base && next != scroll_anchors_.end()) {
                character += static_cast<LONG>((next->character - character) * (std::min)(wrap - 1,source_top - base) / wrap);
            }
            if (!ScrollCharacter(rich_.GetSafeHwnd(),character)) { const auto target = rich_line(character); if (target >= 0 && target != preview_top) rich_.SendMessage(EM_LINESCROLL,0,target - preview_top); }
        }
    } else if (preview_changed) {
        const auto line = static_cast<std::size_t>((std::max<LRESULT>)(0,source.Call(SCI_DOCLINEFROMVISIBLE,source_top)));
        if (truncated_ && line >= source_lines_ && (preview_top >= preview_top_ ||
            (text_positions_.size() > 1 && EndVisible(rich_,text_positions_[text_positions_.size() - 2])))) {
            // A further downward gesture at the bounded preview end has no source counterpart.
        } else if (!preview_top) { if (source_top) source.Call(SCI_SETFIRSTVISIBLELINE,0); }
        else if (!truncated_ && text_positions_.size() > 1 && EndVisible(rich_,text_positions_[text_positions_.size() - 2])) {
            SCROLLINFO info{sizeof(info),SIF_RANGE | SIF_PAGE};
            if (source.GetScrollInfo(SB_VERT,&info)) source.Call(SCI_SETFIRSTVISIBLELINE,(std::max)(0,info.nMax - static_cast<int>(info.nPage) + 1));
        } else {
            RECT bounds{}; rich_.SendMessage(EM_GETRECT,0,reinterpret_cast<LPARAM>(&bounds)); POINTL point{bounds.left + 1,bounds.top + 1};
            const auto first = rich_.SendMessage(EM_CHARFROMPOS,0,reinterpret_cast<LPARAM>(&point));
            if (first < 0 || first > text_positions_.back()) return;
            const auto visible = std::lower_bound(text_positions_.begin(),text_positions_.end(),first);
            const auto character = visible == text_positions_.end() ? text_positions_.back() : *visible;
            auto at = std::upper_bound(scroll_anchors_.begin(),scroll_anchors_.end(),character,[](LONG value,const auto& item) { return value < item.character; });
            if (at != scroll_anchors_.begin()) --at;
            auto source_line = at->source_line;
            while (source_line && !source.Call(SCI_GETLINEVISIBLE,source_line)) --source_line;
            auto target = source.Call(SCI_VISIBLEFROMDOCLINE,source_line);
            const auto wrap = (std::max<LRESULT>)(1,source.Call(SCI_WRAPCOUNT,source_line));
            const auto next = std::find_if(at + 1,scroll_anchors_.end(),[&](const auto& item) { return item.character > at->character; });
            if (next != scroll_anchors_.end()) {
                const auto span = next->character - at->character;
                if (span > 0 && character > at->character) target += (std::min)(wrap - 1,wrap * (character - at->character) / span);
            }
            if (target >= 0 && target != source_top) source.Call(SCI_SETFIRSTVISIBLELINE,target);
        }
    }
    // Remember the actual clamped positions; idle ticks must never feed back.
    source_top_ = source.Call(SCI_GETFIRSTVISIBLELINE); preview_top_ = rich_.SendMessage(EM_GETFIRSTVISIBLELINE); sync_ready_ = true;
}

#pragma once
#include <Ole2.h>
#include <Shellapi.h>
#include <Shlobj.h>
#include <vector>
#include <string>
#include <cstring>
class TestDropData final : public IDataObject {
public:
    explicit TestDropData(const std::vector<std::wstring>& paths) : format_(CF_HDROP) { std::wstring names; for (const auto& path : paths) { names += path; names.push_back(0); } names.push_back(0); bytes_.resize(sizeof(DROPFILES) + names.size() * sizeof(wchar_t)); auto* header = reinterpret_cast<DROPFILES*>(bytes_.data()); header->pFiles = sizeof(DROPFILES); header->fWide = TRUE; memcpy(bytes_.data() + sizeof(DROPFILES),names.data(),names.size() * sizeof(wchar_t)); }
    explicit TestDropData(const std::wstring& text) : format_(CF_UNICODETEXT) { bytes_.resize((text.size() + 1) * sizeof(wchar_t)); memcpy(bytes_.data(),text.c_str(),bytes_.size()); }
    TestDropData(CLIPFORMAT format,std::vector<unsigned char> bytes) : format_(format), bytes_(std::move(bytes)) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** value) override { if (!value) return E_POINTER; *value = nullptr; if (iid != IID_IUnknown && iid != IID_IDataObject) return E_NOINTERFACE; *value = static_cast<IDataObject*>(this); AddRef(); return S_OK; }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override { const auto count = InterlockedDecrement(&references_); if (!count) delete this; return count; }
    HRESULT STDMETHODCALLTYPE GetData(FORMATETC* format,STGMEDIUM* value) override { if (!value || QueryGetData(format) != S_OK) return DV_E_FORMATETC; *value = {}; const auto memory = GlobalAlloc(GMEM_MOVEABLE,bytes_.size()); auto* bytes = memory ? GlobalLock(memory) : nullptr; if (!bytes) { if (memory) GlobalFree(memory); return E_OUTOFMEMORY; } memcpy(bytes,bytes_.data(),bytes_.size()); GlobalUnlock(memory); value->tymed = TYMED_HGLOBAL; value->hGlobal = memory; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*,STGMEDIUM*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* format) override { return format && format->cfFormat == format_ && (format->tymed & TYMED_HGLOBAL) ? S_OK : DV_E_FORMATETC; }
    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*,FORMATETC*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetData(FORMATETC*,STGMEDIUM*,BOOL) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD,IEnumFORMATETC**) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*,DWORD,IAdviseSink*,DWORD*) override { return OLE_E_ADVISENOTSUPPORTED; }
    HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override { return OLE_E_ADVISENOTSUPPORTED; }
    HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override { return OLE_E_ADVISENOTSUPPORTED; }
private:
    LONG references_ = 1;
    CLIPFORMAT format_;
    std::vector<unsigned char> bytes_;
};

#include "main_frame.h"
#include "resource.h"
#include "ui/feature_dialogs.h"
#include "core/crc_profiles.h"
using namespace simple_editor::core;
bool CMainFrame::StartCustomCrc(const std::wstring& settings) {
    auto* view = ActiveHex(); if (IsBusy() || !view || (view == &hex_ && !ApplyTextEdits())) return false; CrcProfile profile; std::wstring error; if (!ParseCrcProfile(settings,&profile,&error)) { status_.SetPaneText(0,error.c_str()); return false; }
    auto* document = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_; auto snapshot = document->Snapshot(&error); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return false; } const auto offset = view->SelectionLength() ? view->SelectionStart() : 0,length = view->SelectionLength() ? view->SelectionLength() : snapshot->Size(); crc_profile_ = settings; cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = EditorJobKind::Tool; job_revision_ = revision_;
    try { job_ = std::async(std::launch::async,[this,profile,settings,offset,length,snapshot = std::move(snapshot)] { EditorJobResult result; result.kind = EditorJobKind::Tool; SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; }; std::uint64_t value = 0; if (CalculateCrc(*snapshot,offset,length,profile,control,&value,&result.error)) { wchar_t hex[32]{}; swprintf_s(hex,L"%0*llX",(profile.width + 3) / 4,value); result.message = profile.name; result.details = L"Profile: " + profile.name + L"\r\nPhysical start: " + std::to_wstring(offset) + L"\r\nBytes: " + std::to_wstring(length) + L"\r\nCRC: 0x" + hex + L"\r\n\r\n" + settings; } return result; }); } catch (...) { return false; } return true;
}
void CMainFrame::CustomCrcDialog() {
    if (IsBusy() || !ActiveHex()) return; CFeatureInput dialog(this,L"Named / custom CRC",L"Presets: CRC-16/ARC, CRC-16/MODBUS, CRC-32/ISO-HDLC, CRC-32/ISCSI, CRC-64/ECMA-182. Optional overrides: name, width (decimal 8-64), polynomial, init, xorout (hex), refin/refout=yes/no. Operates on the selection or whole file; settings save with the workspace.",crc_profile_.empty() ? L"preset=CRC-32/ISO-HDLC" : crc_profile_); dialog.validate = [](const std::wstring& text,std::wstring* error) { CrcProfile profile; return ParseCrcProfile(text,&profile,error); }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer == IDOK) StartCustomCrc(dialog.value);
}

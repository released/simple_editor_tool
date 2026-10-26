#include "main_frame.h"
#include "ui/localization.h"
#include "workspace_frame.h"
#include "resource.h"
#include "core/binary_analysis.h"
#include "core/search.h"
#include "core/firmware_image.h"
#include <iomanip>
#include <sstream>

using namespace simple_editor::core;
bool CMainFrame::StartBinaryTool(UINT command, const std::wstring& output, const std::vector<std::uint8_t>& pattern, const std::vector<std::filesystem::path>& paths, std::uint64_t part) {
    auto* view = ActiveHex(); if (IsBusy() || !view || (view == &hex_ && !ApplyTextEdits())) return false;
    const bool transform = command >= ID_TOOL_FILL && command <= ID_TOOL_REVERSE;
    if (transform && (view != &hex_ || MutationBlocked() || !view->SelectionLength())) { status_.SetPaneText(0,L"Select bytes in an editable file Hex view first."); return false; }
    const auto* source = view == &hex_ ? &document_ : view == &compare_left_ ? &compare_left_document_ : &compare_right_document_;
    std::wstring error; auto snapshot = source->Snapshot(&error,transform); if (!snapshot) { status_.SetPaneText(0,error.c_str()); return false; }
    const auto offset = view->SelectionLength() ? view->SelectionStart() : 0, length = view->SelectionLength() ? view->SelectionLength() : snapshot->Size();
    job_locked_paths_.clear();
    if (!output.empty()) {
        if (command == ID_TOOL_SPLIT && part) { const auto count = (std::max<std::uint64_t>)(1,snapshot->Size() / part + (snapshot->Size() % part != 0)); if (count > 100000) return false; for (std::uint64_t i = 1; i <= count; ++i) job_locked_paths_.push_back(output + L".part" + std::to_wstring(i) + L".bin"); }
        else job_locked_paths_.push_back(output);
        if (workspace_ && !workspace_->LockPaths(job_locked_paths_,true,&job_)) { job_locked_paths_.clear(); status_.SetPaneText(0,L"An output is busy or has unsaved changes."); return false; } job_locks_held_ = workspace_ != nullptr;
    }
    cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_revision_ = revision_; job_kind_ = transform ? EditorJobKind::Transform : command == ID_TOOL_HASH || command == ID_TOOL_IHEX_INSPECT || command == ID_TOOL_SREC_INSPECT ? EditorJobKind::Tool : EditorJobKind::Export;
    try { job_ = std::async(std::launch::async,[this,command,output,pattern,paths,part,offset,length,kind = job_kind_,snapshot = std::move(snapshot)]() mutable {
        EditorJobResult result; result.kind = kind; SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; }; bool success = false;
        if (command == ID_TOOL_HASH) { BinaryAnalysis analysis; success = AnalyzeBinary(*snapshot,offset,length,control,&analysis,&result.error); if (success) { std::wostringstream text; text << L"Offset: 0x" << std::hex << offset << L"\r\nBytes: " << std::dec << length << L"\r\nCRC32: " << std::hex << std::uppercase << std::setw(8) << std::setfill(L'0') << analysis.crc32 << L"\r\nSHA-256: " << analysis.sha256 << L"\r\nEntropy: " << std::dec << std::fixed << std::setprecision(6) << analysis.entropy << L" bits/byte\r\n\r\nByte histogram (hex: count)\r\n"; for (unsigned i = 0; i < 256; ++i) { text << std::hex << std::setw(2) << i << L": " << std::dec << analysis.histogram[i] << ((i + 1) % 4 ? L"    " : L"\r\n"); } text << L"\r\nEntropy by consecutive equal-sized blocks (up to 512):\r\n"; for (std::size_t i = 0; i < analysis.entropy_blocks.size(); ++i) text << std::fixed << std::setprecision(2) << analysis.entropy_blocks[i] << ((i + 1) % 8 ? L"  " : L"\r\n"); result.details = text.str(); result.message = L"Binary analysis"; } }
        else if (kind == EditorJobKind::Transform) { const auto op = command == ID_TOOL_FILL ? RangeTransform::Fill : command == ID_TOOL_INVERT ? RangeTransform::Invert : command == ID_TOOL_SWAP2 ? RangeTransform::Swap2 : command == ID_TOOL_SWAP4 ? RangeTransform::Swap4 : command == ID_TOOL_SWAP8 ? RangeTransform::Swap8 : RangeTransform::Reverse; success = TransformBytes(*snapshot,offset,length,op,pattern,control,&result.error); if (success) { result.document = std::move(snapshot); result.count = 1; result.message = L"Selected bytes transformed. Undo restores the entire operation."; } }
        else if (command >= ID_TOOL_IHEX_INSPECT && command <= ID_TOOL_SREC_BIN) { FirmwareImage image; const auto format = command == ID_TOOL_IHEX_INSPECT || command == ID_TOOL_IHEX_BIN ? FirmwareFormat::IntelHex : FirmwareFormat::SRecord; success = ParseFirmware(*snapshot,format,control,&image,&result.error); if (success && command <= ID_TOOL_SREC_INSPECT) { std::wostringstream text; text << L"Validated records: " << image.records << L"\r\nData bytes: " << image.data_bytes << L"\r\nSegments: " << image.segments.size() << L"\r\n"; if (image.has_entry) text << L"Entry address: 0x" << std::hex << image.entry << L"\r\n"; text << L"\r\nAddress map (up to 200 segments):\r\n"; for (std::size_t i = 0; i < (std::min<std::size_t>)(200,image.segments.size()); ++i) text << L"0x" << std::hex << image.segments[i].address << L" + " << std::dec << image.segments[i].bytes.size() << L" bytes\r\n"; result.details = text.str(); result.message = L"Firmware record validation and address map"; } else if (success) { if (image.End() - image.Begin() > 256ULL * 1024 * 1024) { success = false; result.error = L"Sparse BIN span exceeds 256 MiB. Choose a smaller firmware image."; } else success = FirmwareToBinary(*snapshot,image,image.Begin(),image.End() - image.Begin(),pattern.empty() ? 0xFF : pattern[0],output,control,&result.error); } }
        else if (command == ID_TOOL_BIN_IHEX || command == ID_TOOL_BIN_SREC) success = BinaryToFirmware(*snapshot,part,command == ID_TOOL_BIN_IHEX ? FirmwareFormat::IntelHex : FirmwareFormat::SRecord,output,control,&result.error);
        else if (command == ID_TOOL_JOIN) success = JoinBinaryFiles(paths,output,control,&result.error);
        else if (command == ID_TOOL_SPLIT) { std::vector<std::filesystem::path> completed; success = SplitBinaryFile(*snapshot,part,output,control,&completed,&result.error); result.message = std::to_wstring(completed.size()) + L" split files saved."; if (!success) result.error += L" Completed files retained: " + std::to_wstring(completed.size()); }
        else success = ExtractBytes(*snapshot,offset,length,output,control,&result.error,command == ID_TOOL_ARRAY);
        if (success && result.message.empty()) result.message = L"Output saved: " + output;
        if (!success && result.error.empty()) result.error = L"Operation failed or was cancelled; existing data was retained."; return result;
    }); } catch (...) { ReleaseJobLocks(); status_.SetPaneText(0,L"Could not start the operation."); return false; }
    status_.SetPaneText(0,L"Processing in the background. Esc cancels before replacement."); return true;
}
void CMainFrame::OnBinaryTool(UINT command) {
    if (IsBusy()) return; auto* view = ActiveHex(); if (!view) { status_.SetPaneText(0,L"Choose a Hex view first."); return; }
    if (command == ID_TOOL_INSPECT_LE || command == ID_TOOL_INSPECT_BE) {
        if (view == &hex_ && !ApplyTextEdits()) return; const auto& source = view == &hex_ ? document_ : view == &compare_left_ ? compare_left_document_ : compare_right_document_;
        std::vector<InspectorValue> values; std::wstring error, text = L"At offset 0x"; wchar_t address[32]{}; swprintf_s(address,L"%llX",view->CursorOffset()); text += address; text += command == ID_TOOL_INSPECT_LE ? L" (little endian)\r\n\r\n" : L" (big endian)\r\n\r\n";
        if (!InspectNumbers(source,view->CursorOffset(),command == ID_TOOL_INSPECT_LE,&values,&error)) { status_.SetPaneText(0,error.c_str()); return; }
        for (const auto& value : values) text += value.type + L": " + value.value + L"\r\n";
        CTextResultDialog dialog(this,L"Data inspector",text); path_dialog_ = true; dialog.DoModal(); path_dialog_ = false; return;
    }
    std::vector<std::uint8_t> pattern; std::uint64_t part = 0; std::vector<std::filesystem::path> paths;
    if (command == ID_TOOL_IHEX_BIN || command == ID_TOOL_SREC_BIN) { CValueDialog dialog(this,L"Firmware to BIN",L"Fill byte for gaps (00-FF). Output starts at the lowest data address and spans to the highest address (maximum 256 MiB).",L"FF"); dialog.validate = [&](const std::wstring& value,std::wstring*) { return ParseHexBytes(value,&pattern) && pattern.size() == 1; }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return; }
    if (command == ID_TOOL_BIN_IHEX || command == ID_TOOL_BIN_SREC) { CValueDialog dialog(this,L"BIN to firmware records",L"Base address (hex) for the complete binary document. Output includes record checksums.",L"0"); dialog.validate = [&](const std::wstring& value,std::wstring*) { return ParseAddress(value,&part) && part <= UINT32_MAX; }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return; }
    if (command == ID_TOOL_FILL) { CValueDialog dialog(this,L"Fill selection",L"Repeating Hex byte pattern. The selection changes as one Undo action.",L"00"); dialog.validate = [&](const std::wstring& value,std::wstring*) { return ParseHexBytes(value,&pattern) && !pattern.empty(); }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return; }
    if (command == ID_TOOL_SPLIT) { CValueDialog dialog(this,L"Split file",L"Maximum bytes per output. Hex by default; prefix decimal with d:. New files use prefix.part1.bin, prefix.part2.bin, ... Existing files are never replaced.",L"d:1048576"); dialog.validate = [&](const std::wstring& value,std::wstring*) { return ParseAddress(value,&part) && part; }; path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return; }
    if (command == ID_TOOL_JOIN) { std::vector<wchar_t> buffer(65536); CFileDialog input(TRUE,nullptr,nullptr,OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,L"All files (*.*)|*.*||",this); input.m_ofn.lpstrFile = buffer.data(); input.m_ofn.nMaxFile = static_cast<DWORD>(buffer.size()); path_dialog_ = true; const auto answer = input.DoModal(); path_dialog_ = false; if (answer != IDOK) return; for (auto pos = input.GetStartPosition(); pos;) paths.emplace_back(input.GetNextPathName(pos).GetString()); std::wstring order = L"Join in this order:\r\n"; for (const auto& path : paths) order += path.wstring() + L"\r\n"; order += L"\r\nContinue to choose the output?"; path_dialog_ = true; const auto accepted = UiMessageBox(order.c_str(),MB_OKCANCEL | MB_ICONQUESTION); path_dialog_ = false; if (accepted != IDOK) return; }
    std::wstring output;
    if (command == ID_TOOL_EXTRACT || command == ID_TOOL_ARRAY || command == ID_TOOL_SPLIT || command == ID_TOOL_JOIN || command >= ID_TOOL_IHEX_BIN) { CFileDialog dialog(FALSE,command == ID_TOOL_ARRAY ? L"h" : L"bin",command == ID_TOOL_ARRAY ? L"data.h" : command == ID_TOOL_SPLIT ? L"split" : L"output.bin",OFN_OVERWRITEPROMPT,L"All files (*.*)|*.*||",this); path_dialog_ = true; const auto answer = dialog.DoModal(); path_dialog_ = false; if (answer != IDOK) return; output = dialog.GetPathName().GetString(); }
    StartBinaryTool(command,output,pattern,paths,part);
}

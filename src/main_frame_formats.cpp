#include "main_frame.h"
#include "workspace_frame.h"
#include "resource.h"
#include <sstream>

using namespace simple_editor::core;
bool CMainFrame::StartConvertedCopy(const std::wstring& path,const TextOutputOptions& options) {
    if (IsBusy() || comparison_session_ || !document_.IsOpen()) return false;
    if (text_stale_) RefreshText(); if (!text_editable_) { status_.SetPaneText(0,L"Choose the correct input decoding before exporting a text copy."); return false; }
    CString value; text_.GetWindowTextW(value); const auto text = TextFromEditControl(text_original_,std::wstring(value.GetString(),value.GetLength()));
    const std::vector<ProtectedInput> inputs{{document_.Path(),document_.SourceIdentity()}}; job_locked_paths_ = {path};
    if (workspace_ && !workspace_->LockPaths(job_locked_paths_,true,&job_)) { job_locked_paths_.clear(); status_.SetPaneText(0,L"Output has unsaved changes or is busy."); return false; } job_locks_held_ = workspace_ != nullptr;
    cancelled_ = false; progress_ = 0; shown_progress_ = UINT_MAX; job_kind_ = EditorJobKind::Export;
    try { job_ = std::async(std::launch::async,[this,path,options,text,inputs]() { EditorJobResult result; result.kind = EditorJobKind::Export; SaveControl control; control.cancelled = &cancelled_; control.progress = [this](unsigned p) { progress_ = p; }; if (ExportTextOutput(text,options,inputs,path,control,&result.error)) result.message = L"Converted output copy saved: " + path; return result; }); }
    catch (...) { ReleaseJobLocks(); status_.SetPaneText(0,L"Could not start text conversion."); return false; } return true;
}
void CMainFrame::OnConvertCopy() {
    if (IsBusy() || comparison_session_ || !document_.IsOpen()) { status_.SetPaneText(0,L"Open the source in a file editor to convert an output copy."); return; }
    CValueDialog options(this,L"Output encoding and line endings",L"encoding,BOM,EOL. Encodings: utf8/utf16le/utf16be/big5/system. BOM: bom/no-bom. EOL: preserve/crlf/lf/cr. Example: utf8,bom,lf. Input decoding and the original file are retained.",L"utf8,no-bom,preserve");
    TextOutputOptions format;
    options.validate = [&](const std::wstring& value,std::wstring*) { std::wstring fields[3]; std::wistringstream input(value); for (auto& field : fields) { if (!std::getline(input,field,L',')) return false; const auto begin = field.find_first_not_of(L" \t"), end = field.find_last_not_of(L" \t"); if (begin == std::wstring::npos) return false; field = field.substr(begin,end - begin + 1); for (auto& ch : field) ch = static_cast<wchar_t>(towlower(ch)); } std::wstring extra; if (std::getline(input,extra,L',')) return false;
        if (fields[0] == L"utf8") format.codec = {TextEncoding::Utf8,CP_UTF8,false}; else if (fields[0] == L"utf16le") format.codec = {TextEncoding::Utf16Le,1200,false}; else if (fields[0] == L"utf16be") format.codec = {TextEncoding::Utf16Be,1201,false}; else if (fields[0] == L"big5") format.codec = {TextEncoding::Big5,950,false}; else if (fields[0] == L"system") format.codec = {TextEncoding::System,GetACP(),false}; else return false;
        if (fields[1] != L"bom" && fields[1] != L"no-bom") return false; format.codec.bom = fields[1] == L"bom"; if (format.codec.bom && (format.codec.encoding == TextEncoding::Big5 || format.codec.encoding == TextEncoding::System)) return false;
        if (fields[2] == L"preserve") format.eol = OutputEol::Preserve; else if (fields[2] == L"crlf") format.eol = OutputEol::CrLf; else if (fields[2] == L"lf") format.eol = OutputEol::Lf; else if (fields[2] == L"cr") format.eol = OutputEol::Cr; else return false; return true;
    };
    path_dialog_ = true; const auto accepted = options.DoModal(); path_dialog_ = false; if (accepted != IDOK) return;
    CFileDialog output(FALSE,L"txt",L"converted.txt",OFN_OVERWRITEPROMPT,L"All files (*.*)|*.*||",this); path_dialog_ = true; const auto answer = output.DoModal(); path_dialog_ = false; if (answer == IDOK) StartConvertedCopy(output.GetPathName().GetString(),format);
}

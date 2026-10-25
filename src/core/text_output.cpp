#include "text_output.h"
#include "line_merge.h"
namespace simple_editor::core {
bool ConvertTextOutput(const std::wstring& text,const TextOutputOptions& options,std::vector<std::uint8_t>* bytes,std::wstring* error) {
    if (text.size() > 64 * 1024 * 1024 || options.codec.encoding == TextEncoding::Auto || (options.codec.bom && options.codec.encoding != TextEncoding::Utf8 && options.codec.encoding != TextEncoding::Utf16Le && options.codec.encoding != TextEncoding::Utf16Be)) { if (error) *error = L"Choose an explicit output encoding. BOM applies only to UTF encodings. Text is limited to 64 million UTF-16 units."; return false; }
    std::wstring converted;
    if (options.eol == OutputEol::Preserve) converted = text;
    else for (const auto& line : SplitLines(text)) { converted += line.text; if (!line.ending.empty()) converted += options.eol == OutputEol::CrLf ? L"\r\n" : options.eol == OutputEol::Lf ? L"\n" : L"\r"; }
    return EncodeText(converted,options.codec,true,bytes,error);
}
bool ExportTextOutput(const std::wstring& text,const TextOutputOptions& options,const std::vector<ProtectedInput>& inputs,const std::filesystem::path& path,const SaveControl& control,std::wstring* error,bool require_new) {
    FileStamp stamp; if (!InspectFile(path,&stamp,error)) return false; std::error_code code; const auto output = std::filesystem::absolute(path,code).lexically_normal(); if (code) return false;
    for (const auto& input : inputs) { const auto source = std::filesystem::absolute(input.path,code).lexically_normal(); if (code || _wcsicmp(output.c_str(),source.c_str()) == 0 || (stamp.exists && input.identity != std::pair<DWORD,std::uint64_t>{} && std::make_pair(stamp.volume,stamp.identity) == input.identity)) { if (error) *error = L"Choose an output copy separate from every input file."; return false; } }
    StreamOutput stream; if (!stream.Open(path,error,require_new)) return false;
    if (control.cancelled && *control.cancelled) { if (error) *error = L"Text conversion cancelled."; return false; }
    std::vector<std::uint8_t> bytes; if (!ConvertTextOutput(text,options,&bytes,error) || !stream.Write(bytes.data(),bytes.size(),error)) return false;
    return stream.Commit(control,error);
}
}

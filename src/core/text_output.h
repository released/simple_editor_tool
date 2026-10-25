#pragma once
#include "text_codec.h"
#include "stream_output.h"
namespace simple_editor::core {
enum class OutputEol { Preserve, CrLf, Lf, Cr };
struct TextOutputOptions { TextCodec codec; OutputEol eol = OutputEol::Preserve; };
struct ProtectedInput { std::filesystem::path path; std::pair<DWORD,std::uint64_t> identity{}; };
bool ConvertTextOutput(const std::wstring& text,const TextOutputOptions& options,std::vector<std::uint8_t>* bytes,std::wstring* error);
bool ExportTextOutput(const std::wstring& text,const TextOutputOptions& options,const std::vector<ProtectedInput>& inputs,const std::filesystem::path& output,const SaveControl& control,std::wstring* error,bool require_new = false);
}

#pragma once
#include "text_output.h"
#include "line_merge.h"
namespace simple_editor::core {
enum class TextReportFormat { Html, Summary, UnifiedPatch };
struct TextReportStats { std::uint64_t blocks = 0, equal = 0, added = 0, removed = 0, hunks = 0,ignored = 0; };
bool BuildUnifiedPatch(const std::wstring& left,const std::wstring& right,const std::wstring& left_name,const std::wstring& right_name,const std::atomic_bool& cancel,std::wstring* patch,TextReportStats* stats,std::wstring* error);
bool ApplyUnifiedPatch(const std::wstring& source,const std::wstring& patch,const std::atomic_bool& cancel,std::wstring* output,TextReportStats* stats,std::wstring* error);
bool ExportTextReport(const std::wstring& left,const std::wstring& right,const std::vector<ProtectedInput>& inputs,const std::filesystem::path& output,TextReportFormat format,const SaveControl& control,TextReportStats* stats,std::wstring* error,bool require_new = false,const LineOptions* options = nullptr,const std::wstring& rule_note = L"");
}

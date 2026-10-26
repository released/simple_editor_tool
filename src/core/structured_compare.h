#pragma once
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include "json_rules.h"
namespace simple_editor::core {
enum class StructuredFormat { Csv,Tsv,Json };
struct FieldDifference { std::wstring path,left,right,state; };
struct StructuredComparison { std::uint64_t compared = 0,different = 0,ignored = 0; bool truncated = false; std::vector<FieldDifference> rows; };
struct TableColumn { unsigned left = 0,right = 0; bool key = false; std::wstring tolerance; };
struct TableCompareOptions { bool header = false; std::vector<TableColumn> columns; };
bool ParseTableOptions(const std::wstring& text,TableCompareOptions* options,std::wstring* error);
bool CompareStructured(const std::wstring& left,const std::wstring& right,StructuredFormat format,const std::atomic_bool& cancel,StructuredComparison* result,std::wstring* error,const TableCompareOptions& options = {},const JsonCompareOptions& json_options = {});
}

#pragma once
#include "structured_compare.h"
namespace simple_editor::core {
bool ReadDelimitedTable(const std::wstring& text,wchar_t delimiter,const std::atomic_bool& cancel,std::vector<std::vector<std::wstring>>* rows,std::wstring* error);
bool CompareMappedTable(const std::wstring& left,const std::wstring& right,StructuredFormat format,const TableCompareOptions& options,const std::atomic_bool& cancel,StructuredComparison* result,std::wstring* error);
}

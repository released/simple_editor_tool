#pragma once
#include "binary_document.h"
namespace simple_editor::core {
struct BinaryField { std::wstring path,type,value; std::uint64_t offset = 0,length = 0; bool valid = false; };
struct BinaryTemplateResult { std::vector<BinaryField> fields; std::uint64_t base = 0; };
bool InspectBinaryTemplate(const BinaryDocument& document,const std::wstring& definition,std::uint64_t base,const SaveControl& control,BinaryTemplateResult* result,std::wstring* error);
}

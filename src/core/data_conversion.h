#pragma once
#include "binary_document.h"
namespace simple_editor::core {
enum class DataFormat { Hex,Base64,Utf8 };
struct DataConversion { std::vector<std::uint8_t> bytes; std::string output; };
bool ConvertData(const std::string&,DataFormat input,DataFormat output,const SaveControl&,DataConversion*,std::wstring*);
}

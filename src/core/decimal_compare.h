#pragma once
#include <string>
namespace simple_editor::core {
bool DecimalWithin(const std::wstring& left,const std::wstring& right,const std::wstring& tolerance,bool* equal);
}

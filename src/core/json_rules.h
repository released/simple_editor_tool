#pragma once
#include <string>
#include <vector>
namespace simple_editor::core {
using JsonPointer = std::vector<std::wstring>;
struct JsonArrayKey { JsonPointer path,key; };
struct JsonCompareOptions { std::vector<JsonPointer> ignored; std::vector<JsonArrayKey> arrays; };
bool ParseJsonRules(const std::wstring&,JsonCompareOptions*,std::wstring*);
}

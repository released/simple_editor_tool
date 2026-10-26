#pragma once
#include "text_review.h"
namespace simple_editor::core {
bool CppFunctionOutline(const std::wstring&,const std::atomic_bool&,std::vector<MarkdownHeading>*,bool* truncated);
}

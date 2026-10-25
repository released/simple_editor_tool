#pragma once
#include "text_page.h"
#include "file_operations.h"
namespace simple_editor::core {
struct LogTailResult { TextPage page; FileStamp stamp; bool changed = false; std::wstring state; };
bool ReadLogTail(const std::filesystem::path& path,TextEncoding encoding,const FileStamp& previous,const SaveControl& control,LogTailResult* result,std::wstring* error);
}

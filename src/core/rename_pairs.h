#pragma once
#include "folder_sync.h"
namespace simple_editor::core {
struct ContentPair { std::filesystem::path left,right; FileStamp left_stamp,right_stamp; std::wstring hash,state; std::size_t left_count = 0,right_count = 0; };
bool FindContentPairs(const FolderScan& scan,const SaveControl& control,std::vector<ContentPair>* pairs,std::wstring* error);
}

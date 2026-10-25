#pragma once
#include "multi_file_search.h"
#include "../core/folder_sync.h"
namespace simple_editor::services {
struct BatchReplaceFile {
    std::filesystem::path relative; simple_editor::core::FileStamp stamp;
    std::vector<std::uint8_t> output; std::wstring before,after,source_hash,output_hash;
    std::uint64_t count = 0;
};
struct BatchReplacePlan { std::filesystem::path root; simple_editor::core::FileStamp root_stamp; std::vector<BatchReplaceFile> files; std::vector<std::wstring> errors; std::uint64_t matches = 0,scanned = 0,failed = 0; };
struct BatchReplaceResult { simple_editor::core::SyncResult sync; std::filesystem::path staging; std::wstring error; };
bool PlanFolderReplace(const std::filesystem::path& root,const MultiSearchRequest& request,const simple_editor::core::SaveControl& control,BatchReplacePlan* plan,std::wstring* error);
BatchReplaceResult ApplyFolderReplace(const BatchReplacePlan& plan,const std::vector<std::size_t>& selected,const simple_editor::core::SaveControl& control);
}

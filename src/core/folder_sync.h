#pragma once
#include "file_operations.h"
#include <functional>

namespace simple_editor::core {
enum class FolderState { Equal, Different, LeftOnly, RightOnly, TypeConflict, Error };
enum class FolderCriterion { Content, Size, SizeTime, Sha256 };
enum class FolderNewness { SameTime, LeftNewer, RightNewer };
struct FolderRow {
    std::filesystem::path relative;
    FileStamp left, right;
    FolderState state = FolderState::Equal;
    std::wstring error;
    FolderNewness newness = FolderNewness::SameTime;
    std::wstring text_state,text_error;
};
struct FolderScan {
    std::filesystem::path left_root, right_root;
    FileStamp left_identity, right_identity;
    std::vector<FolderRow> rows;
};
struct FolderOptions {
    bool recursive = true; std::wstring include = L"*", exclude;
    FolderCriterion criterion = FolderCriterion::Content;
    unsigned time_tolerance_seconds = 0;
    std::wstring text_rules;
};
FolderNewness CompareFileTimes(const FileStamp& left,const FileStamp& right,unsigned tolerance_seconds = 0);
enum class SyncAction { Copy, CreateDirectory, RemoveToBackup };
struct SyncOperation {
    std::filesystem::path relative; SyncAction action; FileStamp source,target;
    std::wstring source_sha256,target_sha256;
};
struct SyncPlan {
    std::filesystem::path source_root, target_root;
    FileStamp source_identity, target_identity;
    std::vector<SyncOperation> operations;
    bool recovery = false;
};
struct SyncResult { std::size_t completed = 0; bool cancelled = false; std::filesystem::path backup,journal; std::wstring error; };
bool ScanFolders(const std::filesystem::path& left, const std::filesystem::path& right, const FolderOptions& options,
                  const std::atomic_bool& cancel, const std::function<void(unsigned)>& progress, FolderScan* scan, std::wstring* error);
bool BuildSyncPlan(const FolderScan& scan, const std::vector<std::size_t>& selected, bool left_to_right, bool mirror,
                    SyncPlan* plan, std::wstring* error);
SyncResult ApplySyncPlan(const SyncPlan& plan, const std::atomic_bool& cancel, const std::function<void(unsigned)>& progress,
                         const std::function<bool(const std::filesystem::path&)>& writable = {});
bool MatchFileFilter(const std::wstring& name, const std::wstring& patterns);
} // namespace simple_editor::core

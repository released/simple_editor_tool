#pragma once
#include "folder_sync.h"
namespace simple_editor::core {
struct BaselineSide { bool exists = false, directory = false; std::uint64_t size = 0; std::wstring sha256; };
struct BaselineEntry { std::filesystem::path relative; BaselineSide left,right; };
struct FolderBaseline { std::filesystem::path left_root,right_root; FileStamp left_identity,right_identity; std::vector<BaselineEntry> entries; };
enum class SyncPolicy { Update, Mirror, Newer, NewerBoth, BaselineBoth };
struct SyncConflict { std::filesystem::path relative; std::wstring reason; };
struct SyncBundle { std::vector<SyncPlan> plans; std::vector<SyncConflict> conflicts; std::wstring note; };
bool CaptureFolderBaseline(const FolderScan& scan,const std::atomic_bool& cancel,const std::function<void(unsigned)>& progress,FolderBaseline* output,std::wstring* error);
bool SaveFolderBaseline(const std::filesystem::path& path,const FolderBaseline& baseline,std::wstring* error);
bool LoadFolderBaseline(const std::filesystem::path& path,FolderBaseline* baseline,std::wstring* error);
bool BuildPolicySyncPlan(const FolderScan& scan,const std::vector<std::size_t>& selected,bool left_to_right,SyncPolicy policy,unsigned time_tolerance,bool propagate_deletions,const FolderBaseline* baseline,const std::atomic_bool& cancel,const std::function<void(unsigned)>& progress,SyncBundle* output,std::wstring* error);
}

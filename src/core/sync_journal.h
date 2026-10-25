#pragma once
#include "folder_sync.h"
namespace simple_editor::core {
struct SyncJournalEntry {
    bool started = false,completed = false;
    FileStamp post,backup;
    std::wstring post_hash,backup_hash,error;
};
struct SyncJournal {
    SyncPlan plan;
    std::filesystem::path path,backup;
    std::vector<SyncJournalEntry> entries;
};
class SyncJournalWriter {
public:
    ~SyncJournalWriter();
    bool Begin(const SyncPlan& plan,const std::filesystem::path& backup,std::wstring* error);
    bool Intent(std::size_t index,std::wstring* error);
    bool Completed(std::size_t index,const SyncOperation& operation,std::wstring* error);
    void Failed(std::size_t index,const std::wstring& error);
    std::filesystem::path Path() const { return journal_.path; }
private:
    bool Record(std::size_t index,unsigned phase,const SyncJournalEntry& entry,std::wstring* error);
    bool Write(const std::vector<std::uint8_t>& bytes,std::wstring* error);
    HANDLE file_ = INVALID_HANDLE_VALUE;
    SyncJournal journal_;
};
bool LoadSyncJournal(const std::filesystem::path& path,SyncJournal* journal,std::wstring* error);
bool BuildSyncRetry(const SyncJournal& journal,SyncPlan* plan,std::wstring* error);
bool BuildSyncRecovery(const SyncJournal& journal,SyncPlan* plan,std::wstring* error);
bool IsRecoverySource(const SyncPlan& plan);
}

#pragma once
#include "workspace_session.h"
namespace simple_editor::core {
struct RecoveryDraft {
    std::wstring original_path,label;
    bool native_text = false;
    std::string utf8;
    std::wstring original_text;
    TextCodec original_codec;
    std::shared_ptr<BinaryDocument> document;
    WorkspaceEntry view;
};
struct RecoveryItem {
    std::wstring original_path,label,sha256;
    std::uint64_t size = 0;
    WorkspaceEntry view;
};
struct RecoveryIndex {
    std::filesystem::path root;
    std::wstring generation,previous,workspace_sha256;
    bool clean_exit = false;
    std::vector<WorkspaceEntry> workspace;
    std::vector<RecoveryItem> drafts;
};
struct RecoverySaveResult { bool committed = false,cancelled = false; std::wstring error,warning; RecoveryIndex index; };
RecoverySaveResult SaveRecovery(const std::filesystem::path& root,const std::vector<WorkspaceEntry>& workspace,const std::vector<RecoveryDraft>& drafts,bool clean_exit,const SaveControl& control);
bool LoadRecovery(const std::filesystem::path& root,RecoveryIndex* index,std::wstring* error);
bool LoadPreviousRecovery(const RecoveryIndex& current,RecoveryIndex* index,std::wstring* error);
bool ReadRecoveryDraft(const RecoveryIndex& index,std::size_t item,const SaveControl& control,std::vector<std::uint8_t>* bytes,std::wstring* error);
bool ClearRecovery(const std::filesystem::path& root,std::wstring* error);
}

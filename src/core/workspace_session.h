#pragma once
#include "file_operations.h"
#include "text_codec.h"

namespace simple_editor::core {
struct WorkspaceEntry {
    std::wstring hex_bookmarks,table_rules,text_rules,manual_anchors,crc_profile;
    bool dual_view = false;
    std::wstring folder_text_rules;
    unsigned hex_decoding[3] = {},context_lines = 0;
    bool folder_tree = false;
    bool comparison = false, active = false;
    std::wstring left, right, base, include = L"*", exclude;
    std::wstring binary_region;
    unsigned hex_rows[3] = {16,16,16}, hex_groups[3] = {8,8,8};
    std::uint64_t hex_bases[3] = {};
    int mode = 0, page = 0, encoding = 0, advanced_encoding = 0;
    int zoom[3] = {100,100,100};
    unsigned options = 88;
    unsigned folder_criterion = 0, folder_tolerance = 0, folder_filter = 0, folder_sort = 0;
    bool folder_sort_descending = false;
    unsigned sync_policy = 0;
    bool sync_deletions = false;
    std::wstring sync_baseline;
    std::uint64_t caret = 0, top_row = 0;
};
bool SaveWorkspaceFile(const std::filesystem::path& path, const std::vector<WorkspaceEntry>& entries, std::wstring* error);
bool LoadWorkspaceFile(const std::filesystem::path& path, std::vector<WorkspaceEntry>* entries, std::wstring* error);
} // namespace simple_editor::core

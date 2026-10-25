#include "rename_pairs.h"
#include "binary_analysis.h"
#include <algorithm>
#include <map>
#include <set>
namespace simple_editor::core {
bool FindContentPairs(const FolderScan& scan,const SaveControl& control,std::vector<ContentPair>* output,std::wstring* error) {
    struct Group { std::vector<const FolderRow*> left,right; }; std::map<std::uint64_t,Group> sizes;
    for (const auto& row : scan.rows) { if (row.state == FolderState::LeftOnly && !row.left.directory) sizes[row.left.size].left.push_back(&row); else if (row.state == FolderState::RightOnly && !row.right.directory) sizes[row.right.size].right.push_back(&row); }
    std::vector<ContentPair> pairs; std::size_t completed = 0,total = 0; for (const auto& group : sizes) if (!group.second.left.empty() && !group.second.right.empty()) total += group.second.left.size() + group.second.right.size();
    for (const auto& size : sizes) { if (size.second.left.empty() || size.second.right.empty()) continue; std::map<std::wstring,Group> hashes;
        for (bool first : {true,false}) for (auto* row : first ? size.second.left : size.second.right) { if (control.cancelled && *control.cancelled) { if (error) *error = L"Content pairing cancelled."; return false; } const auto path = (first ? scan.left_root : scan.right_root) / row->relative; const auto& expected = first ? row->left : row->right; FileStamp now; if (!InspectFile(path,&now,error) || !SameFileState(expected,now)) { if (error) *error = L"A candidate changed since the folder scan."; return false; } BinaryDocument file; BinaryAnalysis analysis; SaveControl read; read.cancelled = control.cancelled; if (!file.Open(path.wstring(),error) || !AnalyzeBinary(file,0,file.Size(),read,&analysis,error) || !InspectFile(path,&now,error) || !SameFileState(expected,now)) { if (error && error->empty()) *error = L"A candidate changed during content hashing."; return false; } auto& group = hashes[analysis.sha256]; (first ? group.left : group.right).push_back(row); ++completed; if (control.progress) control.progress(static_cast<unsigned>(completed * 100 / (std::max<std::size_t>)(1,total))); }
        for (const auto& hash : hashes) { if (hash.second.left.empty() || hash.second.right.empty()) continue; ContentPair pair; pair.hash = hash.first; pair.left_count = hash.second.left.size(); pair.right_count = hash.second.right.size(); if (pair.left_count == 1 && pair.right_count == 1) { const auto* left = hash.second.left[0]; const auto* right = hash.second.right[0]; pair.left = scan.left_root / left->relative; pair.right = scan.right_root / right->relative; pair.left_stamp = left->left; pair.right_stamp = right->right; pair.state = left->relative.parent_path() == right->relative.parent_path() ? L"Renamed (same content)" : L"Moved / renamed (same content)"; } else pair.state = L"Ambiguous duplicate content"; if (pairs.size() < 20000) pairs.push_back(std::move(pair)); else { if (error) *error = L"Content pairing exceeds 20,000 groups; narrow the folder filters."; return false; } }
    }
    if (control.cancelled && *control.cancelled) return false; *output = std::move(pairs); return true;
}
}

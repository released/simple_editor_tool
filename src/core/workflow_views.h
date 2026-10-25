#pragma once
#include "folder_sync.h"
#include <map>
#include <set>
namespace simple_editor::core {
struct ContextRows { std::vector<std::size_t> rows; std::map<std::size_t,std::size_t> hidden; };
ContextRows BuildContextRows(const std::vector<unsigned char>& important,unsigned context,const std::set<std::size_t>& expanded);
struct FolderTreeRows { std::vector<std::size_t> rows; std::vector<unsigned> depth; std::vector<std::uint64_t> differences,errors; };
FolderTreeRows BuildFolderTreeRows(const FolderScan&,const std::vector<std::size_t>& eligible,const std::set<std::size_t>& expanded);
}

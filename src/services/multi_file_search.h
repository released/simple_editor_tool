#pragma once
#include "../core/binary_document.h"
#include "../core/file_operations.h"
#include "../core/text_codec.h"
#include "../ui/native_search.h"
#include <memory>
namespace simple_editor::services {
struct MultiSearchRequest {
    NativeSearchOptions text;
    int mode = 0;
    simple_editor::core::TextEncoding encoding = simple_editor::core::TextEncoding::Auto;
    bool recursive = true;
    std::wstring include = L"*",exclude;
};
struct MultiSearchSource {
    std::wstring path;
    std::uint64_t session = 0,generation = 0,revision = 0;
    bool native_text = false;
    std::string utf8;
    std::shared_ptr<simple_editor::core::BinaryDocument> document;
    simple_editor::core::FileStamp stamp;
};
struct MultiSearchMatch {
    std::wstring path;
    NativeSearchMatch match;
    std::uint64_t session = 0,generation = 0,revision = 0;
    bool binary = false,native_text = false;
    simple_editor::core::TextEncoding encoding = simple_editor::core::TextEncoding::Auto;
    simple_editor::core::FileStamp stamp;
};
struct MultiSearchSummary { std::uint64_t files = 0,matches = 0,errors = 0; bool truncated = false,cancelled = false; std::wstring error; std::vector<std::wstring> details; };
using MultiSearchBatch = std::function<bool(std::vector<MultiSearchMatch>)>;
MultiSearchSummary SearchManyFiles(std::vector<MultiSearchSource> sources,const std::filesystem::path& folder,const MultiSearchRequest& request,const std::atomic_bool& cancel,const MultiSearchBatch& consume,const std::function<void(std::uint64_t)>& progress);
}

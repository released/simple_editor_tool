#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
struct NativeSearchOptions {
    std::wstring find, replacement;
    int action = 0, syntax = 0;
    bool match_case = true, whole_word = false, previous = false, wrap = true;
};
struct NativeSearchMatch { std::uint64_t offset = 0, length = 0, line = 0; std::string preview; };
struct NativeSearchPlan {
    std::string output;
    std::vector<NativeSearchMatch> matches;
    std::uint64_t count = 0, cursor = 0, length = 0;
    bool changed = false, found = false, wrapped = false, truncated = false;
};
bool PlanEditorSearch(const std::string& input,const NativeSearchOptions& options,std::uint64_t begin,std::uint64_t end,std::uint64_t selection_start,std::uint64_t selection_end,const std::atomic_bool& cancel,const std::function<void(unsigned)>& progress,NativeSearchPlan* plan,std::wstring* error);
bool DecodeSearchEscapes(const std::wstring& input,std::wstring* output,std::wstring* error);

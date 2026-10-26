#pragma once
#include "line_merge.h"
#include "text_output.h"
#include <map>
namespace simple_editor::core {
struct ReviewNote { std::wstring text; bool reviewed = false; };
struct ComparisonNotes { std::wstring fingerprint; std::map<std::size_t,ReviewNote> notes; };
bool FingerprintComparison(const TextMergeModel&,const std::atomic_bool&,std::wstring*,std::wstring* error);
bool ParseReviewNotes(const std::wstring&,ComparisonNotes*,std::wstring* error);
bool ReadReviewNotes(const std::filesystem::path&,ComparisonNotes*,std::wstring* error);
bool SaveReviewNotes(const std::filesystem::path&,const ComparisonNotes&,const std::vector<ProtectedInput>&,const SaveControl&,std::wstring* error);
bool GuardReviewOutput(const std::filesystem::path&,const std::vector<ProtectedInput>&,FileStamp*,std::wstring* error);
bool ExportReviewedHtml(const TextMergeModel&,const ComparisonNotes&,const std::vector<ProtectedInput>&,const std::filesystem::path&,const SaveControl&,std::wstring* error);
}

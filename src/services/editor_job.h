#pragma once
#include "../core/binary_document.h"
#include "../core/diff_engine.h"
#include <memory>

struct NativeSearchPlan;
namespace simple_editor::core { struct BinaryTemplateResult; struct StringExtraction; struct ModifiedRange; struct MarkdownHeading; struct IpsSummary; struct TextByteRange; struct UnicodeNotices; struct BinaryAnalysis; }
enum class EditorJobKind { Compare, Find, ReplaceAll, Save, Export, Tool, Transform, TextSearch, ByteSearch, Patch, Template, TextCommand, WorkflowChanges, WorkflowStrings, DiskReview, Outline, IpsImport, IpsExport, SelectionMap, UnicodeScan, Statistics };
struct EditorJobResult {
    EditorJobKind kind = EditorJobKind::Compare;
    std::wstring error;
    std::wstring message;
    std::wstring details;
    std::wstring open_path;
    simple_editor::core::FileStamp stamp;
    std::shared_ptr<NativeSearchPlan> search;
    std::shared_ptr<std::vector<simple_editor::core::MarkdownHeading>> headings;
    std::shared_ptr<simple_editor::core::IpsSummary> ips;
    std::shared_ptr<simple_editor::core::TextByteRange> selection_map;
    std::shared_ptr<simple_editor::core::UnicodeNotices> unicode;
    std::shared_ptr<simple_editor::core::BinaryAnalysis> analysis;
    std::shared_ptr<simple_editor::core::BinaryTemplateResult> fields;
    std::shared_ptr<simple_editor::core::StringExtraction> strings;
    std::shared_ptr<std::vector<simple_editor::core::ModifiedRange>> modified;
    std::vector<simple_editor::core::Difference> differences;
    std::unique_ptr<simple_editor::core::BinaryDocument> document;
    std::uint64_t found = UINT64_MAX;
    std::size_t count = 0;
    bool truncated = false;
    bool wrapped = false;
    bool saved = false;
};

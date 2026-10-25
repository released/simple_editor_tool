#include <afxwin.h>
#include "multi_file_search.h"
#include "../ui/code_editor.h"
#include "../core/folder_sync.h"
#include "../core/line_merge.h"
#include "../core/pattern_search.h"
#include <algorithm>
#include <set>
using namespace simple_editor::core;
namespace simple_editor::services {
MultiSearchSummary SearchManyFiles(std::vector<MultiSearchSource> sources,const std::filesystem::path& folder,const MultiSearchRequest& request,const std::atomic_bool& cancel,const MultiSearchBatch& consume,const std::function<void(std::uint64_t)>& progress) {
    MultiSearchSummary result; std::vector<MultiSearchMatch> batch; std::uint64_t retained = 0; BytePattern pattern;
    if (request.mode < 0 || request.mode > 4 || request.text.find.empty() || (request.mode >= 3 && !ParseBytePattern(request.text.find,request.mode == 4,&pattern,&result.error))) { if (result.error.empty()) result.error = L"Choose a non-empty valid search pattern."; return result; }
    const auto issue = [&](const std::wstring& path,const std::wstring& error) { ++result.errors; if (result.details.size() < 200) result.details.push_back(path + L": " + error); };
    const auto flush = [&]() { if (batch.empty()) return true; auto ready = std::move(batch); batch.clear(); return !cancel && consume(std::move(ready)); };
    const auto publish = [&](const MultiSearchSource& source,NativeSearchMatch match) {
        if (retained >= 100000) { result.truncated = true; return !cancel; }
        MultiSearchMatch row; row.path = source.path; row.match = std::move(match); row.session = source.session; row.generation = source.generation; row.revision = source.revision; row.native_text = source.native_text; row.stamp = source.stamp; row.binary = request.mode >= 3; row.encoding = request.encoding; batch.push_back(std::move(row)); ++retained;
        return batch.size() < 256 || flush();
    };
    const auto scan = [&](MultiSearchSource source) {
        if (cancel) return false; ++result.files; if (progress) progress(result.files); std::wstring error;
        if (request.mode >= 3 || !source.native_text) {
            if (!source.document) { source.document = std::make_shared<BinaryDocument>(); if (!InspectFile(source.path,&source.stamp,&error) || !source.document->Open(source.path,&error)) { issue(source.path,error); return true; } }
            if (!source.document->ValidateSource(true,&error)) { issue(source.path,error); return true; }
        }
        if (request.mode >= 3) {
            const auto match = [&](std::uint64_t at) { ++result.matches; NativeSearchMatch row; row.offset = at; row.length = pattern.bytes.size(); std::vector<std::uint8_t> bytes; if (!source.document->Read(at,24,&bytes)) { error = L"Input changed while generating match preview."; return false; } for (auto byte : bytes) { char value[4]{}; sprintf_s(value,"%02X ",byte); row.preview += value; } return publish(source,std::move(row)); };
            if (!ScanBytePattern(*source.document,pattern,0,source.document->Size(),cancel,match,{},&error) || !source.document->ValidateSource(true,&error)) { if (!cancel) issue(source.path,error.empty() ? L"Pattern scan stopped." : error); }
        } else {
            std::string text = source.utf8;
            if (!source.native_text) {
                if (source.document->Size() > 32 * 1024 * 1024) { issue(source.path,L"Text search limit is 32 MiB per file; use a Hex pattern for larger inputs."); return true; }
                std::vector<std::uint8_t> bytes; std::wstring decoded; TextCodec codec;
                if (!source.document->Read(0,static_cast<std::size_t>(source.document->Size()),&bytes) || !DecodeText(bytes,request.encoding,&decoded,&codec,&error) || !source.document->ValidateSource(true,&error)) { issue(source.path,error.empty() ? L"Could not decode source." : error); return true; }
                text = CCodeEditor::ToUtf8(TextForEditControl(decoded));
            }
            auto options = request.text; options.syntax = request.mode; options.action = retained < 100000 ? 4 : 3; options.previous = false; options.wrap = false; NativeSearchPlan plan;
            if (!PlanEditorSearch(text,options,0,text.size(),0,0,cancel,{},&plan,&error)) { if (!cancel) issue(source.path,error); return !cancel; }
            result.matches += plan.count; result.truncated = result.truncated || plan.truncated || (options.action == 3 && plan.count != 0);
            for (auto& match : plan.matches) if (!publish(source,std::move(match))) return false;
        }
        return !cancel;
    };
    bool keep = true;
    if (folder.empty()) { for (auto& source : sources) if (!(keep = scan(std::move(source)))) break; }
    else {
        std::error_code code; const auto root = std::filesystem::absolute(folder,code).lexically_normal(); FileStamp stamp;
        if (code || !InspectFile(root,&stamp,&result.error) || !stamp.exists || !stamp.directory) { if (result.error.empty()) result.error = L"Choose an existing folder."; return result; }
        std::vector<std::filesystem::path> pending{root}; std::size_t entries = 0;
        while (keep && !pending.empty() && !cancel) {
            const auto directory = pending.back(); pending.pop_back(); if (!CheckPlainPath(directory,&result.error)) break;
            std::filesystem::directory_iterator iterator(directory,code),end; if (code) { issue(directory.wstring(),L"Cannot enumerate folder."); code.clear(); continue; }
            while (keep && iterator != end && !cancel) {
                if (++entries > 200000) { result.error = L"Folder search exceeds 200,000 entries. Narrow the folder or filter."; keep = false; break; }
                const auto path = iterator->path(),relative = path.lexically_relative(root); const auto name = path.filename().wstring(); FileStamp item; std::wstring error;
                if (CompareStringOrdinal(name.c_str(),-1,L".SimpleEditorTool.backups",-1,TRUE) != CSTR_EQUAL && !(MatchFileFilter(name,request.exclude) || MatchFileFilter(relative.wstring(),request.exclude))) {
                    if (!InspectFile(path,&item,&error)) issue(path.wstring(),error);
                    else if (item.directory) { if (request.recursive) pending.push_back(path); }
                    else if (MatchFileFilter(name,request.include.empty() ? L"*" : request.include)) { MultiSearchSource source; source.path = path.wstring(); source.stamp = item; keep = scan(std::move(source)); }
                }
                iterator.increment(code); if (code) { issue(directory.wstring(),L"Folder enumeration failed or changed."); code.clear(); break; }
            }
        }
        FileStamp now; if (!InspectFile(root,&now,&result.error) || !SameFileState(now,stamp)) { if (result.error.empty()) result.error = L"Search root changed."; }
    }
    if (!cancel && keep && !flush()) keep = false; result.cancelled = cancel || !keep; return result;
}
}

#include "log_tail.h"
namespace simple_editor::core {
bool ReadLogTail(const std::filesystem::path& path,TextEncoding encoding,const FileStamp& previous,const SaveControl& control,LogTailResult* output,std::wstring* error) {
    if (encoding == TextEncoding::Big5 || encoding == TextEncoding::System) { if (error) *error = L"Live tail supports UTF-8 and UTF-16. Use sequential text pages for legacy encodings."; return false; }
    FileStamp current; if (!InspectFile(path,&current,error) || !current.exists || current.directory) { if (error && error->empty()) *error = L"Waiting for the saved log path to exist."; return false; } if (control.cancelled && *control.cancelled) return false;
    LogTailResult result; result.stamp = current; if (SameFileState(current,previous)) { result.state = L"No new data"; *output = std::move(result); return true; }
    BinaryDocument document; if (!document.Open(path.wstring(),error)) return false; const auto begin = document.Size() > 256 * 1024 ? document.Size() - 256 * 1024 : 0;
    if (!ReadTextPage(document,begin,encoding,false,control,&result.page,error) || !InspectFile(path,&current,error) || !SameFileState(current,result.stamp)) { if (error && error->empty()) *error = L"Log changed during reading; retaining the previous view and retrying."; return false; }
    result.changed = true; result.state = !previous.exists ? L"Following saved disk content" : current.volume != previous.volume || current.identity != previous.identity ? L"File rotated: following the new file at this path" : current.size < previous.size ? L"File truncated: following its new content" : L"File updated"; *output = std::move(result); return true;
}
}

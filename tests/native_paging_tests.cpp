#include "../src/core/text_page.h"
#include "../src/core/diff_engine.h"
#include <winioctl.h>
#include <psapi.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
using namespace simple_editor::core;
int RunPagingTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    const auto path = root / L"page.txt"; const auto write = [&](const std::vector<std::uint8_t>& bytes) { std::ofstream out(path,std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); };
    std::vector<std::uint8_t> bytes(31,'a'); bytes.insert(bytes.end(),{0xF0,0x9F,0x9A,0x80,'z',0xEF,0xBB,0xBF,'x'}); write(bytes); BinaryDocument source; std::wstring error; source.Open(path.wstring(),&error); TextPage first,next,jump;
    check(ReadTextPage(source,0,TextEncoding::Utf8,true,{},&first,&error,32) && first.end == 31 && first.text == std::wstring(31,L'a'),L"UTF-8 page ends never split a four-byte code point");
    check(ReadTextPage(source,first.end,TextEncoding::Utf8,true,{},&next,&error,32) && next.text == L"\U0001F680z\uFEFFx" && next.end == bytes.size(),L"sequential UTF-8 pages preserve astral characters and embedded BOM values");
    check(ReadTextPage(source,33,TextEncoding::Utf8,false,{},&jump,&error,32) && jump.begin == 35 && jump.text == L"z\uFEFFx",L"random UTF-8 byte navigation advances to a complete character boundary"); source.Close();
    TextCodec codec{TextEncoding::Utf16Be,1201,true}; EncodeText(std::wstring(14,L'a') + L"\U0001F680z",codec,true,&bytes,&error); write(bytes); source.Open(path.wstring(),&error);
    check(ReadTextPage(source,0,TextEncoding::Auto,true,{},&first,&error,32) && first.end == 30 && first.codec.encoding == TextEncoding::Utf16Be && ReadTextPage(source,first.end,TextEncoding::Auto,true,{},&next,&error,32) && next.text == L"\U0001F680z",L"UTF-16 BOM detection and surrogate-safe boundaries retain both halves of a character");
    check(ReadTextPage(source,33,TextEncoding::Auto,false,{},&jump,&error,32) && jump.begin == 34 && jump.text == L"z",L"UTF-16 random byte navigation aligns code units and skips an incomplete surrogate"); source.Close();
    codec = {TextEncoding::Big5,950,false}; EncodeText(std::wstring(31,L'a') + L"\u4E2D\u6587",codec,false,&bytes,&error); write(bytes); source.Open(path.wstring(),&error);
    check(ReadTextPage(source,0,TextEncoding::Big5,true,{},&first,&error,32) && first.end == 31 && ReadTextPage(source,first.end,TextEncoding::Big5,true,{},&next,&error,32) && next.text == L"\u4E2D\u6587" && !ReadTextPage(source,32,TextEncoding::Big5,false,{},&jump,&error,32),L"legacy code pages retain lead/trail boundaries and refuse ambiguous random offsets"); source.Close();
    write({'a',0xF0,0x9F}); source.Open(path.wstring(),&error); const auto retained = first.text; check(!ReadTextPage(source,0,TextEncoding::Utf8,true,{},&first,&error,32) && first.text == retained,L"truncated EOF input is rejected without replacing the previous page"); source.Close();
    std::atomic_bool cancel{true}; SaveControl control; control.cancelled = &cancel; check(!ReadTextPage(source,0,TextEncoding::Utf8,true,control,&first,&error),L"page cancellation preserves prior results");
    const std::uint64_t size = 1024ULL * 1024 * 1024; const auto large = root / L"large-sparse.txt",other = root / L"other-sparse.txt";
    const auto sparse = [&](const std::filesystem::path& target,bool changed) { HANDLE file = CreateFileW(target.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr); if (file == INVALID_HANDLE_VALUE) return false; DWORD returned = 0; const bool marked = DeviceIoControl(file,FSCTL_SET_SPARSE,nullptr,0,nullptr,0,&returned,nullptr) != FALSE; LARGE_INTEGER end{}; end.QuadPart = size; bool ok = marked && SetFilePointerEx(file,end,nullptr,FILE_BEGIN) && SetEndOfFile(file); std::vector<char> block(256 * 1024,'A'); block.back() = '\n'; DWORD done = 0; end.QuadPart = 0; ok = ok && SetFilePointerEx(file,end,nullptr,FILE_BEGIN) && WriteFile(file,block.data(),static_cast<DWORD>(block.size()),&done,nullptr) && done == block.size(); end.QuadPart = size - block.size(); if (changed) block.back() = 'Z'; ok = ok && SetFilePointerEx(file,end,nullptr,FILE_BEGIN) && WriteFile(file,block.data(),static_cast<DWORD>(block.size()),&done,nullptr) && done == block.size(); CloseHandle(file); return ok; };
    if (!sparse(large,false) || !sparse(other,true)) { check(false,L"create owned 1 GiB sparse stress inputs"); return failures; }
    BinaryDocument a,b; a.Open(large.wstring(),&error); b.Open(other.wstring(),&error); PROCESS_MEMORY_COUNTERS before{sizeof(before)},after{sizeof(after)}; GetProcessMemoryInfo(GetCurrentProcess(),&before,sizeof(before)); const auto start = std::chrono::steady_clock::now();
    const bool first_ok = ReadTextPage(a,0,TextEncoding::Utf8,true,{},&first,&error), last_ok = ReadTextPage(a,size - 256 * 1024,TextEncoding::Utf8,false,{},&next,&error); const auto pages_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    check(first_ok && last_ok && first.text.size() == 256 * 1024 && next.end == size && next.text.back() == L'\n',L"1 GiB text source supports bounded first/last-page access without scanning the middle");
    cancel = false; std::uint64_t count = 0,last = 0; const auto compare_start = std::chrono::steady_clock::now(); const bool compared = CompareDocuments(a,b,false,cancel,[&](const DifferenceBatch& batch) { count += batch.size(); if (!batch.empty()) last = batch.back().left_offset; return true; },[](unsigned) {},&error); const auto compare_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - compare_start).count(); GetProcessMemoryInfo(GetCurrentProcess(),&after,sizeof(after));
    check(compared && count == 1 && last == size - 1,L"1 GiB streaming binary comparison reports the final byte difference exactly");
    const auto cancel_start = std::chrono::steady_clock::now(); const bool cancelled = CompareDocuments(a,b,false,cancel,[](const DifferenceBatch&) { return true; },[&](unsigned p) { if (p >= 1) cancel = true; },&error); const auto cancel_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cancel_start).count(); check(cancelled && cancel,L"large streaming comparisons observe cancellation during progress");
    std::wcout << L"MEASURE: sparse_bytes=" << size << L" pages_ms=" << pages_ms << L" compare_ms=" << compare_ms << L" cancel_ms=" << cancel_ms << L" working_set_before=" << before.WorkingSetSize << L" working_set_after=" << after.WorkingSetSize << L" peak_working_set=" << after.PeakWorkingSetSize << L'\n';
    return failures;
}

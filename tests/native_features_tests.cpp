#include "../src/core/binary_document.h"
#include "../src/core/search.h"
#include "../src/core/text_codec.h"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace simple_editor::core;

int RunFeatureTests(const std::filesystem::path& root) {
    int failures = 0;
    const auto check = [&](bool condition, const wchar_t* name) { if (!condition) { ++failures; std::wcerr << L"FAIL: " << name << L'\n'; } };
    std::uint64_t at = 0; std::vector<std::uint8_t> bytes; std::wstring error;
    check(ParseAddress(L" 0x100000000 ", &at) && at == 0x100000000ULL, L"64-bit hex address");
    check(ParseAddress(L"d:255", &at) && at == 255, L"decimal address");
    check(!ParseAddress(L"10000000000000000", &at) && !ParseAddress(L"-1", &at) && !ParseAddress(L"0x", &at), L"invalid address and overflow");
    check(ParseHexBytes(L"00 FF\t41", &bytes) && bytes == std::vector<std::uint8_t>({0,255,65}), L"hex byte search input");
    check(!ParseHexBytes(L"F", &bytes) && !ParseHexBytes(L"XY", &bytes), L"reject invalid hex input");

    const auto path = root / L"search.bin";
    std::vector<std::uint8_t> source(1024 * 1024 - 1, 'x'); source.insert(source.end(), {'A','B','A','B','A','B'});
    { std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<const char*>(source.data()), source.size()); }
    BinaryDocument doc; check(doc.Open(path.wstring(), &error), L"open search fixture");
    std::atomic_bool cancel(false); std::vector<std::uint64_t> matches;
    check(ScanMatches(doc, {'A','B'}, 0, doc.Size(), cancel, [&](auto offset) { matches.push_back(offset); return true; }, &error) && matches.size() == 3 && matches[0] == 1024 * 1024 - 1, L"stream search crosses 1 MiB boundary");
    auto snapshot = doc.Snapshot(&error); check(snapshot != nullptr, L"independent edit snapshot");
    check(doc.ReplaceMatches(matches, {'A','B'}, {'1','2','3'}), L"replace all expands matches");
    check(doc.Size() == source.size() + 3, L"replace all length");
    std::vector<std::uint8_t> tail; doc.Read(source.size() - 6, 20, &tail);
    check(tail == std::vector<std::uint8_t>({'1','2','3','1','2','3','1','2','3'}), L"expanded replace content");
    check(doc.Undo(&at) && at == matches[0], L"one undo reverses replace all");
    doc.Read(0, source.size(), &bytes); check(bytes == source && !doc.IsModified(), L"replace all undo restores original");
    check(doc.Redo(&at), L"replace all redo");
    check(doc.Undo(&at) && doc.ReplaceMatches(matches, {'A','B'}, {}), L"replace all deletes matches");
    check(doc.Undo(&at) && doc.Size() == source.size(), L"undo adjacent deletion offsets");
    if (snapshot) { snapshot->Read(0, source.size(), &bytes); check(bytes == source, L"snapshot unchanged by later edits"); }
    check(!doc.ReplaceMatches({matches[0], matches[0]}, {'A','B'}, {'Q'}), L"reject overlapping batch atomically");
    doc.Read(0, source.size(), &bytes); check(bytes == source, L"failed batch leaves bytes unchanged");
    check(!doc.Open((root / L"missing.bin").wstring(), &error) && doc.IsOpen() && doc.Size() == source.size(), L"failed open preserves current file");
    check(doc.SetByte(0, 'Q'), L"edit before save as"); const auto save_as = root / L"saved_as.bin";
    check(doc.Save(save_as.wstring(), &error) && doc.Path() == save_as.wstring() && !doc.IsModified(), L"save as adopts destination");
    check(doc.SetByte(1, 'Z') && doc.Save(L"", &error), L"save after save as uses new path");
    { std::ifstream original(path, std::ios::binary); check(original.get() == 'x', L"save as leaves original intact"); }

    cancel = true; int calls = 0;
    check(ScanMatches(doc, {'x'}, 0, doc.Size(), cancel, [&](auto) { ++calls; return true; }, &error) && calls == 0, L"cancelled search has no callbacks");
    const auto aligned_path = root / L"aligned-search.bin";
    { std::ofstream out(aligned_path, std::ios::binary); out << "AAAAAA"; }
    BinaryDocument aligned_doc; aligned_doc.Open(aligned_path.wstring(), &error); cancel = false; matches.clear();
    check(ScanMatches(aligned_doc, {'A','A'}, 1, 6, cancel, [&](auto offset) { matches.push_back(offset); return true; }, &error, 2) && matches == std::vector<std::uint64_t>({2,4}), L"UTF16 search stays aligned from odd caret");

    const std::wstring text = L"Line 1\n\x4E2D\x6587\r\n\xD83D\xDE03";
    for (auto encoding : {TextEncoding::Utf8, TextEncoding::Utf16Le, TextEncoding::Utf16Be}) {
        TextCodec codec; codec.encoding = encoding; codec.bom = true;
        std::vector<std::uint8_t> encoded; std::wstring decoded; TextCodec detected;
        check(EncodeText(text, codec, true, &encoded, &error), L"encode Unicode with BOM");
        check(DecodeText(encoded, TextEncoding::Auto, &decoded, &detected, &error) && decoded == text && detected.bom, L"BOM and newline round trip");
    }
    TextCodec big5; big5.encoding = TextEncoding::Big5; big5.code_page = 950;
    check(EncodeText(L"\x4E2D\x6587", big5, false, &bytes, &error), L"encode CP950");
    std::wstring decoded; TextCodec codec;
    check(DecodeText(bytes, TextEncoding::Big5, &decoded, &codec, &error) && decoded == L"\x4E2D\x6587", L"decode CP950");
    check(!EncodeText(L"\xD83D\xDE03", big5, false, &bytes, &error), L"reject lossy CP950 conversion");
    check(!DecodeText({0xFF,0xFE,0x41}, TextEncoding::Auto, &decoded, &codec, &error), L"reject truncated UTF16");
    check(!DecodeText({'A',0,'B'}, TextEncoding::Utf8, &decoded, &codec, &error), L"reject embedded NUL before editable control");
    check(DecodeText({}, TextEncoding::Auto, &decoded, &codec, &error) && decoded.empty(), L"empty text file");
    if (!failures) std::wcout << L"PASS: search, replace-all/undo, snapshots, addresses, text encodings and save-as\n";
    return failures;
}

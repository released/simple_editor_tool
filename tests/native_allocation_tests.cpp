#include "../src/core/binary_document.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <filesystem>

#ifndef _DEBUG
namespace {
thread_local long fail_after = -1;
thread_local bool injected = false;
void* Allocate(std::size_t size) {
    if (fail_after == 0) { fail_after = -1; injected = true; throw std::bad_alloc(); }
    if (fail_after > 0) --fail_after;
    if (auto* result = std::malloc(size ? size : 1)) return result;
    throw std::bad_alloc();
}
}
// Fault injection belongs to the test executable, never the product allocator.
void* operator new(std::size_t size) { return Allocate(size); }
void* operator new[](std::size_t size) { return Allocate(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value,std::size_t) noexcept { std::free(value); }
void operator delete[](void* value,std::size_t) noexcept { std::free(value); }
#endif

int RunAllocationTests(const std::filesystem::path& root) {
#ifdef _DEBUG
    (void)root;
    std::wcout << L"NOT RUN: global allocation injection uses Release; Debug STL proxy constructors may terminate inside noexcept.\n";
    return 0;
#else
    using simple_editor::core::BinaryDocument;
    const auto path = root / L"allocation.bin";
    { std::ofstream file(path,std::ios::binary); file << std::string(32,'A'); }
    const auto bytes = [](BinaryDocument& doc) { std::vector<std::uint8_t> value; doc.Read(0,static_cast<std::size_t>(doc.Size()),&value); return value; };
    const auto history_equal = [&](BinaryDocument& a,BinaryDocument& b) {
        for (bool redo : {false,true}) for (;;) {
            std::uint64_t x = UINT64_MAX,y = UINT64_MAX;
            const bool first = redo ? a.Redo(&x) : a.Undo(&x),second = redo ? b.Redo(&y) : b.Undo(&y);
            if (first != second || x != y || bytes(a) != bytes(b)) return false;
            if (!first) break;
        }
        return true;
    };
    int failures = 0;
    for (int operation = 0; operation < 6; ++operation) {
        bool ok = true,completed = false; int stages = 0;
        for (long stage = 0; stage < 128; ++stage) {
            BinaryDocument doc; std::wstring error; std::uint64_t offset = UINT64_MAX;
            ok = ok && doc.Open(path.wstring(),&error) && doc.SetByte(1,'X') && doc.SetByte(5,'Y') && doc.Undo(&offset);
            auto expected = doc.Snapshot(&error,true); if (!expected) { ok = false; break; }
            const auto original = bytes(doc); const std::vector<std::uint8_t> replacement{9,8,7},match{'A'};
            const std::vector<std::uint64_t> offsets{4,8}; bool changed = false,escaped = false;
            injected = false; fail_after = stage;
            try {
                switch (operation) {
                case 0: changed = doc.Replace(3,5,replacement); break;
                case 1: changed = doc.ReplaceMatches(offsets,match,replacement); break;
                case 2: changed = doc.Undo(&offset); break;
                case 3: changed = doc.Redo(&offset); break;
                case 4: changed = doc.Insert(3,replacement); break;
                case 5: changed = doc.Delete(3,5); break;
                }
            } catch (...) { escaped = true; }
            fail_after = -1;
            if (escaped || (injected && changed)) ok = false;
            if (!changed) { ++stages; if (bytes(doc) != original || doc.IsModified() != expected->IsModified() || !history_equal(doc,*expected)) ok = false; }
            if (!injected) { completed = changed; break; }
        }
        ok = ok && completed && stages > 0;
        std::wcout << (ok ? L"PASS: " : L"FAIL: ") << L"allocation rollback operation " << operation << L", " << stages << L" failure stages; bytes and full Undo/Redo retained\n";
        if (!ok) ++failures;
    }
    return failures;
#endif
}

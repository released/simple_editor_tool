#pragma once

#include <Windows.h>

#include <cstdint>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace simple_editor::core {

struct SaveControl {
    const std::atomic_bool* cancelled = nullptr;
    std::function<void(unsigned int)> progress;
};

class BinaryDocument {
public:
    BinaryDocument();
    ~BinaryDocument();

    BinaryDocument(const BinaryDocument&) = delete;
    BinaryDocument& operator=(const BinaryDocument&) = delete;

    bool Open(const std::wstring& path, std::wstring* error);
    void Close();
    bool IsOpen() const;
    bool IsModified() const;
    std::uint64_t Size() const;
    const std::wstring& Path() const;
    std::pair<DWORD,std::uint64_t> SourceIdentity() const { return {source_info_.dwVolumeSerialNumber,(static_cast<std::uint64_t>(source_info_.nFileIndexHigh) << 32) | source_info_.nFileIndexLow}; }
    bool ValidateSource(bool check_path, std::wstring* error = nullptr) const;

    bool Read(std::uint64_t offset, std::size_t length, std::vector<std::uint8_t>* data) const;
    bool SetByte(std::uint64_t offset, std::uint8_t value);
    bool Insert(std::uint64_t offset, const std::vector<std::uint8_t>& data);
    bool Delete(std::uint64_t offset, std::uint64_t length);
    bool Replace(std::uint64_t offset, std::uint64_t remove_length,
                 const std::vector<std::uint8_t>& data);
    bool ReplaceMatches(const std::vector<std::uint64_t>& offsets,
                        const std::vector<std::uint8_t>& before,
                        const std::vector<std::uint8_t>& after);
    std::unique_ptr<BinaryDocument> Snapshot(std::wstring* error, bool include_history = false) const;
    std::unique_ptr<BinaryDocument> SnapshotRange(std::uint64_t offset,std::uint64_t length,std::wstring* error) const;
    void Swap(BinaryDocument& other) noexcept;
    bool Undo(std::uint64_t* offset);
    bool Redo(std::uint64_t* offset);
    bool Save(const std::wstring& destination, std::wstring* error, const SaveControl& control = {});

private:
    enum class Source { Original, Added };

    struct Piece {
        Source source;
        std::uint64_t start;
        std::uint64_t length;
    };

    struct EditAction {
        std::vector<std::uint64_t> offsets;
        std::vector<std::uint64_t> after_offsets;
        std::vector<std::uint8_t> before;
        std::vector<std::uint8_t> after;
    };

    void RebuildIndex();
    std::size_t FindPiece(std::uint64_t offset) const;
    std::size_t SplitAt(std::uint64_t offset);
    void Normalize();
    bool ReplaceWithoutHistory(std::uint64_t offset, std::uint64_t remove_length,
                               const std::vector<std::uint8_t>& data);
    bool ReplaceBatch(const std::vector<std::uint64_t>& offsets, std::uint64_t length,
                      const std::vector<std::uint8_t>& data);
    bool ReadOriginal(std::uint64_t offset, std::size_t length, std::uint8_t* output) const;
    bool ReopenSource(std::wstring* error);

    HANDLE file_;
    BY_HANDLE_FILE_INFORMATION source_info_{};
    std::wstring path_;
    std::uint64_t size_;
    std::vector<std::uint8_t> added_;
    std::vector<Piece> pieces_;
    std::vector<std::uint64_t> starts_;
    std::vector<EditAction> undo_;
    std::vector<EditAction> redo_;
};

} // namespace simple_editor::core


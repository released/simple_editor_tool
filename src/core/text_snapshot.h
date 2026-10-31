#pragma once
#include "binary_document.h"
#include "line_merge.h"
#include "file_operations.h"

namespace simple_editor::core {
// Native controls are captured on the UI thread; conversions use only these owned values.
struct TextInputSnapshot {
    std::unique_ptr<BinaryDocument> document;
    std::string displayed;
    std::wstring original;
    TextCodec codec;
    TextEncoding requested = TextEncoding::Auto;
    std::uint64_t session = 0,revision = 0,generation = 0;
    bool dirty = false,loaded = false;
    bool Materialize(const std::atomic_bool& cancel,std::wstring* error);
};
struct PreparedTextSideCopy : TextSideCopy {
    std::shared_ptr<TextInputSnapshot> inputs[2];
    TextCodec target_codec;
    std::wstring displayed,initial;
    bool prepared = false;
    bool Prepare(const std::atomic_bool& cancel,std::wstring* error);
};
}

#include "hex_view.h"
using namespace simple_editor::core;
bool CHexView::SelectDecoded(CPoint point) {
    if (!document_ || decoding_ == HexDecoding::None || point.y < 0) return false; const int hit_column = (point.x - 8 + horizontal_offset_) / char_width_ - (HexColumn(bytes_per_row_) + 2 + bytes_per_row_ + 3); if (hit_column < 0 || hit_column >= static_cast<int>(bytes_per_row_) + 3) return false; const int column = (std::min)(hit_column,static_cast<int>(bytes_per_row_) - 1);
    const auto at = (top_row_ + point.y / row_height_) * bytes_per_row_ + column; if (at >= document_->Size()) return true; std::vector<DecodedGlyph> glyphs; std::wstring error; if (!DecodeHexWindow(*document_,at,1,decoding_,&glyphs,&error)) { if (report) report(error); return true; } if (!glyphs.empty()) { if (hit_column >= static_cast<int>(bytes_per_row_) && glyphs.front().offset + glyphs.front().length <= at + 1) return false; SelectRange(glyphs.front().offset,glyphs.front().length); decoded_mode_ = true; SetFocus(); if (report) report(L"Read-only decoded character selected by its physical byte range. Click Hex/ASCII to edit."); } return true;
}

#include "main_frame.h"
#include "ui/feature_dialogs.h"
#include "core/search.h"
#include <sstream>
#include <iomanip>
using namespace simple_editor::core;
namespace { std::wstring HexField(std::uint64_t value) { std::wostringstream text; text << L"0x" << std::hex << std::uppercase << value; return text.str(); } }
void CMainFrame::ReviewBitFields() {
    if (IsBusy() || comparison_session_ || ActiveHex() != &hex_ || !document_.IsOpen() || !ApplyTextEdits()) { status_.SetPaneText(0,L"Choose a file Hex page to inspect named bitfields."); return; }
    const auto revision = revision_; const auto generation = text_.Generation();
    if (bitfield_definition_.empty()) bitfield_definition_ = L"enabled " + HexField(hex_.CursorOffset()) + L" 8 le 0 1\r\nmode " + HexField(hex_.CursorOffset()) + L" 8 le 1 3";
    CFeatureInput definition(this,L"Named bitfield definitions",L"One field per line: name byte-offset storage-bits le|be first-bit bit-count. Offsets are hex (d:decimal); widths and bit positions are decimal. Storage: 8/16/32/64. Bit 0 is the least significant bit of the decoded unsigned word, independent of byte order. Up to 256 fields.",bitfield_definition_);
    std::vector<BitField> fields; definition.validate = [&](const auto& value,auto* error) { return ParseBitFields(value,&fields,error); };
    path_dialog_ = true; const auto answer = definition.DoModal(); path_dialog_ = false; if (answer != IDOK) return;
    if (revision != revision_ || generation != text_.Generation()) { status_.SetPaneText(0,L"Document changed; repeat bitfield inspection."); return; } bitfield_definition_ = definition.value;
    std::wstring error; std::vector<std::vector<std::wstring>> rows;
    for (const auto& field : fields) { std::uint64_t value = 0; if (!ReadBitField(document_,field,&value,&error)) { status_.SetPaneText(0,error.c_str()); return; } rows.push_back({field.name,HexField(field.offset),std::to_wstring(field.storage) + (field.little ? L" LE" : L" BE"),std::to_wstring(field.first) + L":" + std::to_wstring(field.count),std::to_wstring(value),HexField(value)}); }
    const bool readonly = MutationBlocked() || hex_.ReadOnly(); CReviewList dialog(this,L"Named bitfields / flags",{L"Name",L"Byte address",L"Storage",L"First:count",L"Unsigned",L"Hex"},std::move(rows)); dialog.accept_label = readonly ? L"Go to field" : L"Edit selected...";
    path_dialog_ = true; const auto selected = dialog.DoModal(); path_dialog_ = false; if (selected != IDOK || dialog.selected.size() != 1 || revision != revision_ || generation != text_.Generation()) return;
    const auto field = fields[dialog.selected[0]]; hex_.SelectRange(field.offset,field.storage / 8); if (readonly) return;
    std::uint64_t old = 0,value = 0; if (!ReadBitField(document_,field,&old,&error)) { status_.SetPaneText(0,error.c_str()); return; }
    CValueDialog edit(this,L"Edit named bitfield",field.name + L": enter unsigned hex or d:decimal. Other bits are preserved. One Undo; Save is separate.",HexField(old));
    edit.validate = [&](const auto& text,auto* message) { if (!ParseAddress(text,&value) || (field.count < 64 && value >= (1ULL << field.count))) { *message = L"The unsigned value must fit the field width."; return false; } return true; };
    path_dialog_ = true; const auto accepted = edit.DoModal(); path_dialog_ = false; if (accepted == IDOK && generation == text_.Generation()) ApplyBitField(field,value,revision);
}
bool CMainFrame::ApplyBitField(const BitField& field,std::uint64_t value,std::uint64_t revision) {
    if (IsBusy() || comparison_session_ || ActiveHex() != &hex_ || MutationBlocked() || hex_.ReadOnly() || revision_ != revision || text_.GetModify()) { status_.SetPaneText(0,L"Bitfield edit rejected: document is read-only, busy or changed."); return false; }
    std::wstring error; if (!WriteBitField(document_,field,value,&error)) { status_.SetPaneText(0,error.c_str()); return false; }
    DocumentChanged(); hex_.SetDocument(&document_); hex_.SelectRange(field.offset,field.storage / 8); status_.SetPaneText(0,L"Named bitfield updated. Other bits retained; one Undo, Save separately."); return true;
}

#include "../src/core/text_review.h"
#include "../src/core/editor_config.h"
#include "../src/core/bit_fields.h"
#include "../src/core/ips_patch.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace simple_editor::core;
int RunEditorReviewTests(const std::filesystem::path& root) {
    int failures = 0; const auto check = [&](bool ok,const wchar_t* label) { std::wcout << (ok ? L"PASS: " : L"FAIL: ") << label << L'\n'; if (!ok) ++failures; };
    const auto write = [](const std::filesystem::path& path,const std::vector<std::uint8_t>& bytes) { std::ofstream file(path,std::ios::binary); file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())); };
    const auto config = [&](const std::filesystem::path& path,const std::string& text) { write(path,{text.begin(),text.end()}); };
    std::atomic_bool cancel{false}; std::wstring error; bool truncated = false; std::vector<MarkdownHeading> headings;
    check(MarkdownOutline(L"# Title\r\n```md\r\n# hidden\r\n```\r\n## Child ##\r\nSubtitle\r\n===\r\n    # indented\r\n<!--\r\n# comment\r\n-->\r\n### End\r\n",cancel,&headings,&truncated) && headings.size() == 4 && headings[1].title == L"Child" && headings[1].line == 5 && headings[2].line == 6 && headings[2].level == 1 && !truncated,L"Markdown outline preserves original line numbers and ignores code/comments");
    check(MarkdownOutline(L"---\ntitle: example\n---\n# Visible\n",cancel,&headings,&truncated) && headings.size() == 1 && headings[0].line == 4,L"Markdown front matter does not create spurious headings");
    check(MarkdownOutline(L"---\n# First\r## Second\r\n#\n",cancel,&headings,&truncated) && headings.size() == 3 && headings[0].line == 2 && headings[2].title == L"(empty heading)",L"Markdown accepts CR/LF/CRLF and empty ATX headings");
    std::wstring many; for (unsigned i = 0; i < 4100; ++i) many += L"# heading\n"; check(MarkdownOutline(many,cancel,&headings,&truncated) && headings.size() == 4096 && truncated,L"Markdown heading results have an explicit bound");
    cancel = true; const auto old_headings = headings.size(); check(!MarkdownOutline(L"# X",cancel,&headings,&truncated) && headings.size() == old_headings,L"Cancelled outline retains the caller's prior results"); cancel = false;
    TextMergeModel model; std::vector<MovedBlock> moves; const std::wstring anchor = L"keep1\nkeep2\nkeep3\nkeep4\nkeep5\n";
    check(BuildTextMerge(L"",L"start\nmoveA\nmoveB\n" + anchor,L"start\n" + anchor + L"moveA\nmoveB\n",false,{},cancel,&model,&error),L"Build real two-way moved-block fixture"); const auto raw = RenderMerge(model);
    check(FindMovedBlocks(model,cancel,&moves,&truncated) && moves.size() == 1 && moves[0].left == 1 && moves[0].right == 6 && moves[0].lines == 2 && RenderMerge(model) == raw,L"Unique exact moved block pairs retain original coordinates and merge output");
    cancel = true; const auto old_moves = moves.size(); check(!FindMovedBlocks(model,cancel,&moves,&truncated) && moves.size() == old_moves,L"Cancelled move detection retains prior results"); cancel = false;
    model.three_way = true; check(!FindMovedBlocks(model,cancel,&moves,&truncated),L"Move discovery rejects three-way models");
    const std::wstring a1 = L"one1\none2\none3\none4\none5\n",a2 = L"two1\ntwo2\ntwo3\ntwo4\ntwo5\n",a3 = L"three1\nthree2\nthree3\nthree4\nthree5\n",repeat = L"duplicate1\nduplicate2\n";
    check(BuildTextMerge(L"",L"start\n" + repeat + a1 + repeat + a2 + a3,L"start\n" + a1 + a2 + repeat + a3 + repeat,false,{},cancel,&model,&error) && FindMovedBlocks(model,cancel,&moves,&truncated) && moves.empty(),L"Repeated ambiguous moved blocks are not paired arbitrarily");
    const auto parent = root / L"ec",child = parent / L"src"; std::filesystem::create_directories(child / L"nested");
    config(root / L".editorconfig","root=true\n[*]\nindent_size=15\n"); config(parent / L".editorconfig","root=true\n[**/*.md]\nindent_style=space\nindent_size=2\n[*.cpp]\nindent_size=3\n");
    config(child / L".editorconfig","[*.md]\nindent_style=tab\ntab_width=8\nindent_size=tab\ncharset=utf-8\n[bad{a,b}]\nindent_size=14\n");
    auto settings = ResolveEditorConfig(child / L"notes.md",{}); check(settings.files == 2 && settings.indentation.tab_width == 8 && settings.indentation.indent_width == 8 && settings.indentation.use_tabs && settings.warnings.size() == 2,L"EditorConfig root, child precedence, tab width and unsupported diagnostics");
    settings = ResolveEditorConfig(parent / L"notes.md",{}); check(settings.files == 1 && settings.indentation.indent_width == 2,L"EditorConfig double-star directory pattern matches zero directories");
    settings = ResolveEditorConfig(child / L"nested" / L"notes.md",{}); check(settings.indentation.tab_width == 8,L"EditorConfig filename patterns match descendants");
    settings = ResolveEditorConfig(child / L"NOTES.MD",{}); check(settings.indentation.tab_width == 4,L"EditorConfig matching remains case-sensitive");
    config(child / L".editorconfig","[*.md]\nindent_size=unset\ntab_width=6\nindent_style=tab\n"); settings = ResolveEditorConfig(child / L"notes.md",{}); check(settings.indentation.tab_width == 6 && settings.indentation.indent_width == 6 && settings.indentation.use_tabs,L"EditorConfig unset removes inherited indentation size");
    config(child / L".editorconfig",std::string(65537,'x')); settings = ResolveEditorConfig(child / L"notes.md",{}); check(!settings.warnings.empty() && settings.indentation.indent_width == 2,L"Oversized EditorConfig is diagnosed and skipped");
    const auto binary = root / L"bits.bin"; const std::vector<std::uint8_t> original = {0xA5,0x5A,0x12,0x34,0x56,0x78,0x9A,0xBC}; write(binary,original); BinaryDocument document; check(document.Open(binary.wstring(),&error),L"Open owned bitfield fixture");
    std::vector<BitField> fields; std::uint64_t value = 0; std::vector<std::uint8_t> bytes;
    check(ParseBitFields(L"flags 0 16 le 4 8\nmode 0 16 be 4 8\nwide 0 64 be 0 64",&fields,&error) && ReadBitField(document,fields[0],&value,&error) && value == 0xAA && ReadBitField(document,fields[1],&value,&error) && value == 0x55,L"Named bitfield decoding defines bit zero after endian conversion");
    check(WriteBitField(document,fields[0],0x12,&error) && document.Read(0,2,&bytes) && bytes == std::vector<std::uint8_t>({0x25,0x51}),L"Bitfield edit preserves every unrelated bit"); std::uint64_t offset = 0;
    check(document.Undo(&offset) && document.Read(0,original.size(),&bytes) && bytes == original,L"Bitfield edit is one native Undo");
    check(!WriteBitField(document,fields[0],256,&error) && !ParseBitFields(L"bad 0 64 le 63 2",&fields,&error) && !ParseBitFields(L"x 0 8 le 0 1\nx 0 8 le 1 1",&fields,&error),L"Bitfield overflow, invalid shifts and duplicate names are rejected");
    check(ParseBitFields(L"wide 0 64 be 0 64",&fields,&error) && WriteBitField(document,fields[0],UINT64_MAX,&error) && ReadBitField(document,fields[0],&value,&error) && value == UINT64_MAX,L"Full 64-bit field avoids undefined shift-by-width"); document.Undo(&offset);
    BitField be; be.storage = 16; be.little = false; be.first = 4; be.count = 8; check(WriteBitField(document,be,0x12,&error) && document.Read(0,2,&bytes) && bytes == std::vector<std::uint8_t>({0xA1,0x2A}),L"Big-endian bitfield edit preserves outside bits"); document.Undo(&offset); be.offset = document.Size(); check(!ReadBitField(document,be,&value,&error),L"A bitfield cannot read beyond EOF");
    const auto base_file = root / L"ips-base.bin"; const std::vector<std::uint8_t> base = {'a','b','c','d','e','f','g','h'}; auto updated = base; updated[1] = 'X'; updated[4] = updated[5] = updated[6] = updated[7] = 'Z'; write(base_file,base); BinaryDocument patch_doc; patch_doc.Open(base_file.wstring(),&error);
    std::vector<std::uint8_t> patch; IpsSummary built,applied;
    check(BuildIpsPatch({'a','b','c'},{'a','b','c'},false,cancel,&patch,&built,&error) && built.base_sha256 == L"BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD" && built.records == 0,L"Patch SHA-256 matches the independent standard abc test vector");
    check(BuildIpsPatch(base,updated,false,cancel,&patch,&built,&error) && patch == std::vector<std::uint8_t>({'P','A','T','C','H',0,0,1,0,1,'X',0,0,4,0,0,0,4,'Z','E','O','F'}),L"IPS export matches an independent literal plus RLE byte vector");
    check(ApplyIpsPatch(patch_doc,patch,built.base_sha256,{},&applied,&error) && patch_doc.Read(0,base.size(),&bytes) && bytes == updated && applied.result_sha256 == built.result_sha256 && applied.preview.size() == 2,L"Hash-verified IPS import decodes every record and retains bounded before/after previews");
    check(patch_doc.Undo(&offset) && patch_doc.Read(0,base.size(),&bytes) && bytes == base,L"Entire IPS import is one Undo");
    check(!ApplyIpsPatch(patch_doc,patch,std::wstring(64,L'0'),{},&applied,&error) && patch_doc.Read(0,base.size(),&bytes) && bytes == base,L"Incorrect base SHA rejects without mutating the draft");
    auto bad = patch; bad.push_back(0); check(!ApplyIpsPatch(patch_doc,bad,built.base_sha256,{},&applied,&error),L"IPS resize extensions/trailing bytes are rejected"); bad = patch; bad.resize(10); check(!ApplyIpsPatch(patch_doc,bad,built.base_sha256,{},&applied,&error),L"Truncated patch records are rejected");
    bad = {'P','A','T','C','H',0,0,1,0,2,'x','y',0,0,2,0,1,'z','E','O','F'}; check(!ApplyIpsPatch(patch_doc,bad,built.base_sha256,{},&applied,&error),L"Overlapping IPS records are rejected before application");
    bad = {'P','A','T','C','H',0,0,8,0,1,'x','E','O','F'}; check(!ApplyIpsPatch(patch_doc,bad,built.base_sha256,{},&applied,&error),L"Out-of-range patch data cannot extend the document");
    bad = {'P','A','T','C','H',0,0,0,0,0,0,0,'x','E','O','F'}; check(!ApplyIpsPatch(patch_doc,bad,built.base_sha256,{},&applied,&error),L"Zero-length RLE records are rejected");
    check(BuildIpsPatch(base,updated,true,cancel,&patch,&built,&error) && patch[0] == 'I' && ApplyIpsPatch(patch_doc,patch,built.base_sha256,{},&applied,&error) && applied.ips32,L"IPS32 header and four-byte offsets round-trip independently of IPS"); patch_doc.Undo(&offset);
    cancel = true; SaveControl stopped; stopped.cancelled = &cancel; check(!ApplyIpsPatch(patch_doc,patch,built.base_sha256,stopped,&applied,&error) && !patch_doc.IsModified(),L"Cancelled IPS import retains the base document"); cancel = false;
    const auto reserved_file = root / L"ips-reserved.bin"; std::vector<std::uint8_t> reserved(0x454F48,0),changed = reserved; changed[0x454F46] = 1; write(reserved_file,reserved); BinaryDocument reserved_doc; reserved_doc.Open(reserved_file.wstring(),&error);
    check(BuildIpsPatch(reserved,changed,false,cancel,&patch,&built,&error) && ApplyIpsPatch(reserved_doc,patch,built.base_sha256,{},&applied,&error) && reserved_doc.Read(0x454F45,3,&bytes) && bytes == std::vector<std::uint8_t>({0,1,0}),L"IPS exporter safely represents data at the reserved EOF offset");
    check(!BuildIpsPatch(base,{},false,cancel,&patch,&built,&error),L"Patch export rejects resizing");
    return failures;
}

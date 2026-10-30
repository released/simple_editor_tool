# Simple Editor Tool User Guide

## Document comparison tools

The **Review** menu adds the commands below. See
[Document editing and comparison](DOCUMENT_EDITING_COMPARISON.md) for bounds and
state rules.

- Select Left, Right and Base encodings independently; press Compare to see the
  actual codec, BOM and EOL summary. Swap sides swaps Left/Right encodings too.
- Select one original line in each two-way source pane, choose **Align selected
  source lines**, then Compare. **Manage alignment points** removes pairs; remove
  all and compare again to restore regular merge editing. At most 128 monotonic
  pairs are supported; anchored output is read only.
- Select folder result rows and choose **Refresh selected folder rows**. Only
  their files/subtrees are rescanned. Other rows are labelled **not rescanned**;
  run full Compare before Sync All, policy sync, baseline capture or copying
  those older rows. Selected refresh accepts 1–128 rows with unchanged scan rules.
- Select Text and choose **Compare selection with clipboard...** for a fresh
  pasted-text tab. Both sources must fit 16 MiB UTF-8 / 200,000 lines. Other tabs,
  drafts and Undo remain. Pasted contents are never written to workspace files.
- **Choose syntax mode...** overrides automatic highlighting for a normal Text
  file and its clone. **Syntax extension associations...** stores mappings such
  as `.conf=JSON` or `.inc=C/C++`. Choose Automatic to use mappings again; manual
  overrides travel with explicitly saved workspaces. Text/encoding/Undo remain.
- Supported code files automatically show **Functions** at the left of **Text**.
  This includes C/C++ headers and definitions, Python, JavaScript/TypeScript,
  Java/C#, PowerShell, Shell, CMake and SQL; Batch shows subroutine labels.
  Click an entry or select it and press Enter to jump. Drag the divider to resize.
  Each document retains its own list/width; switching tabs updates the visible
  outline and retains drafts/Undo. Shared views navigate in the active editor.
  **Review > Function sidebar / refresh** refreshes the nonmodal panel.
  Edits disable stale navigation and trigger a debounced background refresh of
  unsaved text. Plain text and JSON/Markdown/XML/HTML/YAML/diff hide the panel.
  This is heuristic navigation, not a complete parser: complex macros, templates,
  operators, annotations and dynamic constructs may be omitted. Limits are 32 MiB,
  4,096 entries, one million tokens and bounded nesting; partial lists are labelled.
- From a file pair opened by a folder comparison, use **Previous differing
  file**, **Next differing file** and **Return to parent folder comparison**.
  Navigation follows visible parent order without wrapping; existing child drafts
  remain. Changed/closed parents disable stale navigation.
- **Annotate selected difference...** attaches a note and Reviewed state to a
  changed two-way/three-way block. Review state never resolves conflicts. Save or
  Load `.setreview` metadata explicitly; mismatched source fingerprints are
  refused. Changed comparisons retain old notes for explicit save/clear. Limits
  are 1,024 notes and 4,096 UTF-16 code units per note. Unsaved notes prompt on
  normal close and are not part of workspace/crash recovery.
- **Export annotated HTML...** writes full Base/Left/Right contents and notes to
  a separate report. Metadata contains fingerprints/comments, not source buffers.
  Source files and hard links cannot be overwritten by either export.

The large-text pager provides **Find...**, **Find next**, `Ctrl+F` and `F3`.
Search is streamed, exact and case sensitive for UTF-8/UTF-16 LE/BE, with 1–4,096
UTF-16 code units per query. Ctrl+F starts at the current page; F3 continues to
EOF without wrapping. Esc cancels and retains the page. Pager regex, replacement
and legacy-code-page searching are not supported. Ordinary wheel scrolls;
Ctrl+wheel zooms.

[繁體中文使用指南](USER_GUIDE.md)

Simple Editor Tool is a native Windows x64 application for text and hexadecimal
editing, binary/source comparison, three-way merging and folder synchronization.
MFC and the C/C++ runtime are statically linked. Python, .NET, device drivers and
separately installed editor plugins are not required.

## 1. Starting the application and choosing a language

Keep `SimpleEditorTool.exe`, `lang_en.ini` and `lang_tw.ini` in the same directory.
Run the EXE directly. The first-run default is Traditional Chinese. Choose
**語系 / Language → English** to use English, or choose **繁體中文** to switch back.
A saved preference takes priority over the default and is reused at the next start.

Switching updates existing interface text while retaining documents, drafts,
carets, selections and Undo. It does not change file encoding. Document contents,
paths, search input/history, programming syntax, format identifiers and original
third-party license text are not translated. Diagnostics missing from a catalog
remain in English. Native Windows file/font dialogs and some system buttons
continue to follow Windows language settings.

Language files are UTF-8 INIs. Source catalogs are under `src/res`; delivery
copies belong beside the EXE. `[Language]` contains the version, language ID and
display name. `[Strings]` maps stable keys to display text. Values support `\n`,
`\r`, `\t` and `\\`. Preserve keys, format parameters and message placeholders
such as `{0}`. Restart after editing catalogs to reload them. Missing, malformed
or incompatible catalogs leave a usable built-in English interface. Restore both
files and restart to restore Chinese. The EXE alone can use the English fallback.

## 2. Opening files and managing tabs

Choose **Open files...**, press `Ctrl+O`, or drag one or more files into the main
window. Each file has its own workspace tab. Opening another file retains existing
tabs; opening the same path activates its existing editor. Decodable text within
the editing bound opens on **Text**. Binary and oversized inputs use **Hex**.
The application starts maximized.

**New text file** and **New from Unicode clipboard** create untitled documents;
the first Save uses Save As. Close a selected tab with **Close tab**, `Ctrl+W`,
middle-click or its context menu. Unsaved changes prompt before closing. Tab
menus also close other/left/right tabs or move a tab. Drag tabs to reorder them.
Recent-file and reopen-closed-file actions are available from File.

Only one normal application window runs in a Windows session. A second launch
forwards its file paths to the existing workspace without discarding drafts:

```powershell
.\SimpleEditorTool.exe "first.bin" "notes.md"
```

## 3. Editing text

Supported source encodings include UTF-8, UTF-16 LE/BE, Big5/CP950 and the Windows
system code page. Select decoding from Encoding. Display/edit adapters preserve
the source newline style. **Convert output encoding / EOL to a copy...** exports
an explicit encoding, BOM and CRLF/LF/CR policy to another file. It does not
silently overwrite the source; lossy conversion is refused.

Text offers line numbers, syntax colors, Markdown heading/emphasis/link/code
colors, bookmarks, folding, whitespace/EOL display, wrapping, multiple selections
and column selection. The default dark content background is `#3F3F3F`.
Preferences can select light, dark or system-contrast colors.

View commands configure the font and Tab width, sort lines, remove duplicate or
blank lines, trim trailing spaces and enable shared Text views. Shared views use
one draft, Undo history and save state, with independent carets and scrolling.
Word completion uses document/language words; it is not a semantic code analyzer.

EditorConfig supports the documented indentation subset and manual overrides;
loading settings does not reformat the document. `Ctrl+G` goes to a line, and
**Markdown outline...** navigates headings. Review provides three Text mark
colors. Marks do not change content; editing clears outdated marks.

## 4. Finding and replacing

`Ctrl+F` and `Ctrl+H` open the same Find/Replace window. Choose **Current document**
or **All open text documents (including unsaved)**. The latter includes untitled
drafts, skips binary/oversized inputs and excludes comparison panes. Search a
comparison pane with Current document.

Search supports literals, escapes, single-line Scintilla POSIX regex, case/whole
word options, both directions, selection scope, wrapping, Count and Find All.
Use the dedicated multiline literal controls to paste multiple lines directly.
Escapes include `\n`, `\r`, `\t`, `\xHH`, `\uHHHH` and `\\`.
Regex capture replacements use `\1` through `\9`; this is not an unrestricted
multiline regex engine.

Replace All creates one Undo action per affected document and does not save.
All-open replacement can preview and select participating files. Read-only,
busy, closed or changed participants prevent stale plans from being applied.
`F3`/`Shift+F3` repeat the search. Double-click a result to navigate.

`Ctrl+Shift+F` searches open drafts or a disk folder. Results include paths,
lines/addresses and previews. Reviewed folder replacement provides preflight
version checks, backups and journals without rewriting open unsaved drafts.
See [Multi-file search](MULTI_FILE_SEARCH.md).

## 5. Editing and navigating hexadecimal data

Hex displays physical offsets, bytes and ASCII/decoded characters. Choose
8/16/32 bytes per row, grouping and a display base address. Display bases do not
change physical file offsets.

Drag or use Shift+arrows to select. `Ctrl+C` copies Hex text and exact bytes;
`Ctrl+X` cuts. `Ctrl+V` follows insert/overwrite mode. The Hex context menu also
offers explicit paste modes and UTF-8 text paste. Click the ASCII column or press
Tab to change input columns; Ins toggles insert/overwrite. Insert, delete and
modify bytes with Undo/Redo.

`Ctrl+G` accepts hexadecimal addresses such as `0x100`, or `d:256` for decimal.
EOF is valid. Byte search accepts exact pairs, `AA ?F ??` wildcard patterns and
typed values such as `u32le:1234`, `i16be:-2` and `f32le:1.5`.
Numeric replacements use Hex bytes.

Review can position a current selection between Text and Hex, including the
actual BOM and original line endings. Partial multibyte characters, surrogate
pairs, BOM-only byte ranges and halves of display-only newline expansion are
rejected. Named colored physical ranges help record addresses. Modification
review compares the draft against its saved disk version.

## 6. Comparing files or directly pasted text

Choose **Compare files...**; **Text/source (2-way)** is the default. Enter the
left/right paths, use Browse, or select open and recently opened files from the
dropdowns, then press Compare. Up to 20 recent files remain available after their
document tabs close and are saved through the existing portable preferences.
Entering Compare from an open document fills an empty left path with that document.

After an explicit Compare completes successfully, an information dialog appears
when no differences are found under the current settings. Ignoring whitespace,
case or EOL may treat different original bytes as equivalent. Display filters do
not affect this decision; failed/cancelled comparisons do not show the notice.
Automatic recomparison after copying a block does not repeat the popup.

Drop a single file onto the left or right side to select that input, including
the read-only source panes in comparison results. Press Compare to run afterward.
Drops preserve existing editor tabs and unsaved drafts; inputs cannot change
during a running comparison. Multiple-file drops still open each file in a retained
tab. Swap sides exchanges the inputs; compare again afterward. Select either Hex
mode explicitly from the mode list when comparing binary data.

| Mode | Use |
| --- | --- |
| Hex - equal address | Compare bytes at the same physical offsets. |
| Hex - insertion aware | Resume alignment after nearby insertions/deletions. |
| Text/source (2-way) | Align lines and produce independent editable merge output. |
| Pasted text (2-way) | Paste text A/B directly without saving input files first. |
| Text merge (3-way) | Merge left/right versions against a common Base. |
| Folders / sync | Compare folders and preview synchronization. |
| CSV / TSV / JSON | Read-only structured comparison. |

Open file editors contribute their current unsaved snapshots; other inputs are
read from disk. Source panes are read-only and separate from merge output.
Comparison alone does not modify input files. Binary rows show both offsets and
values; double-click or use `F7`/`Shift+F7` to navigate. Drag the splitters to
resize panes. Region settings permit different starts and ignored physical spans.

Pasted inputs support typing, selection and Undo/Redo, including an empty side.
Editing invalidates old results. Pasted drafts remain in the current session,
are excluded from workspace INIs and prompt before closing. Save result writes
only the selected output; pasted comparisons default to UTF-8 output.

## 7. Merging text and reviewing differences

Two-way Text/source comparison places complete sources side by side with original
left/right line numbers in the middle. Merged output starts collapsed; check
**Merged output** to expand it without discarding its draft.
Select a changed source line or middle row and use **Block -> / <- Block** to
copy that difference to the opposite side. The middle list supports multiple
selection. **All -> / <- All** copies the entire text. **Previous difference /
Next difference**, or `Shift+F7` / `F7`, cycles through difference blocks.

Copies update or create the target's unsaved document tab, retain other tabs,
and immediately compare the current drafts again. The destination retains its
encoding and newline convention; enable Ignore EOL when only newline differences
remain. With output collapsed, Undo/Redo targets the last copied side; shortcuts
in a source pane target that side. **Save left / Save right**, or Save in its
document tab, explicitly writes that file. Save any separate unsaved merged output
before undoing a source draft. With output collapsed, `Ctrl+S` saves
the focused source side, or the last copied side when a button has focus.
Check that tab's status for the final
save result. Stale sources, read-only/busy destinations and unrepresentable text
reject copying while preserving drafts. Manual anchors, pasted comparisons and
three-way mode retain their respective merge workflows.

Ignore spaces, case or EOL differences; filter changes/conflicts; select blocks
and take left, right or both. Three-way mode can also take Base. Independent
changes merge automatically; overlapping changes display conflict markers.
You may edit the output and explicitly mark the manual result resolved after
removing markers. Further edits require acknowledgement again. Unresolved
conflicts block normal saving unless Allow unresolved markers is enabled.

Full sources provide character differences, original line numbers, synchronized
scrolling and display-only alignment gaps. Aligned sources disable wrapping and
folding to retain row correspondence. Text and pasted inputs retain independent
view settings. Three-way mode can show/hide full Base. Character detail is
bounded to the first 8,192 UTF-16 units per line and the existing computation
budget; complete longer lines remain readable/searchable. Missing highlights
beyond the bound do not imply equality.

Other review options include similar-line alignment, exact unique moved blocks,
unchanged-context collapse and read-only manual line anchors. Literal ignore
rules and timestamp masking do not rewrite sources. Named comparison presets
support explicit Save/Apply/Delete; applying never automatically compares or
synchronizes. See [Comparison and inspection tools](WORKFLOW_EXPANSION.md).

## 8. External changes and safe saving

`Ctrl+S` saves the active file; Save As selects another destination. Saving runs
in the background and can be cancelled before replacement. That document's
mutations pause during Save, while scrolling, zoom and other tabs remain usable.
A committed atomic replacement is not undone by a late cancellation request.

An external source change retains the Text draft and offers Keep draft, Compare
with disk and Reload. Keep dismisses that disk version only. Compare opens
separate read-only snapshots. Reload uses the normal unsaved-change decision.
Source identity, size or write-time mismatches reject operations depending on
old bytes. A failed/cancelled operation is not a successful save; retain the draft,
check the source, then reload or use a safe Save As.

## 9. Comparing and synchronizing folders

Select left/right folders, optionally recurse, and enter semicolon-separated
include/exclude globs. Compare content, size, size/time or SHA-256. Equal sizes
or timestamps do not guarantee equal bytes. Sort/filter results or use the tree;
double-click file pairs to open separate comparisons. Independent folder text
rules supplement raw byte results without redefining synchronization contents.

Synchronization requires an operation preview. Update retains target-only files;
Mirror moves them to backup. Newer and baseline bidirectional policies are also
available. Both-modified entries become conflicts, and baseline deletion
propagation is opt-in. Individual entries/subtrees may be skipped. Unsaved or
busy affected editors block operations.

Backups live under the target's `.SimpleEditorTool.backups/<operation>/data/`
and are excluded from scans. Cancellation/errors may retain already committed
operations; there is no automatic whole-folder rollback. Keep the reported journal
and backups and review Retry/Recover plans. Reparse points, overlapping roots and
file/directory type conflicts are refused. See [Folder comparison and recovery](FOLDER_COMPARE_AND_RECOVERY.md).

## 10. CSV, TSV and JSON

CSV/TSV parse quoting and multiline fields, with optional headers, column mapping,
keys and exact decimal tolerance. Duplicate keys are ambiguous. Empty mapping
settings preserve positional comparison.

JSON ignores object-key order and formatting while retaining types and exact
decimal numbers. Use `ignore /metadata/generatedAt` to ignore a subtree or
`key /devices /id` to pair array entries. RFC 6901 pointers use `~1` for `/` and
`~0` for `~`; quote pointers containing spaces. IDs must be unique strings or
numbers; numeric `1` differs from string `"1"`. Missing/duplicate IDs reject the
comparison. Other arrays remain positional. Ambiguous ignores or nested key rules
below a reordered keyed array are unsupported. These modes do not automatically
save, synchronize or modify raw inputs.

## 11. Binary analysis and conversion

Analyze a selected range or the whole file for CRC32, SHA-256, byte distribution
and Shannon entropy. In the interactive plots, click a frequency bar for its
count or an entropy bin to select its physical range in Hex. Changed sources
reject stale navigation.

The numeric inspector supports endian-aware integers/floats; writable fixed-width
types use one Undo. ULEB128/SLEB128 are endian-independent, read-only 64-bit
interpretations consuming at most 10 bytes with overflow/EOF checks. Named
bitfields modify selected bits while retaining the rest.

For Base64/Hex/UTF-8 conversion, choose input/output formats, then Preview. Hex
accepts pairs and whitespace, without addresses or `0x`. Base64 uses the standard
alphabet and correct `=` padding. UTF-8 must be valid and NUL-free. The preview
shows at most 64 KiB; Copy full output includes the entire formatted result.
From an editable file Hex selection, Apply decoded bytes explicitly replaces the
captured range with the decoded payload as one Undo action; it does not save.

Other tools extract bytes/export C arrays, fill/invert/swap/reverse ranges,
split/join files, extract strings, calculate named/custom CRCs and apply
fixed-width unsigned integer operations. Confirm physical range, endian and
operation settings before applying.

## 12. Binary templates and firmware files

Each `.settpl` line uses `path type offset [decimal-count [stride]]`, for example:

```text
header.version u16le 4
header.flags u16be 6
entries[].value u32le 20 4 8
```

Offsets/strides are hexadecimal unless prefixed `d:`. Types include integers,
floats, bytes and fixed-length strings. The field tree navigates valid physical
ranges. Templates are read-only descriptions with no scripts, expressions or
automatic code loading, and are incompatible with ImHex Pattern Language.
See [Binary templates](BINARY_TEMPLATES.md).

Intel HEX/Motorola S-record tools validate record checksums, overlap and
termination, and convert to/from BIN. Sparse BIN conversion requires a gap-fill
value and address base. BIN does not preserve entry-point or other record
metadata. Outputs must be separate from inputs. These are file tools: they do
not program devices, install drivers or establish real-board validation.
See [Firmware formats](FIRMWARE_FORMATS.md).

## 13. Reports and patches

Export binary CSV/TSV/HTML and text HTML/summary/unified patches. Binary reports
stream complete differences beyond the display-row limit. Outputs cannot replace
inputs or their hard links. Text patch application verifies source hashes when
present, exact positions and context, then previews and applies as one Undo.
Save remains separate.

IPS/IPS32 currently support equal-length data. Import requires the expected Base
SHA-256. Unsupported addresses, mismatches and stale previews are refused;
inputs are not silently overwritten. See [Text reports and patches](TEXT_REPORTS_AND_PATCHES.md)
and [Text review](EDITOR_REVIEW.md).

## 14. Workspaces, recovery and logs

Workspace INIs save paths, views, comparison rules, zoom and physical annotations,
not unsaved file/merge contents. Loading retains existing tabs/drafts and waits
for explicit comparison. Pasted comparisons are excluded. Preferences can enable
startup restoration and separate draft snapshots. Recovery opens untitled copies
without overwriting originals. Recovery payloads may be sensitive; choose their
retention/cleanup explicitly and do not publish personal settings/drafts/backups
as examples.

Log following uses a separate read-only view with pause/resume and follow-end;
it does not modify the source. Language choice is stored in the preferences of
`simple_editor_tool.ini` beside the EXE. Use a writable location for persistent
settings.

## 15. Large files and bounds

Hex reads visible ranges rather than loading the entire original binary. Beyond
the editable text bound, use the read-only pager: up to 256 KiB source bytes per
page, with Next/Previous/First/Go to byte. Line numbers restart per page and are
not whole-file line numbers. Big5/system code pages require trusted sequential
boundaries.

| Operation | Current bound |
| --- | --- |
| Editable Text | 32 MiB. |
| Text/structured comparison | 16 MiB per side; text has a 200,000-line bound. |
| All-open search snapshots | 128 MiB combined. |
| Search/Unicode locations | First 100,000 retained per operation; complete counts may still be reported. |
| Binary difference display | First 1,000,000 rows; reports stream complete differences. |
| Byte transformation/pattern replacement | 64 MiB per operation, plus command-specific match limits. |
| Base64/Hex/UTF-8 conversion | 32 MiB input, 16 MiB decoded bytes, 64 KiB preview. |
| Comparison presets | 32 profiles; at most 1 MiB INI. |
| Language catalogs | At most 1 MiB / 4,096 entries per file; invalid files fall back to English. |
| Intel HEX/S-record | 2,000,000 records / 64 MiB decoded data; BIN span at most 256 MiB. |

Unicode notices cover a defined invisible/bidi subset, not all Unicode security
issues. Alignment, templates, EditorConfig, JSON rules and patch formats have
documented subset/precision limits; consult their linked guides.

## 16. Shortcuts and command-line use

| Shortcut | Action |
| --- | --- |
| Ctrl+O / Ctrl+N / Ctrl+S | Open / New / Save. |
| Ctrl+F / Ctrl+H | Find / Replace. |
| F3 / Shift+F3 | Find Next / Previous. |
| Ctrl+Shift+F | Open-file/folder search. |
| Ctrl+G | Go to Line in Text; Go to Address in Hex. |
| F7 / Shift+F7 | Next / previous difference. |
| Ctrl+Z / Ctrl+Y | Undo / Redo. |
| Ctrl+R / Ctrl+W | Reload / Close tab. |
| Ctrl+Tab / Ctrl+Shift+Tab | Next / previous workspace tab. |
| Wheel | Scroll; only Ctrl+wheel zooms. |
| Ctrl+0 | Reset the active view to 100%. |
| Ctrl+Shift+P / Alt+Left / Alt+Right | Command search / previous location / next location. |

Hex, Text and Compare retain independent 50%–300% zoom levels shown in the status
bar. Preferences customize supported shortcuts with conflict checks for reserved
editing/navigation bindings.

Headless comparison does not open the GUI or load/update personal GUI settings:

```powershell
.\SimpleEditorTool.exe --compare --left "old.bin" --right "new.bin" --mode binary --report "diff.csv"
.\SimpleEditorTool.exe --compare --left "old.cpp" --right "new.cpp" --mode text --report "diff.patch" --format patch
.\SimpleEditorTool.exe --help
```

Exit codes: `0` equal, `1` differences, `2` option/input/output failure, `3`
cancelled before commit. Code 1 is not a failed report. Existing reports require
`--overwrite`; quote paths with spaces. See [Command-line comparison](COMMAND_LINE.md)
for complete modes and examples.

## 17. Explorer integration and troubleshooting

Tools > Windows Explorer adds/removes the current-user file context-menu command
without administrator permission. Windows 11 may put it under Show more options.
After moving the EXE, register its new location. Remove its owned entry before
deleting the tool if desired. Default file associations are unchanged.

- Text will not decode: check the encoding; use Hex for binary or paging for large text.
- A replacement/edit is refused: check read-only state, Save/sync locks and source versions; do not force stale results.
- Merge output will not save: resolve conflicts or explicitly permit unresolved markers.
- UI remains English: restore both valid language files beside the EXE and restart.
- Language choice is not retained: check settings-directory write access and reported preference-save errors.
- Sync stopped partway: retain its journal/backups and review Retry/Recover before deleting recovery data.

WinMerge, Beyond Compare and Notepad++ workflows are references; complete feature
parity is not claimed.

# Simple Editor Tool

A native Windows desktop tool for editing hexadecimal bytes and text, comparing
binary and source files, merging changes, and synchronizing folders.

## Features

- Hex editor with address, byte and ASCII views, insert/delete, Undo/Redo and
  navigation for large files.
- Hex range selection, Cut/Copy/Paste, ASCII/UTF-8 input and insert/overwrite modes.
- Text editing with UTF-8, UTF-16 LE/BE, Big5/CP950 and Windows system encodings.
- Line numbers, wrap, whitespace display, bookmarks, folding, column/multiple
  selections and syntax highlighting for common source/configuration languages.
- Binary comparison with differing addresses and values, side-by-side Hex panes
  and synchronized scrolling.
- Line-aligned text/source comparison with basic syntax colors, whitespace rules,
  two-way merging and three-way conflict resolution.
- Folder comparison by content, size, time or SHA-256; directed/newer/baseline
  synchronization with exclusions, operation journals and guarded recovery.
- Read-only CSV/TSV field and normalized JSON comparison, binary field templates
  with a structure tree, and paged viewing of large text files.
- Headless comparison/report commands with explicit result exit codes.
- Independent file and comparison tabs, drag-and-drop, content zoom and saved
  workspace settings.

## Getting started

The application runs on Windows x64. The executable includes the MFC and C/C++
runtime; no additional runtime or device-driver installation is required.

1. Run [build/SimpleEditorTool.exe](build/SimpleEditorTool.exe).
2. Choose **Open files...** or drag files into the window.
3. Use **Hex** to inspect/edit bytes, **Text** to edit decoded text, or
   **Compare files...** to select comparison inputs.

Each opened file keeps its own tab, edits and Undo history. Opening or dropping
another file adds a tab; reopening the same path activates its existing tab.
**Close tab** closes only the selected session and asks about unsaved edits.

**New text file** and **New from Unicode clipboard** create untitled documents;
the first Save uses Save As. Recent/reopen commands, middle-click close, tab
dragging and close-other/left/right actions are available from File or tab menus.

Several files can also be opened from the command line:

~~~powershell
.\SimpleEditorTool.exe "first.bin" "second.bin"
~~~

Launching the executable again forwards file paths to the existing window.
The window starts maximized.

## Windows Explorer right-click menu

Choose **Tools > Windows Explorer > Add file context menu** to register
**以 Simple Editor Tool 開啟** for the current Windows user. Right-click a file
to open it in the tool. Windows 11 may place the command under **Show more
options**. No administrator permission, shell DLL or driver is required.

Selected files open as independent tabs in the existing window. Windows applies
its own multi-selection limit to this static menu; use Open files or drag/drop
for larger selections. File associations and default double-click actions stay
unchanged. The menu uses the EXE's current location; after moving the tool, add
the menu again from its new location.

Choose **Tools > Windows Explorer > Remove file context menu** before deleting
the tool or when you no longer want the command. This removes only the tool's
own menu entry for the current user.

## Compare files

Choose **Compare files...**, enter the left and right paths, or use the
**Browse** buttons and open-file dropdowns. Select a mode and press **Compare**.
Use **Swap sides** to exchange inputs, then compare again.

| Mode | Purpose |
| --- | --- |
| Hex - equal address | Compare bytes at the same offsets in both files. |
| Hex - insertion aware | Compare bytes and resume alignment after nearby insertions/deletions. |
| Text/source (2-way) | Align text lines and merge selected changes. |
| Text merge (3-way) | Merge two versions against a common Base file. |
| Folders / sync | Compare saved folder contents and plan synchronization. |
| CSV / TSV | Compare quoted fields with optional keys, column mappings and decimal tolerances. |
| JSON | Compare typed values while ignoring object-key order and formatting. |

Binary results list added, removed and changed bytes with both addresses.
Use **Compare > Export binary differences...** to write a CSV, TSV or HTML
report. Reports stream all differences from the compared snapshots, including
results beyond the display limit. The comparison inputs cannot be report outputs.
Double-click a result to navigate to it. **F7** and **Shift+F7** move between
differences. Drag the pane dividers to adjust the comparison layout.
Binary rows appear in batches during comparison. Cancellation clears partial
results. Structured modes are read-only; see [Structured data](docs/STRUCTURED_DATA.md).

For file comparison, an already-open editor contributes its unsaved Text/Hex
snapshot. Other files are read from disk. Comparison inputs are read-only
snapshots; opening a comparison preserves the original editor tabs and drafts.

Hex layout commands choose 8/16/32 bytes per row, grouping and a display base.
Comparison regions can use different start offsets and ignored spans; reports
use those same rules. Selected difference ranges can be merged to a separate
output after preview, including insertion/deletion offset changes.

## Compare and merge text

Choose **Compare > Text/source comparison...** or **Three-way text merge...**.
These commands are also available in **File** when no tabs are open.

Text mode aligns source lines and can ignore spaces, case or line-ending
differences. **Changes only** and **Conflicts only** filter the visible rows.
Select changed rows and choose **Take left**, **Take right** or **Take both**.
Three-way mode also offers **Take base**.

For a three-way merge, select the common **Base** file along with the left and
right versions. Independent changes merge automatically; overlapping changes
appear as conflicts. Resolve them using block choices, or edit the merged output
and remove the conflict-marker lines, then choose **Mark manual result
resolved**. Further manual edits require that acknowledgement again.

**Save result...** writes the output file you choose. Unresolved conflicts block
saving unless **Allow unresolved markers** is explicitly enabled. Comparison
alone does not modify either input file.

Manual edits, block choices and output replacements share multi-step Undo/Redo.
Taking another block preserves independent manual edits. Overlapping changes
produce visible conflict markers and require resolution before a normal save.
Text display and saves preserve source encoding and line-ending styles.

Changed characters are highlighted within source rows, including CJK and emoji.
The right-edge overview navigates through differences. Unique moved lines show
their original line numbers. **Align similar lines (2-way)** adjusts source-row
alignment while retaining the merged output and its Undo history.

Export HTML, summary or unified patch reports from **Compare**. Patch application
checks the source and hunk context, previews the output and applies as one Undo
action; Save remains separate. See [Text reports and patches](docs/TEXT_REPORTS_AND_PATCHES.md).

Two-way text comparisons also support literal ignore rules, timestamp masking
and explicit manual line anchors. Anchored views are read-only. Text tools offer
line sorting/cleanup, shared dual views and document/language word completion.
Hex tools include named colored ranges, an editable live numeric inspector and
named/custom streaming CRC. Saved logs can be followed in a separate read-only
window. Folder workflows provide content-based rename/move pairing and reviewed
multi-file replacement with version checks, backups and recovery journals.
See [Editor and comparison workflows](docs/EDITOR_AND_COMPARE_WORKFLOWS.md) for
commands, examples and limits.

Folder comparisons can additionally evaluate text rules, and offer an expandable
result tree. Hex supports a read-only Unicode/Big5 decoding column, modification
review, binary string extraction and fixed-width integer operations. Text change
markers, comparison-context collapse, command search and Back/Forward navigation
help inspect and navigate larger workspaces. See [Native editing and comparison
workflows](docs/NATIVE_WORKFLOWS.md).

## Compare and synchronize folders

Choose **Folder comparison / sync...** from **Compare** (or **File** with no
open tabs). Browse to both folders, choose whether to include subfolders, and
press **Compare**.

The list shows equal, different, left-only and right-only entries, sizes and
unreadable/type-conflicting entries. Include/exclude filters accept
semicolon-separated globs, for example `*.cpp;*.h` or `*.tmp;build`.
Folder comparisons use saved disk contents.

Choose content, size, size/time or SHA-256 criteria. Results include modification
times and the newer side; column headings sort and Filter selects result states.
Size-based criteria can consider different byte contents equal.

Select rows and use a **Selected** arrow, or use **Sync all** in the desired
direction. Selecting a folder includes its scanned descendants. Review the
operation preview, then choose **Apply listed operations**.

- **Update** copies source changes and retains target-only files.
- **Mirror** also moves target-only file contents into recovery backups.
- Overwritten files are backed up before replacement.

Newer-only and baseline policies also support reviewed bidirectional operations.
Both-modified files become conflicts; baseline deletion propagation is opt-in.
The preview can exclude individual entries or subtrees.

Backups retain relative paths under the target folder's
`.SimpleEditorTool.backups/<operation>/data/` directory, which is excluded from
comparison scans. Double-click a file pair to open a separate comparison;
a file present on only one side opens in an editor.

Synchronization checks for files changed since comparison. Affected open
editors with unsaved changes or active operations block synchronization; clean
affected editors are temporarily read-only.

Cancellation or an error leaves already committed operations in place. Review
the reported journal and backup location. **Retry journal** plans pending work;
**Recover journal** previews restoration after checking current outputs and
backup fingerprints. There is no
automatic whole-folder rollback. Reparse points, overlapping roots and
file/folder type conflicts are rejected for synchronization.
See [Folder comparison and recovery](docs/FOLDER_COMPARE_AND_RECOVERY.md).

## Find, navigation and zoom

The **View** menu provides line navigation, word wrap, whitespace/EOL display,
bookmarks, folding, column selection, additional matching selections, Tab width,
font and a dark content theme. Text Undo/Redo supports multiple edit steps.
Language rules follow the file extension. Third-party notices are available in
**Help > About / native components**; source notices are in
[Native source dependencies](docs/THIRD_PARTY.md).

The **Tools** menu provides little/big-endian integer/float interpretation,
CRC32/SHA-256 and byte distribution/entropy for the selected range or whole file.
It also offers range extraction, C array export, fill/invert/reverse/word swapping,
and file split/join. Mutating range tools require a selection and use one Undo
action; a single range transformation is limited to 64 MiB. Extraction and analysis
stream the input. Split uses new numbered files and retains completed parts if
cancelled. Join previews the selected file order before choosing its output.

Intel HEX and Motorola S-record commands validate record checksums and addresses
and convert to/from BIN. See [Firmware formats](docs/FIRMWARE_FORMATS.md) for
address bases, gap filling and format limits.

**Open binary field template...** displays named fields in a structure tree and
navigates to their byte ranges. See [Binary templates](docs/BINARY_TEMPLATES.md).
**Large text pages...** provides a bounded read-only viewer beyond the editable
text limit; see [Large files](docs/LARGE_FILES.md).

For automated comparisons and reports, use `--compare --left ... --right ...`.
See [Command-line comparison](docs/COMMAND_LINE.md) for modes, output protection
and exit codes.

In Hex mode, drag to select bytes or use Shift+arrows. **Ctrl+C** copies Hex text
and exact byte data; **Ctrl+X** cuts a selection. **Ctrl+V** follows the current
insert/overwrite mode. The Hex context menu also offers explicit paste modes
and UTF-8 text paste. Click the ASCII column or press Tab to switch input columns;
Ins toggles insert/overwrite. The status line shows the selection offset/length.

Text and merged-output searches support case/whole-word options, both directions,
selection scopes, Count, Find All and a recent-pattern dropdown. Find All opens a
result list; double-click a result to navigate. Text-result offsets refer to the
editor's UTF-8 buffer. Searches and replacement planning run in the background;
changed buffers reject old results. Replace All is one Undo action.

**Ctrl+Shift+F** searches all open file drafts or a selected disk folder, with
path/line/address results and guarded navigation. Results arrive in bounded
batches; the retained list is limited to 100,000 rows while final counts include
all matches. See [Multi-file search](docs/MULTI_FILE_SEARCH.md).

Escaped text accepts `\n`, `\r`, `\t`, `\xHH`, `\uHHHH` and `\\`. Regex uses
[Scintilla's POSIX syntax](https://www.scintilla.org/ScintillaDoc.html#Searching)
within individual lines; capture replacements use `\1` through `\9`.
Hex search accepts exact pairs (`00 FF 41`), nibble wildcards (`AA ?F ??`) and
typed values (`u32le:1234`, `i16be:-2`, `f32le:1.5`). Numeric replacements use Hex
bytes. Pattern replacement supports a selected range/output up to 64 MiB and
1,000,000 matches; an empty replacement deletes matches.

**Encoding** selects input decoding. **Convert output encoding / EOL to a copy...**
exports the current text draft with an explicit encoding, BOM and preserve/CRLF/
LF/CR line endings. The original file and its draft remain available; lossy
conversion is rejected. Save file/merge edits separately.

Go To Address accepts hexadecimal offsets such as `0x100`, or decimal
offsets prefixed with `d:`, such as `d:256`. EOF is a valid position.

| Shortcut | Action |
| --- | --- |
| Ctrl+F / F3 / Shift+F3 | Find / Find Next / Find Previous |
| Ctrl+H | Replace in the file editor |
| Ctrl+G | Go To Line in Text; Go To Address in Hex |
| F7 / Shift+F7 | Next / previous difference |
| Ctrl+S | Save the active file or merge result |
| Ctrl+Z / Ctrl+Y | Undo / Redo |
| Ctrl+R | Reload from disk |
| Wheel up / down | Scroll content |
| Ctrl+wheel up / down | Enlarge / reduce content |
| Shift+wheel | Scroll without changing zoom |
| Ctrl+0 | Reset the active view to 100% |
| Ctrl+W | Close the selected workspace tab |
| Ctrl+Tab / Ctrl+Shift+Tab | Next / previous workspace tab |

Hex, Text and Compare retain independent zoom levels from 50% to 300%.
The status bar displays the active percentage. Zoom preserves edits, selection
and Undo history.

## Save and restore a workspace

Use **File > Save workspace...** to save paths, view/compare settings and Hex
range annotations to an INI file.
**File > Open workspace...** restores editor and comparison tabs, rules,
encodings, zoom and Hex caret/scroll positions. Existing tabs and drafts are preserved;
restored comparisons wait for an explicit **Compare**.

Workspace files contain paths, settings and annotations. Save file and merge edits
separately before closing the application.

**Preferences** separately offers opt-in startup restore and bounded draft
recovery snapshots outside the delivery folder. Recovery validates copies and
opens separate untitled tabs; it never silently overwrites an original file.
Font/theme/view defaults, configurable shortcuts and recent files use the portable
INI beside the EXE. See [Preferences and recovery](docs/PREFERENCES_AND_RECOVERY.md).

## Saving and external file changes

Save and Save As write in the background. Scrolling, zooming and switching tabs
remain available while the active document is temporarily read-only.
**Esc** cancels before final file replacement; cancellation after replacement
does not undo the committed save.

The editor checks file identity, size and last-write time. If an external change
is detected, reload the current disk version or use Save As to recover edits
from an intact opened file to another path. Failed saves retain edits and report
any recovery-file locations. These checks do not exclusively lock files against
other programs.

## Limits

| Area | Limit |
| --- | --- |
| Hex clipboard | 64 MiB per operation. |
| Text editing | 32 MiB per file; invalid encodings or embedded NUL data use Hex mode. |
| Text merge | 16 MiB and 200,000 lines per input; source rows preview up to 8,192 characters per line. |
| Binary difference list | First 1,000,000 differences, with a truncation indicator. |
| Structured comparison | 16 MiB / 200,000 nodes or cells per input; 20,000 retained differences. |
| Large text viewer | 256 KiB source pages, read-only, page-relative line numbers. |
| Folder comparison | 200,000 entries. |
| Workspace INI | 1 MiB and 200 sessions. |

Large repetitive text regions may appear as one coarse changed block. The
preview limit does not truncate saved merge results. Comparison-row syntax colors
use basic lexical rules. Similar-line and character alignment use bounded work;
large changed regions can retain a coarse alignment.
Windows manages application DPI scaling; native per-monitor DPI transitions are
not provided.

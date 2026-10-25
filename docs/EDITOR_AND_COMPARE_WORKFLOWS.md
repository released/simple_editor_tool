# Editor and comparison workflows

## Hex ranges and live values

Select bytes on a file's Hex page, then choose **Tools > Name selected Hex
range...**. Enter a name, optional comment and color. **Hex range bookmarks...**
lists ranges, removes selected entries and navigates to a current range. Offsets
are physical bytes, independent of the display base. Each file supports 256
ranges. Save a workspace to retain them. Edits or a changed source invalidate
older ranges; recreate them explicitly. Unsaved ranges reopen as stale because
workspace files do not store the underlying draft bytes.

**Tools > Live numeric inspector...** follows the active Hex caret. Choose little
or big endian, select a numeric type, edit its value and press **Write value**.
Unsigned/signed integers from 8 to 64 bits and 32/64-bit floating point use exact
field widths. Each accepted write is one Undo action. A moved caret, changed
document, read-only source or insufficient remaining bytes blocks the write;
select the field again after a change. Comparison panes permit inspection only.

## Text productivity

**View > Line operations** sorts ascending/descending, removes duplicates or
blank lines, and trims trailing spaces/tabs. A selection expands to whole lines;
without a selection the command covers the document. Sorting is ordinal and
case sensitive. Deduplication keeps the first identical line. Blank-line removal
recognizes empty/space/tab lines. The operation preserves final-newline presence
and uses one native Undo action. Input is bounded to 32 MiB and 200,000 lines.
Saving continues to use the file's encoding and newline adapter.

**View > Toggle shared Text views** displays two views of one file buffer with
independent carets and scrolling. Edits, Undo and Save belong to that single file;
saving locks both views. This is distinct from opening independent workspace
file tabs. The shared-view setting is retained by a workspace.

Type a prefix and choose **View > Complete from document / language**. Candidates
come from document identifiers and the selected language's built-in keywords.
Keyword lists have dedicated Python/JSON sets and a common C-family fallback
for the other modes; they are not complete dictionaries for every syntax lexer.
Prefix matching is case sensitive. Indexing runs in the background with bounded
input/candidates; a changed buffer or moved caret discards the old suggestion.
This is local word completion and does not resolve symbols, imports or types.

## Two-way text rules and manual alignment

On a two-way Text/source comparison choose **Compare > Text comparison rules...**.
Enter one setting per line, then run **Compare** again:

```text
ignore-blank=yes
mask-timestamps=yes
ignore-prefix=DEBUG:
ignore-contains=volatile counter
mask=host-name
```

Prefix/contains/mask operands are literal and case sensitive. Timestamp masking
recognizes the 19-character shapes `YYYY-MM-DD HH:MM:SS` and
`YYYY-MM-DDTHH:MM:SS`; it does not validate calendar dates or mask arbitrary time
formats. Up to 16 literal rules of at most 256 characters are accepted. Blank and
selected-line rules affect comparison significance. Sources and merged output
retain their original text; ignored blocks are explicitly counted. These rules
apply to two-way mode. Three-way conflict handling retains its existing rules.
HTML/summary reports include active rules and ignored-block counts. Unified
patches describe raw source changes so they remain applicable.

**Compare > Manual line alignment...** accepts one `left-line,right-line` pair
per line, starting at 1. Both sequences must strictly increase. Run **Compare**
again to adopt up to 128 anchors. The anchored comparison is read-only and hides
merge output/actions. Existing edited output is protected by the usual discard
decision. Clear the anchors and compare again to restore merging and report
export. Anchors never rewrite a source file or automatically choose a merge.

## Reviewed replacement across saved files

Choose **Edit > Preview replacements in folder...**, or the File menu command
when no file is open. Select the folder, search/replacement text, syntax,
encoding, include/exclude filters and recursion. Literal, escaped and native
Scintilla single-line POSIX regex searches use the same rules as editor search.

Preview scans saved disk text without writing it. Errors/skipped files are
reported. Select changed files in the review list; **Details...** displays before
and after previews (first 65,536 characters each). **Use selected** applies only
the selected files. The original encoding/BOM and newline adapter are retained.
Dirty/busy editor targets are refused. File identity, metadata and SHA-256 are
checked again before each commit; an outdated preview cannot overwrite a
changed file. Reparse paths are excluded.

Each file commits atomically and keeps its prior contents under the target
folder's `.SimpleEditorTool.backups`, together with a synchronization journal.
The folder is not a single atomic transaction. Close/Esc requests cancellation
at a safe file boundary; already committed files remain changed. The result
lists backup/journal locations and any retained private staging directory.
Use the folder synchronization journal recovery workflow to review a recovery
plan. Inspect retained staging before removing it manually.
See [Folder comparison and recovery](FOLDER_COMPARE_AND_RECOVERY.md).

Limits: 200,000 scanned entries, 32 MiB per input, 64 MiB per encoded output,
2,000 changed files and 128 MiB of retained outputs/previews. Narrow the folder
or filters if a limit is reached. This workflow does not replace unsaved drafts.

## Renamed/moved content

After a folder comparison choose **Compare > Find renamed / moved content...**.
Unmatched files are paired by size and SHA-256. Unique equal-content pairs can
open in a separate binary comparison. Duplicate-content groups remain explicitly
ambiguous; no arbitrary pairing is selected. Source versions are rechecked.
This read-only analysis never renames files or alters a synchronization plan.

## Follow a saved log

On a saved file choose **Tools > Follow saved log file...**. A separate read-only
window polls disk and shows a decoded window covering at most the final 256 KiB
of source bytes. Choose UTF-8/UTF-16 or BOM/UTF-8 detection, pause/resume polling,
and enable/disable following the end. Truncation and path-identity rotation are
reported. Invalid/racing reads retain the previous view. Closing joins its
worker. The original editor and unsaved draft are never reloaded by this window.
Legacy code-page random-offset following is not offered; use the normal editor.
Line numbers are relative to the displayed window.

## Named and custom CRC

**Tools > Named / custom CRC...** calculates a streaming CRC for the Hex selection
or whole file. Presets: `CRC-16/ARC`, `CRC-16/MODBUS`, `CRC-32/ISO-HDLC`,
`CRC-32/ISCSI` and `CRC-64/ECMA-182`. Example custom parameters:

```text
name=CRC-17/CAN-FD
width=17
polynomial=1685B
init=0
xorout=0
refin=no
refout=no
```

Widths are decimal 8–64; polynomial/init/xorout are hexadecimal and must fit the
width. The polynomial omits its leading term. Set `preset=CRC-32/ISO-HDLC` to use
a preset and optionally override fields. Results identify the actual parameters,
physical range and CRC. Profiles persist with their file's workspace entry.
The existing fixed CRC32/SHA-256/entropy command remains available. CRC detects
accidental corruption; it does not establish authenticity.

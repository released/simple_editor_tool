# Native editing and comparison workflows

## Folder text comparison rules

In a folder comparison choose **Compare > Folder text comparison rules...**.
Settings apply after the next **Compare**. For example:

```text
enabled=yes
include=*.cpp;*.h;*.txt;*.ini;*.log
encoding=auto
ignore-eol=yes
mask-timestamps=yes
```

`encoding` accepts `auto`, `utf8`, `utf16le`, `utf16be`, `big5`, or `system`.
`ignore-space`, `ignore-case`, and `ignore-eol` accept `yes`/`no`. The existing
literal rules (`ignore-blank`, `ignore-prefix`, `ignore-contains`, `mask`, and
`mask-timestamps`) are also supported. Set `enabled=no` or clear the settings to
disable this analysis. Include masks apply to filenames and use semicolons.

The **Text rules** column distinguishes **Byte-identical**, **Equal under text
rules**, **Important text differences**, and **Text analysis error**. An empty
cell means the file was outside this analysis, such as an excluded extension or
an unmatched path. Inputs are limited to 16 MiB each. Invalid text and limits
produce an explicit error; they are never treated as equality.
The folder Filter dropdown can isolate important text differences, equality
under text rules, or text analysis errors independently of the raw state filters.

The ordinary State column still uses the selected content/size/time/SHA-256
criterion. Text rules supply additional analysis and never silently change a
synchronization operation. Review the actual paths and copies in the operation
preview. These are line/literal rules rather than language-aware syntax analysis.

## Decoded characters beside Hex bytes

On a Hex page choose **Tools > Hex decoded character column...**. Enter:

| Value | Decoder |
| --- | --- |
| 0 | Off; retain the normal ASCII column |
| 1 | UTF-8 |
| 2 | UTF-16 little endian |
| 3 | UTF-16 big endian |
| 4 | Big5 / CP950 |

The additional column is read-only. Clicking a character selects its complete
physical byte range, including multi-byte characters crossing a Hex row. Click
Hex/ASCII or press Tab before typing byte edits. Display bases do not change
physical offsets. Invalid/control characters appear as dots; a continuation of a
character on another row is marked with a middle dot.

Big5 decoding requires a known boundary. The viewer checks up to 4096 preceding
bytes; if the phase is still ambiguous it displays dots until a proven boundary
instead of inventing a character. Visible decoding is bounded to 64 KiB plus
boundary lookbehind. Each of the three Hex panes retains its own decoder in a
saved workspace. Viewing never converts or saves the source bytes.

## Review modifications before saving

The Text margin shows change history, with separate markers for unsaved and saved
changes. Native Undo/Redo and shared Text views retain their history. Existing
line bookmarks use a separate margin marker. **View > Previous unsaved modification** and **Next unsaved modification**
navigate the changed Text lines.

For Hex choose **Tools > Review unsaved byte modifications...**. A background
comparison against saved disk contents produces ranges with saved/draft offsets,
byte counts and added/removed/changed state. Select a result to navigate; the
current-generation ranges also highlight bytes in Hex. Previous/Next unsaved
modification navigates those adopted ranges. Refresh the review after another
edit, reload or save; older overlays expire rather than guessing new offsets.
Deleted ranges can have zero draft bytes and navigate to their alignment anchor.
The list supports up to 100,000 coalesced ranges. Analysis does not save changes.

## Search commands

Choose **View > Search commands...** or press **Ctrl+Shift+P**. When no file is
open the command is available from File. Type part of a command or menu path,
select an available result and press **Run command**. The list includes shortcut
labels. Unavailable commands retain the normal busy, page, read-only and selection
restrictions. Commands use the same native handlers as their menus.

## Extract strings from a binary

On an editor tab choose **Tools > Extract binary strings...**. Enter
`minimum,type-mask`, for example `4,7`. Minimum length is 2–4096 Unicode characters.
Type masks can be added: ASCII=1, UTF-16 LE=2, UTF-16 BE=4. With a Hex selection,
only that range is scanned; otherwise the complete captured draft is scanned.

The background scan crosses read-chunk boundaries and supports both UTF-16 byte
phases and valid surrogate pairs. Results contain physical offset, original byte
length, encoding and a preview. Short strings/random data can produce false
positives; increase the minimum or narrow the range/types. The first 100,000 rows
are retained, each with at most 128 UTF-16 preview units; the total count remains
separate. Select a result to navigate while the source version is current.

**Tools > Export extracted string previews...** writes the retained results as
UTF-8 CSV to an explicit output. It exports previews and complete byte ranges,
not unlimited full string contents. CSV construction is bounded to 64 MiB of
UTF-16 text. Outputs cannot overwrite the input path/identity or a dirty/busy open
editor. Another edit invalidates the older result; extract again before export.

## Selected integer operations

Select bytes in a writable file Hex page, then choose **Tools > Selected integer
operations...**. Enter `operation width endian operand`, for example:

```text
xor 16 le FF
add 32 be d:1
left 8 le d:2
```

Operations: `xor`, `and`, `or`, `add`, `subtract`, `left`, `right`. Widths are
unsigned 8/16/32/64 bits; endian is `le`/`be`. Operands are hexadecimal by default,
or decimal with `d:`. Arithmetic wraps modulo 2^width. Shift counts must be less
than the width; bitwise/arithmetic operands must fit it. The selection must be a
multiple of the word size and at most 64 MiB.

The operation prepares a private snapshot in the background, rejects cancelled
or stale results and applies one Undo transaction. Save remains separate.

## Collapse text comparison context

In a two-way or three-way text comparison choose **Compare > Unchanged context
lines...**. Enter 1–50 to retain that many rows around significant differences;
enter 0 to disable collapse. Conflicts-only chooses conflict locations as the
context centers. Double-click a hidden-row placeholder to expand it. This view
is separate from the existing Changes only filter and unified-patch context.

Placeholders cannot become merge choices. Source line indices, merge output,
Undo and report contents remain unchanged. Context count persists in a workspace;
individual expanded regions are temporary for the adopted comparison.

## Return to earlier locations

**View > Back / Forward**, or **Alt+Left / Alt+Right**, navigates file-editor
Text/Hex locations across workspace tabs, including the active shared Text view.
The history holds up to 256 positions and remains in memory. Document changes,
reloads, saves, closed tabs or unavailable shared views can invalidate older
positions; stale entries are skipped. Navigation never restores an older draft or
reopens a closed tab automatically. Comparison-source rows are outside this history.

## Folder result tree

After a folder comparison choose **Compare > Toggle folder result tree**. A
directory row shows an expand/collapse marker; double-click it to toggle children.
Use **Expand all result folders** or **Collapse all result folders** as needed.
Ancestor directories remain visible when their filtered descendants match. The
details column summarizes changed files and errors in the scanned subtree.

Tree mode preserves original scan identities and the complete filtered candidate
set. Collapsing rows does not narrow **Sync all**. Selecting a directory still
uses the existing descendant-expansion and operation-preview rules. The original
left/right path fields and Browse buttons are retained. Workspace files retain
tree mode; expansion state is temporary for the current scan.

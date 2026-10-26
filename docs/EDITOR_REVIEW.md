# Text review and binary patches

The **Review** menu contains document review commands. Commands act on the
current file or comparison tab; editing and importing do not save files.

## Full comparison sources and moved blocks

File Text comparisons show read-only left/right source panes above the aligned
rows and merged output. Select or search complete source text, including long
lines. Selecting an aligned row selects its original source lines. Scrolling a
source follows the corresponding original line on the other side; unmatched
stretches use the nearest mapped line. Small windows retain the rows and output
when there is insufficient room for full sources.

**Review > Moved blocks in text comparison...** lists unique exact blocks moved
between the two sides. **Go to pair** selects both original ranges. Detection
uses complete unmatched runs of at least two lines, including their line endings;
repeated ambiguous blocks are skipped. Up to 4096 pairs are listed. This is
informational and does not change merge decisions or report text. It applies to
two-way comparisons, including pasted text.

## Draft versus disk

For a saved Text file, choose **Compare current draft with disk...** to open a
separate comparison of the current draft and a fresh disk version. The original
tab, unsaved text and Undo history remain intact, including when the source was
changed externally. Both comparison inputs are read-only. The reviewed result
must be saved to a separate file, including when another path is a hard link to
the source. Each input is limited to 16 MiB.

## Markdown outline and text marks

**Markdown outline...** presents ATX (`#`) and Setext headings as a tree. Choose a
heading to jump to its original line. Fenced/indented code and standalone HTML
comment blocks are excluded. The outline is a bounded navigation aid, rather
than a Markdown renderer: up to 4096 headings, title previews of 256 UTF-16 units
and 32 MiB of editor input. It does not rewrite text or clear Undo.

**Text marks > Mark all** highlights a literal case-sensitive string in yellow,
cyan or pink. Colors can coexist. **Clear text marks** removes them; an actual
text edit also clears old marks. Marking does not change text or Undo. Each
search highlights up to 100,000 occurrences and reports truncation.

## Multiline Find/Replace and all-open preview

Open Find/Replace with **Ctrl+F** or **Ctrl+H**, then select **Multiline literal**
to paste text containing line breaks directly into the Find/Replace boxes. This
uses literal matching and the existing current/all-open scopes. Regex remains a
single-line mode. Each input is limited to 32,768 UTF-16 units; multiline entries
are not added to the single-line history. Clear line breaks before turning the
multiline option off.

With **All open documents**, **Preview All...** plans replacements before
modifying anything. Select files in the preview, use **Details** for before/after
excerpts, then **Apply selected**. Only selected files change, with one Undo per
file. All chosen drafts are checked again before any application. Cancel leaves
every draft intact. Details show the first 32 KiB of each before/after buffer and
label truncation; the replacement plan uses the complete bounded snapshots.
No files are implicitly saved.

## EditorConfig indentation

Local saved files read a supported subset of `.editorconfig` automatically.
**Reload EditorConfig indentation** discards the document's manual indentation
override and reads the rules again. Manual **View > Tab settings** takes
precedence for that document session until reload or reopening. Documents retain independent effective settings;
shared Text views use the same document indentation.
Remembering a document's view does not promote its EditorConfig or manual
indentation into other documents' global defaults. Enter after an opening brace
uses the effective indentation size, separately from the visual tab width.

Supported properties are `indent_style` (`tab`/`space`), `indent_size` (1–16 or
`tab`), `tab_width` (1–16), and `unset`. Parent files apply before child files;
later matching sections override earlier ones. `root = true` stops the search.
Patterns support case-sensitive `*`, `**` and `?`, with `/` as separator. Patterns
without `/` match the file name in descendants; `**/` also matches zero directories.

This is a subset, not full EditorConfig conformance. Unsupported patterns and
properties produce status diagnostics; charset/EOL rules never rewrite the file.
Lookup is local-path-only, up to 32 parents, 64 KiB per UTF-8 configuration and
128 sections per file, with bounded glob work. Nonregular configurations are
skipped. Rule changes take effect on reopen or explicit reload.

## Named bitfields and flags

On a file's **Hex** page, choose **Named bitfields / flags...**. Define one field
per line:

```text
enabled 0 8 le 0 1
mode 0 8 le 1 3
status 10 16 be 4 8
```

The columns are `name byte-offset storage-bits le|be first-bit bit-count`.
Byte offsets are physical file offsets in hexadecimal (`d:` accepts decimal),
independent of a Hex display base; widths and bit positions
are decimal. Storage is 8, 16, 32 or 64 bits. **Bit 0 is the least significant
bit of the decoded unsigned word**, independent of byte order. Up to 256 unique
names and 32,768 UTF-16 units of definitions are accepted. Lines beginning with
`#` are comments. Definitions stay in the session.

The list displays addresses, storage, bit ranges and unsigned decimal/hex values.
Select one field and choose **Edit selected...** to enter hex or `d:decimal`.
Values must fit the field. Other bits are preserved and one Undo restores the
edit. Read-only files allow inspection/navigation only. This declarative format
does not execute scripts or accept other editors' template languages.

## IPS / IPS32 patches

**Binary patches > Export IPS patch... / Export IPS32 patch...** compares the
current Hex draft with its saved disk baseline and writes a separate patch.
The result dialog supplies the base and result SHA-256 hashes; share the base
hash alongside the patch because IPS itself stores no base identity.

**Preview / import IPS or IPS32...** asks for the expected base SHA-256 from the
patch author. It must match the current draft. The importer validates all records
on a private snapshot, then shows hashes and before/after previews for the first
200 records (16 bytes per record). Accepting adopts the complete validated draft
as one Undo action. Cancel, stale state or a hash mismatch preserves the draft.
Save is always explicit.

Only equal-length patches are supported: IPS documents up to 16 MiB, IPS32 up
to 64 MiB, patch input up to 65 MiB, 100,000 records and 64 MiB decoded record
bytes. Literal and RLE records are accepted. Overlaps, zero-length RLE, truncated
records, out-of-range writes, resize extensions and trailing bytes are rejected.
No ROM header adjustment or automatic truncation/extension is performed.

# Comparison and inspection tools

These commands are available from the **Review** menu of an open document or
comparison. They use the portable native application and Windows system APIs.

## Aligned full sources and Base

File Text/source comparisons show complete read-only sources above the difference
list and separate editable merge output. Character differences use red/green
indicators; changed lines also have a background mark. Display-only blank gaps
align missing lines, including insertions before the first line. Source gutters,
Find, Go To Line, selection and copying use original text and line numbers.
Source panes keep wrapping and folding disabled so corresponding display rows
remain aligned. Zoom changes only presentation. Pasted inputs and merge output
retain their independent wrap/edit/Undo behavior.

Character detail is bounded to the first 8,192 UTF-16 units per line and the
existing diff computation budget. Longer lines remain complete and searchable;
the absence of a character mark beyond the bound does not imply equality.
Existing input limits remain 16 MiB and 200,000 source lines per comparison side.
Three-way comparison includes a read-only Base source. **Show / hide full Base
source** changes its visibility without changing merge choices or output.

## External disk changes and selection mapping

When the saved source changes externally, the file Text page retains the current
draft and shows a persistent notification. **Keep draft** dismisses that disk
version; another detected disk version shows it again. **Compare with disk**
opens independent read-only draft/disk snapshots. **Reload...** uses the existing
unsaved-change decision. Neither detection nor comparison automatically saves or
reloads the draft. Missing/unreadable files still retain the draft. Hex reads and
saves continue to reject an invalid original source.

**Show Text selection in Hex** and **Show Hex selection in Text** map the current
file's selection between decoded UTF-16 text coordinates and physical byte
offsets. Mapping includes the actual BOM, original mixed line endings, UTF-8,
UTF-16 LE/BE, Big5 and supported system encoding. Whole characters are required:
BOM-only bytes, partial multibyte/surrogate selections, unmappable characters and
half of a display-only CRLF expansion are rejected. An unchanged document/version
is required when the background result arrives. Mapping does not save files.

## Named comparison settings

Presets apply to file Text, three-way, folders, CSV/TSV and JSON comparison.
Binary range settings and pasted drafts are outside this preset menu.

**Comparison presets** provides explicit Save, Apply and Delete actions for up to
32 named settings, stored in `comparison_presets.ini` beside the executable
(maximum 1 MiB). Names are case sensitive and contain 1–80 characters. Saving an
existing name asks before replacing it.

Presets contain the comparison mode, encoding, comparison/view flags, include and
exclude filters, folder view settings and Text/table/JSON/folder-text rules.
Source selectors, drafts, output paths, physical ranges, per-file manual anchors,
sync baseline paths and sync actions are excluded. Rule/filter literals are saved
as entered. Apply requires the same comparison mode, retains existing output and
waits for an explicit Compare. It never starts synchronization.

## Unicode notices

**Inspect unusual Unicode...** lists codepoint names, original UTF-8 byte offsets,
line numbers and scalar columns. A selected row navigates to that character.
**Toggle live Unicode notices** enables debounced background notices in the current
normal file Text buffer. Native indicators and `U+xxxx` display representations
make otherwise invisible characters visible without replacing them or adding Undo
entries. **Allowed Unicode code points...** accepts hexadecimal scalars such as
`U+00A0 U+200B`. These settings belong to the current session.

The defined notice set is U+00A0, U+061C, U+200B, U+200E/F, U+2028/29,
U+202A–202E, U+2060, U+2066–2069 and embedded U+FEFF. A leading U+FEFF is skipped.
This is a targeted notice feature, not a Unicode security/confusable validator.
Ordinary CJK, ZWJ and ZWNJ are not flagged. Snapshots are limited to 32 MiB UTF-8;
up to 100,000 navigable locations are retained with a complete count. Changes
invalidate old indicators/results; text and native Undo remain intact.

## Binary plots and conversion

**Interactive binary statistics...** analyzes the selected Hex range, or the
entire source when no bytes are selected. It displays CRC32, SHA-256, Shannon
entropy, 256 byte-frequency bars and up to 512 consecutive entropy bins. Click a
frequency bar for its count. Click an entropy bin, then **Select block in Hex**
to select its physical source range. Version/source checks reject stale results.
The last bin may be shorter. Read-only comparison Hex sources are supported.

**Base64 / Hex / UTF-8 conversion...** accepts pasted input with explicit input
and output formats. **Preview** runs a bounded background conversion. The preview
shows at most 64 KiB; **Copy full output** copies the complete formatted result.
Changing input or either format invalidates the old result. Closing cancels it.

Hex accepts byte pairs and whitespace, without addresses or `0x` prefixes.
Base64 accepts the standard alphabet and whitespace, with complete groups and
canonical `=` padding. UTF-8 must be valid and NUL-free. Binary zero bytes are
supported through Hex/Base64. Input is limited to 32 MiB, decoded bytes to 16 MiB.
The Hex formatted output may therefore exceed the preview bound.

Open the dialog from an editable normal file Hex selection (at most 16 MiB) to
enable **Apply decoded bytes**. It replaces that captured range with the decoded
payload, regardless of the chosen formatted output, as one Undo action. The
source/revision and write guards are rechecked. Copy-only use is available from
other views. Applying never implicitly saves to disk.

## JSON pointer and array-key rules

In JSON comparison, choose **JSON ignored paths / array keys...** and enter one
rule per line, then Compare again:

~~~text
ignore /metadata/generatedAt
key /devices /id
~~~

Pointers use RFC 6901: `/a~1b/~0x` addresses property `a/b`, then `~x`.
Double-quote pointers containing spaces; an empty quoted pointer denotes the
root. `ignore` skips exactly that subtree. `key` pairs array entries using a
relative property/index pointer within each entry. IDs must be unique strings or
numbers; numeric `1` and string `"1"` are distinct. Numbers retain existing exact
decimal normalization. Missing, duplicate or unsupported identifiers reject the
comparison instead of guessing pairings. Arrays without a rule stay positional.

Rules are limited to 128 ignores and 32 keyed arrays, 512 UTF-16 units / 64 tokens
per pointer, and 65,536 units overall. Ignoring beneath a reordered keyed array
is ambiguous and rejected; ignore the container itself or use paths outside it.
Nested key rules below a reordered keyed array are also rejected; configure
independent array containers. A configured pointer resolving to a non-array is an
error, while a missing container is an ordinary addition/removal.
Raw inputs remain unchanged. Rules are saved in workspaces and named presets;
they affect read-only JSON comparison, not folder synchronization or merge output.

## LEB128 inspection

Both the fixed and live numeric inspectors include unsigned **ULEB128** and signed
**SLEB128**, decoded from the Hex caret with their actual consumed byte length.
The interpretation is independent of the endian selector and is read-only.
Up to 10 bytes are accepted for a 64-bit value; truncated input, unterminated
continuations and overflow are reported. Finite non-minimal encodings are
accepted. Existing fixed-width numeric editing is unchanged.

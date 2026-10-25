# CSV, TSV and JSON comparison

Choose **Compare files...**, browse the left/right paths, then select **CSV**,
**TSV** or **JSON** and press **Compare**. These modes show field paths and
left/right values. F7 / Shift+F7 navigate differences. They are read-only;
editing and merging remain available through the Text/source modes.

CSV and TSV compare logical rows and columns. Quoted separators, doubled quotes,
multiline quoted fields and trailing empty cells are supported. Row-ending style
outside field values does not change a cell. Rows/columns are compared by their
position by default. **Compare > Table keys and columns...** enables explicit
key-based row alignment and column mappings. Enter one mapping per line using
1-based column numbers, then compare again:

```text
header=yes
1,2,key
2,1,0.01
3,3,text
```

This skips the first row, pairs rows by left column 1/right column 2, compares
left column 2/right column 1 with an absolute decimal tolerance of 0.01, and
compares column 3 literally. Only mapped columns participate. Multiple `key`
columns form a composite key. Duplicate/missing keys remain explicit ambiguous
results. Tolerances use exact decimal arithmetic, preserving large integers;
invalid numeric values are reported. Each side's column can be mapped once,
up to 64 mappings. Empty settings restore positional comparison. Without a key,
mapped columns retain positional rows. These settings persist in a workspace.
There is no automatic sorting or file rewriting.
CSV handling follows the quoting conventions described by
[RFC 4180](https://www.rfc-editor.org/rfc/rfc4180); TSV uses the same rules with
a tab separator. Ragged rows retain their actual cells.

JSON object member order and insignificant formatting are ignored. Arrays keep
their order; strings, numbers, booleans and null retain distinct types. Exact
decimal normalization considers `1`, `1.00` and `1e0` equal without rounding
large integers through floating point. String escapes are decoded. Duplicate
object names, invalid numbers/escapes and unpaired surrogates are rejected.
The syntax reference is [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259).
This is semantic value comparison, not JSON canonicalization or JSON Patch.

Each input is limited to 16 MiB and 200,000 JSON nodes or table cells. JSON depth
is limited to 128. Results retain the first 20,000 differences and report the
full difference count; value/path previews are bounded. Missing containers may
appear as one added/removed field. Source snapshots and open-editor drafts use
the same guards as other comparison modes.

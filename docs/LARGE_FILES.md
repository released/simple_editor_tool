# Large files

Hex mode reads visible rows from a file snapshot and keeps byte edits in a piece
table. It does not load the entire original binary just to display it. Binary
comparison delivers ordered batches to the result list while its worker runs;
cancellation removes the incomplete comparison. The list retains at most
1,000,000 byte differences. Exporting a binary report streams the complete
comparison independently of that display limit.

For large text choose **Tools > Large text pages...**. The read-only native
viewer loads up to 256 KiB of source bytes per page. Use **Next**, **Previous**,
**First page**, or **Go to byte...**. The status shows physical start/end byte
offsets (end exclusive), total size and zoom %. Line numbers restart on each
page; the viewer does not calculate whole-file line numbers. A line may continue
onto the next page. Previous retains the latest 4096 visited page boundaries.

UTF-8 and UTF-16 pages keep complete code points/surrogate pairs. Random offsets
advance to a complete character; UTF-16 offsets first align to a code unit.
`Auto` detects a UTF-16 BOM, otherwise chooses UTF-8 in this viewer. Choose the
source decoding before opening it if necessary. Big5/system legacy code pages
support sequential pages; arbitrary nonzero offsets are refused because their
byte boundaries can be ambiguous. NUL or malformed text produces an error and
retains the previous page. The same file remains available in Hex mode.

The wheel scrolls the page; Ctrl+wheel zoom changes only the viewer font. Esc
cancels a pending read and Close waits for its reader. Pages preserve the
underlying draft snapshot and do not
offer Save or edit history. Source identity/size/time checks reject externally
changed files. No complete-file line index or unbounded decoded cache is built.

Editable Text remains limited to 32 MiB. Text merge and structured comparisons
have separate 16 MiB input limits; they do not automatically switch to paging.
Find in files displays bounded result batches and retains at most 100,000 rows
while reporting total matches. These bounds are deliberate; file type, line
length, encoding, edit history and storage speed still affect memory and latency.

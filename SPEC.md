# Simple Editor Tool Specification

## Current Scope

- Native workflow extensions N01-N09: independent folder text-rule evaluation,
  bounded read-only Hex decoding, Text change history/Hex modification review,
  command palette, streaming unknown-string extraction and preview CSV export,
  unsigned selected integer operations, expandable comparison context,
  generation-guarded file navigation and optional folder-result trees.
- Their operational contracts and bounds are defined in docs/NATIVE_WORKFLOWS.md.

- Named colored physical Hex ranges and live endian-aware scalar inspection/editing.
- CSV/TSV key/column mappings and exact decimal tolerance; two-way literal ignore
  rules and timestamp masking preserve raw sources/output.
- Explicit monotonic manual two-way anchors produce a read-only view; clear and
  recompare before merging/reporting. Shared Text views share one buffer/Undo/save.
- Bounded background line cleanup and local document/language word completion.
- Reviewed folder multi-file replacement with metadata/hash guards, owner locks,
  per-file atomic writes, backups/journal and safe cancellation. No draft rewrite.
- Read-only rename/move content pairing, independent bounded UTF-8/UTF-16 log
  following, and streaming named/custom CRC profiles (8–64 bits).
- Operational settings, bounds and first-version limitations for these workflows
  are specified in docs/EDITOR_AND_COMPARE_WORKFLOWS.md and docs/STRUCTURED_DATA.md.

- Open arbitrary files without decoding their bytes.
- Optional current-user Explorer file context menu, managed from Tools with an
  executable icon and quoted absolute EXE/input paths. Keep normal associations,
  other applications' menu entries and existing file tabs. No admin/driver/DLL
  installation; moved portable tools require re-registering their new path.
- Keep each opened file in an independent workspace tab until the user closes it. Multi-file drop, Open and command-line forwarding open every file; reopening an existing path activates its existing editor without replacing drafts.
- Retain each session's native Text buffer/Undo, document edits, caret, encoding, view state, zoom and background job when another session is selected or opened.
- Provide independent comparison tabs with editable left/right path dropdowns, native Explorer Browse buttons, Swap sides, mode selection and Compare/Recompare. Selecting paths must not replace an editor session.
- Compare current unsaved Text/Hex snapshots from open editor tabs; comparison snapshots are read-only. Path changes invalidate old results and source views until Compare is run again.
- Display address, hexadecimal bytes, and ASCII in a virtualized view.
- Overwrite individual bytes and insert or delete bytes in the Hex view.
- Undo and redo byte edits.
- Save safely or save to another path.
- Open ordinary text and source files with selectable common encodings.
- Compare two files in equal-address or insertion-aware mode in a background worker.
- List changed, added, and removed byte addresses and navigate to each difference.
- Synchronize the two comparison hex views.
- Allow an active comparison to be cancelled without mixing stale results into a later run.
- Forward all command-line files from a secondary/renamed launch to the existing instance using bounded multi-path messages and a startup-ready handshake.
- Expand the address column to fit the document's largest offset.
- Find literal text using the current encoding, or search for exact hex byte sequences.
- Find Next wraps around EOF. Text searches respect UTF-16 and DBCS character boundaries.
- Replace the selected match or all non-overlapping matches; Replace All is a single Undo/Redo action.
- Jump to a 64-bit hexadecimal address (optional `0x`) or a decimal address prefixed `d:`; permit EOF.
- Previous/next difference selects both addresses and wraps at list boundaries.
- Allow dragging both the left/right splitter and the pane/list splitter; retain ratios during resize and tab changes.
- Select Auto, UTF-8, UTF-16 LE/BE, Big5/CP950, or the Windows system code page.
- Ordinary wheel up/down scrolls content. Ctrl+wheel enlarges/reduces content by 10% per notch, bounded to 50%-300%, accumulating high-resolution partial wheel deltas.
- Hex/Text/Compare retain separate session zoom levels. Compare's two panes and difference list share one level. A dedicated status pane displays the active percentage without overwriting operation progress.
- Shift+wheel retains content scrolling; Ctrl+0 resets the active tab to 100%. Zoom preserves edits, Undo, selection and document state, including while saving.
- At Hex scroll boundaries, repeated wheel/scroll messages retain the viewport
  without requesting repaint or synchronization. Moving away from a boundary
  resumes ordinary drawing and synchronized comparison scrolling.
- Provide line-aligned text/source comparison with ignore-space/case/EOL rules, basic lexical syntax colors, changed/conflict filters and difference navigation.
- Select changed blocks from either side or both, with merge-choice Undo/Redo and independently editable output. Three-way merge uses a common Base, merges independent changes automatically and exposes overlapping edits as conflicts.
- Require resolution or explicit permission to save unresolved conflict markers. Manual resolution requires removing marker lines and acknowledging the reviewed result; subsequent edits invalidate that acknowledgement.
- Compare folder contents recursively with semicolon-separated include/exclude globs. Offer selected/all synchronization in either direction and an explicit separate mirror mode.
- Preview all copy/update/create/remove operations before applying. Update preserves target-only files; mirror moves their file contents to recovery backups. Selecting a directory includes its scanned descendants.
- Save/load portable workspace INI files for editor, binary, text, three-way and folder sessions, including paths, rules, encodings, zoom, selected page and Hex position. Existing sessions/drafts take precedence; restored comparisons wait for Compare.
- New/clipboard untitled documents, first-save Save As, recent/reopen, tab ordering,
  middle-click and reviewed close-many operations retain independent native buffers.
- Portable font/theme/view/shortcut preferences preserve unrelated INI settings.
  Startup restore and separate bounded draft recovery snapshots are opt-in.
- Cross-file searches capture open drafts or scan selected folders, deliver bounded
  batches and validate result versions before navigation.
- Headless read-only comparison/reporting supports documented modes and exit codes.
- CSV/TSV logical cells and normalized JSON typed values have separate read-only
  comparison modes with explicit parser/result quotas.
- Declarative binary field templates expose a bounded native structure tree.
- Large text has a separate read-only 256 KiB pager with encoding-safe boundaries.
  Binary comparison consumes bounded result batches while the worker runs.

## Comparison Semantics

Equal Address compares byte at offset N in the left file with byte at offset N in
the right file. Insertion-aware uses bounded 64 KiB windows and 16-byte anchors
to resume matching after inserted/deleted regions. A byte only on the right is
`added`; a byte only on the left is `removed`. Both addresses are retained.

## Safety

- File-length-changing edits use the same undo/redo and atomic-save path as overwrite edits.
- A normal save writes a sibling temporary file, flushes it, and replaces the
  destination only after the complete new file is available.
- Opening or comparing a file never modifies it.
- Background search/replace-all uses a snapshot. Cancellation or a document revision change must discard the result without changing the editor.
- Reject invalid/lossy text conversion and embedded NUL before exposing text for editing.
- Save As adopts the new path; later Save must not overwrite the original source.
- Record source file identity, length and last-write time; refuse reads/saves using a source detected as changed in place.
- Before saving over the original path, detect path replacement/deletion and retain unsaved edits on failure. Save As may recover from an intact original handle after a path replacement/deletion.
- Revalidate the destination before commit. Use a flushed sibling temporary file, a native replacement backup for an existing destination, and recover/preserve backup files if commit fails.
- Compare the opened file versions with independent read handles. Read failures and mid-run truncation must terminate with an error, not equality or an endless loop.
- File > Reload (`Ctrl+R`) reopens disk content after the standard unsaved-change confirmation. Cancelling must preserve the current editor.
- Save and Save As perform disk writes, flush and replacement in a background worker, with percentage progress and cooperative cancellation before replacement.
- During a save, pause that document's changes and modal navigation while allowing scroll, resize, zoom, workspace switching and opening other files. Restore editing on every success/failure/cancellation path and preserve the caret.
- Successful saved snapshots become the current clean document. Never discard a committed save merely because a cancellation request arrived after replacement.
- Save-before-close/reload actions resume only after success. Closing a saving tab is deferred; failure/cancellation keeps it and aborts pending workspace exit. Opening another file creates its own session and does not repurpose a saving document.
- Close Tab removes only the chosen session. Cancelling a close retains it; closing an inactive/saved tab retains the user's active editor. A new Open request cancels queued workspace-exit continuation.

## WinMerge / Beyond Compare alignment status

The implemented workflow follows [WinMerge's file-path selection](https://manual.winmerge.org/en/Open_paths.html)
and [Beyond Compare's separate session tabs](https://www.scootersoftware.com/v5help/optionstab.html).
These products are references; the application does not claim full feature parity.

| Capability | Current status |
| --- | --- |
| Multiple retained editor/comparison sessions; individual close/save decisions | Implemented |
| Left/right editable paths, native Explorer browsing, open-file dropdown, Swap/Recompare | Implemented |
| Hex/binary equal-address and insertion-aware comparison, addresses and F7 navigation | Implemented |
| Text/source editing with common encodings and safe save | Implemented |
| Side-by-side line-aligned text/source comparison, syntax highlighting and whitespace rules | Implemented; basic lexical coloring, literal visible-row Find |
| Two-way selected-block merge and three-way conflict resolution | Implemented; independent editable output, conflict guard, choices/Undo/Redo |
| Recursive folder comparison, filters, selected copy, update and mirror synchronization | Implemented; full operation preview, backups and stale-plan rejection |
| Persistent saved comparison sessions or restored workspace tabs | Implemented via explicit workspace INI Save/Load; paths/settings only |
| Embedded folder tree | Not added: user selected left/right paths plus native Windows Browse |

## Advanced comparison limits and safety

Text merge accepts up to 16 MiB and 200,000 lines per input. The virtual source
list previews at most 8,192 characters per line; full text is retained for output.
Bounded LCS and unique-line anchors control comparison memory. Highly repetitive
large regions can form a coarse changed block; no source text is discarded.
Manual edits, block choices and output replacements share native multi-step
Undo/Redo; overlapping manual/block changes produce explicit conflicts.
The MFC bridge displays CRLF, while the save/snapshot adapter retains source
line endings. New manually inserted lines use the source's first newline style
(CRLF when none exists). Take both separates unterminated final lines.

Folder scans compare disk bytes, not unsaved editor drafts, and stop above
200,000 entries. They do not follow reparse points. Synchronization rejects
overlapping roots, unreadable/type-conflicting entries and paths changed since
the scan, including a check immediately before each copy commit. Affected clean
open editors are temporarily read-only; dirty/busy editors block the operation.
Locks include physical file identities and are released only by their owner.

Overwrite/removal recovery files retain relative paths under the target's
`.SimpleEditorTool.backups/<operation>/data/` directory, excluded from subsequent
scans. Synchronization is atomic per copied file, not across the whole folder.
Cancel/error leaves completed operations committed and reports backup/journal
paths. Retry and recovery rebuild a preview and verify expected current state
and fingerprints before applying. Empty-directory removal does not sweep new files.
Backups are retained for user review; automatic multi-file rollback is absent.
File-state guards are optimistic metadata checks, not an exclusive filesystem
transaction against other applications or a power-loss guarantee.

Workspace INI files are bounded to 1 MiB / 200 sessions and contain no unsaved
file or merge buffers. Saving paths/settings warns when drafts exist. Loading
preserves existing drafts and reports missing files. Advanced modes implement
the named local workflows; remote protocols, archives/plugins and full language
parsers from the reference products are outside this implementation. Separate
opt-in recovery generations retain bounded draft copies outside delivery;
restoring them creates untitled drafts and never overwrites original paths.
The X02-X04 independent extension decisions are in docs/EXTENSION_ASSESSMENT.md.

Content font sizes use Windows device contexts; ordinary Windows scaling remains
in effect. Native per-monitor DPI transitions are not implemented. Physical
multi-monitor/DPI acceptance is not inferred from synthetic resize/zoom checks.

## Delivery layout

- `build` contains only the production EXE and existing INI files; no PDB, screenshots, test outputs, or development subdirectories.
- Linker PDB/import libraries and compiler intermediates use `artifacts/obj`; Debug and test executables use `artifacts/debug` and `artifacts/tests`.
- Normal clean removes only allowlisted intermediates under `artifacts/obj`, preserving delivery files, settings, evidence and unknown files.


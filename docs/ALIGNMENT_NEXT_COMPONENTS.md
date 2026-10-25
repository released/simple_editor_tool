# Alignment component design

This engineering plan continues the acceptance rows in
ALIGNMENT_IMPLEMENTATION.md. It is not a product capability list or a claim of
full competitor parity. Keep the existing MFC/static native delivery and guarded
snapshot/save/preview contracts.

## Search and command-line integration

- S03 captures open editor snapshots before a background cross-file search, or
  enumerates an explicitly selected folder without following reparse points.
  Reuse text/Hex pattern engines, cap retained results, report unreadable files
  and keep text line/normalized UTF-8 positions and physical Hex addresses. Batch results to the native UI;
  navigation checks the input version before selecting a match.
- X01 parses explicit headless comparison options before the GUI singleton path.
  It reads disk inputs and writes only an explicitly named report, with an
  explicit overwrite option. Exit codes: equal 0, different 1, error 2, cancelled
  3. Reuse the same binary range/report, text merge/report and folder engines.
  Existing positional GUI launches/forwarding remain unchanged.

## Folder comparison and synchronization

- F01 adds size, size/time, byte-content and SHA-256 criteria; metadata newness
  labels; sorting and state filters. UI selections still refer to original scan
  row identities. Persist these rules with workspace settings.
- F02 keeps existing directed Update and Mirror semantics and adds newer-only,
  bidirectional and baseline-aware planning. Each operation owns its direction.
  Equal timestamps with different content and both-modified baseline entries
  are explicit conflicts. Never choose a winner for a conflict automatically.
  Deletion propagation is an explicit option and retains recovery backups.
- F03 allows preview entries/subtrees to be skipped. Persist operation intent,
  completion/error state and recovery locations under the owned backup area.
  Retry revalidates pending work; recovery previews and verifies expected
  post-operation state before restoring. Retain backups and any failed recovery
  files. Reparse/overlap/source-change/open-editor guards remain mandatory.

## Workspace and preferences

- U01 adds temporary-backed untitled documents, clipboard creation, recent and
  reopen actions, close-many decisions, middle-click close and tab ordering.
  Saving an untitled document uses Save As; opening files still retains every
  other session and draft.
- U02 makes startup restore/draft snapshots opt-in. Store owned recovery data
  outside the delivery directory, bound snapshot size/space, distinguish drafts
  from original paths and offer explicit recovery/discard. Recovery cannot
  silently overwrite an original file.
- U03 persists view/font/theme/shortcut preferences in portable INI settings,
  preserving unrelated existing keys. Current delivery retains Windows-managed
  DPI scaling and device-context font sizing. Per-monitor DPI transitions were
  not adopted; report that capability limit and physical monitor coverage.

## Additional native data workflows

- X02 evaluates CSV/TSV field and normalized JSON comparisons and supplies a
  bounded native workflow where feasible. The original review places archives,
  images and remote FTP/SFTP in a separate extension scope; assess these without
  inventing new installed runtime/server requirements or claiming them as implemented.
- X03 builds on distribution/entropy with bounded field templates and a structure
  tree. Disassembly and process/raw-device sources are independent extensions in
  the original recommendation, not mandatory new privileged data sources.
- X04 evaluates native command recording/batch profiles and plugin feasibility.
  Prefer a bounded non-executable command data model and one coherent Undo for
  an accepted native implementation. Optional DLL plugins remain an explicit
  separate decision; the review does not authorize automatic code discovery.

## Large files and validation

- P01 supplies a bounded read-only text pager/index outside the editable-text
  limit, with source checks and encoding boundaries. Add incremental result
  consumption and measure representative sparse/large inputs, peak memory and
  cancellation. Keep editable text/merge limits honest until measured otherwise.
- Run focused core/native UI checks per component and reuse unchanged evidence.
  Canonical Release/Debug delivery, exact-formal-EXE startup/forwarding,
  dependency/output inspection and the workspace-owned clean-preservation audit
  are delivery checks after the remaining runtime/build changes settle.
  Physical clipboard, multiple physical monitors, real servers and real devices
  are separate coverage, not implied by synthetic checks.

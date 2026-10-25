# Approved feature-discovery implementation

The user approved all twelve recommendations from the independent research on
2026-10-25. This is the active engineering ledger; the original thirty-row
alignment ledger records the earlier completed scope. README remains a product
entry describing actual available behavior only. Publication remains deferred.

## Baseline, assumptions and constraints

- Baseline EXE: 3,035,136 bytes, SHA-256
  D216B7CE90D07909DEACE93F049ACF914FCA76FA00E08C11C189CFC82385A179.
- Native MFC x64/static CRT/editor, guarded snapshots/atomic saves, independent
  document tabs and zero extra runtime/driver installation remain mandatory.
- There is no missing user decision for the approved scope. Use bounded native
  first versions described below; report limitations explicitly. Do not add the
  previously rejected macro/plugin/remote/raw-device extensions.
- Existing source is untracked; preserve it and the original three delivery INIs.
  No new library is needed. Reuse pinned Scintilla/Lexilla and Windows APIs.

## Implementation plan and acceptance

Implement core models, then actual native commands and persistence, followed by
focused checks. Complete every row; finishing one batch is not completion.

| ID | Approved feature / first-version acceptance | State |
| --- | --- | --- |
| R01 | Named/color/commented physical Hex ranges, list/jump, bounded workspace persistence; stale ranges cannot silently navigate after edits/reload | Complete; PASS |
| R02 | CSV/TSV key-based row alignment, column mapping and per-column numeric tolerance; duplicate keys explicit, positional default preserved | Complete; PASS |
| R03 | Blank-line/selected-line ignore and timestamp masking on comparison keys; originals/output preserved, ignored status explicit | Complete; PASS |
| R04 | Sort/deduplicate/remove-empty/trim line tools, selected scope, one native Undo, encoding/EOL preserved | Complete; PASS |
| R05 | Folder multi-file replacement planning, per-file preview/selection, precommit version guards, backup/journal, cancellation and dirty-editor locks | Complete; PASS |
| R06 | Live numeric inspector and explicit endian-aware scalar edits, exact-width/range guards and one Undo | Complete; PASS |
| R07 | Explicit two-way manual line anchors, monotonic validation, read-only aligned view; protect existing output before recompare | Complete; PASS |
| R08 | Two views of the same native Text buffer, shared Undo/save/revision, independent caret/scroll; existing tabs remain independent | Complete; PASS |
| R09 | Prefix/language/document-word completion with bounded cancellable indexing and native completion popup | Complete; PASS |
| R10 | Read-only renamed/moved content pairing, size/hash guards and ambiguity display; no automatic rename/sync plan changes | Complete; PASS |
| R11 | Separate read-only log tail, bounded decoded window, explicit rotation/truncation handling and clean worker shutdown | Complete; PASS |
| R12 | Named/custom CRC configurations, streaming calculation and known vectors; preserve existing fixed CRC32/SHA-256 workflow | Complete; PASS |

Batch order: R01/R02/R03, R04/R06/R08/R09, R05/R07/R10,
R11/R12. Shared options and workspace fields are integrated before dependent
builds; do not mutate class layouts during compilation.

## Risks and validation

- Positions are physical bytes; base addresses are presentation only. Invalidated
  bookmarks require an explicit new range instead of guessing edit rebasing.
- Rule-normalized keys never become saved/merged source text. Manual alignment
  starts with a read-only two-way view, not silent changes to merge choices.
- Shared views use one owning editor buffer; snapshots/recovery/save must capture
  it once. Only genuine insert/delete events dirty the owner.
- Batch replacement is atomic per file, not per folder. Preview does not grant
  permission to overwrite subsequently changed inputs; retain recovery material.
- Workers remain bounded/cancellable. File following is opt-in/read-only and
  cannot reload another tab's dirty buffer.
- Run focused core and Debug/Release UI cases for each component and affected
  workspace/save/merge paths. Final canonical builds, exact artifact GUI/CLI,
  output/INI inventory and clean preservation are delivery checks. Physical
  clipboard/gestures/monitors are separate coverage, not synthetic-test claims.

## Final acceptance and delivery

All twelve first-version rows passed. Changed files, purposes, commands, actual
results and explicit limitations are recorded in
[FEATURE_DISCOVERY_ACCEPTANCE.md](verification/FEATURE_DISCOVERY_ACCEPTANCE.md)
and [discovery-20261025.json](verification/discovery-20261025.json).
Release EXE: 3,271,680 bytes, SHA-256
41C83EACD05C2B0FD7097212B14A98A8136D9729447B92D76CCAE0A1EE7AABCA.
Canonical Debug/Release, focused core/native UI, exact formal GUI/CLI, delivery
inventory, original INI hashes and actual clean preservation all passed.
No required item remains in this approved twelve-row scope. Publication is deferred.
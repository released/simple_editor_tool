# Approved native workflow extensions

The user approved N01-N09 after the second independent research, 2026-10-25.
This is the active engineering ledger. Earlier R01-R12 and thirty alignment
rows are historical completed scopes. Publication remains deferred.

## Baseline and analysis

Release: 3,271,680 bytes, SHA-256
41C83EACD05C2B0FD7097212B14A98A8136D9729447B92D76CCAE0A1EE7AABCA.
Original three delivery INIs must stay byte identical. Preserve existing source,
drafts, shared views, Undo, original codecs/EOL, guarded snapshots and atomic
saves. Native MFC/static CRT/Scintilla/Win32 only; no new package/runtime/driver.

All nine recommendations are authorized; no missing user decision blocks work.
Current gaps were verified against the implementation: folder criteria are raw
only, Hex renders ASCII, change-history tracking is disabled, and native commands
for palette/string extraction/arithmetic/context/navigation/tree are absent.

## Plan and acceptance

| ID | Native first-version acceptance | State |
| --- | --- | --- |
| N01 | Explicit folder text rules and extension filter; independent raw/rules/evaluation-error results; raw synchronization guards retained | Complete / PASS |
| N02 | Read-only selectable UTF-8/UTF-16/Big5 Hex decoding, complete character-to-physical-byte mapping including row boundaries | Complete / PASS |
| N03 | Text change markers and change navigation; Hex saved-baseline differences/list/highlighting; existing Undo/shared views retained | Complete / PASS |
| N04 | Searchable native command palette with existing command routing and context/busy restrictions | Complete / PASS |
| N05 | Bounded streaming ASCII/UTF-16 strings results with physical ranges, minimum length, guarded navigation and export | Complete / PASS |
| N06 | Selected unsigned 8/16/32/64-bit XOR/AND/OR/add/subtract/shift with endian, overflow contract and one Undo | Complete / PASS |
| N07 | Expandable unchanged text comparison regions with selected context count; stable source/block mappings, output/report unchanged | Complete / PASS |
| N08 | Bounded Back/Forward navigation across file Text/Hex sessions; generation guards, shared views and closed-session safety | Complete / PASS |
| N09 | Optional expandable folder-result tree, ancestor inclusion and descendant status counts; path Browse and synchronization scope retained | Complete / PASS |

Implementation order: core models and shared optional workspace fields; actual
native commands and dialogs; focused core/native Debug+Release cases; canonical
builds and exact formal GUI/CLI; delivery/INI/dependency and clean preservation.
Do not change shared layouts during dependent builds. No automatic new work on
previously assessed plugins/macros/remote/archive/image/device extensions.

## Risks and verification

- Folder semantic equality is additional analysis, never a replacement for raw
  synchronization states or SHA/version guards. Read/parse failure is explicit.
- Decoded text is read-only; invalid sequences remain mapped to real byte ranges.
  Visible Hex decoding must remain bounded, not load entire files.
- Scintilla history markers 21-24 conflict with existing bookmark 24; move the
  bookmark marker consistently. Enable history only at an empty Undo/savepoint,
  never clear an existing Undo history merely to enable a view.
- Workers publish only against their captured generation; stale navigation and
  change overlays cannot redirect a write or falsely describe a newer buffer.
- Arithmetic modifies only the selection on a private snapshot then adopts one
  transaction. Fixed-width unsigned arithmetic is modulo 2^width; shifts must be
  less than width, and selection sizes must be divisible by word size.
- Tree/context rows are presentation mappings, never filenames/merge operations.
  Collapsing visible rows does not silently narrow an operation preview.
- Tests are selected by impact. Record actual passes, reused evidence and missing
  physical/server/clean-PC coverage separately. Product README stays user-facing.


## Final verification

All nine rows are complete. Core Release assertions: workflows 37, merge-sync
47, folder-tools 31, file-safety 28, recovery 11 and discovery 20 PASS. Native UI
Debug and Release each passed workflows 43, code-editor 26, search-tools 32,
discovery 19, discovery-completion 3, workspace 35, advanced 52, async-save 32,
zoom 34, recovery 13 and folder-tools 12. Final scoped follow-up reruns passed;
unchanged regression evidence is explicitly distinguished in acceptance.

Canonical Debug/Release passed; each byte-matched formal EXE passed 12 GUI
process assertions and 7 CLI cases. Release: 3,384,320 bytes, SHA-256
B4D36D863CF3F9165F773D9B0CC8DE561770BF4B19ED8017300458C4B96B10EF.
Clean scan 99 scripts PASS; fixture 17 / actual 18 protected files preserved;
699 allowlisted intermediates removed. Delivery remains EXE + original three
unchanged INIs. Six actual native captures/configuration reviewed. Review has
no remaining high/medium finding; approved scope has no outstanding item.

Details and explicit bounds/coverage gaps:
[acceptance](verification/NATIVE_WORKFLOW_ACCEPTANCE.md),
[identity and counts](verification/native-workflows-20261025.json),
[user guide](NATIVE_WORKFLOWS.md). Publication remains deferred.

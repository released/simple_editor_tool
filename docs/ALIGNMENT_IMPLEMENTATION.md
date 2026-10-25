# Alignment implementation ledger

User authorization: implement all recommendations in FEATURE_ALIGNMENT_REVIEW.md
(2026-10-25). This ledger records engineering status; README describes available
product behavior only. No publication is authorized by this feature request.

## Baseline and constraints

- Existing MFC x64, static CRT/MFC, singleton/forwarding, retained tabs and safe
  snapshot/atomic-save behavior are preserved. The pre-change Release is
  1,083,904 bytes, SHA256
  13CE079CEFB03C01EDF826B45A74FDCAAE38E9609604885AC44807F5CB67DEA9.
- All development/test/evidence output stays under artifacts. Delivery build
  contains EXE plus original INIs. No Python, .NET or separately installed driver
  or runtime is introduced. Required native dependencies must be statically
  linked, pinned and documented with their notices.
- Source tree starts untracked with no commits/remotes. Preserve existing work.
- Native application verification uses isolated synthetic documents. Real remote
  accounts, processes/devices and private documents are not automatic fixtures.

## Execution plan

1. Hex range/clipboard/ASCII/insert mode and streamed CSV/TSV/HTML binary reports.
2. Text-control foundation: line numbers, wrap, whitespace, code/column editing,
   bookmarks/folding, coherent Undo/Redo; search options and merged-output search.
3. Intraline/moved/similar-line diff, overview, text reports/patch and Hex merging.
4. Inspector, hashes, firmware records and range/layout tools.
5. Folder criteria, operation selection/journal/recovery and baseline-aware sync.
6. Workspace/new/recent/tab actions, preferences and recoverable draft snapshots.
7. Cross-file searches, CLI, structured format comparison and binary templates;
   native automation evaluation; paged large-text views and measured limits.

Each stage is integrated into real product commands, then built and verified
against its affected core/UI paths before continuing. A row becomes complete
only with implementation and recorded acceptance evidence; partial stages do
not complete the user's overall request. Follow the original review's scope:
X02/X03/X04 include P3 evaluations. Archives, remote sources, disassembly,
process/raw-device access and optional plugins are separate extensions in that
review. The earlier execution ledger over-expanded those rows; completing their
evaluation does not imply implementing every independent extension.

## Items

| ID | Acceptance scope | State |
| --- | --- | --- |
| B01 | Streaming CSV/TSV/HTML binary reports, full counts and ranges | Implemented; focused core/UI checks passed |
| B02 | Mouse/keyboard range selection, clipboard formats/cut/paste, selection info | Implemented; isolated-store UI checks passed; native clipboard transport NOT RUN |
| B03 | ASCII input, insert/overwrite and read-only states | Implemented; focused UI checks passed |
| B04 | Endian-aware integer/float inspector | Implemented; core analysis 24 and Debug/Release format-tools 11 passed |
| B05 | Binary difference-range merge to guarded output | Implemented; core merge-tools 19, region/core and Debug/Release format/region UI passed |
| T01 | Unicode-aware intraline highlights | Implemented; core presentation 12 and Debug/Release UI passed; actual screenshot inspected |
| T02 | Clickable difference overview, moved/similar-line information | Implemented; same focused presentation checks; bounded 2-way similarity and unique-moved-line hints |
| T03 | Output Find/Replace and coherent manual/block Undo/Redo | Implemented; Debug/Release search-tools 32 passed, including independent edits and overlap conflicts |
| T04 | Text HTML/summary reports, unified patch generation/guarded application | Implemented; core text-reports 22 and Debug/Release UI 13 passed |
| S01 | Direction/case/word/range/count/history search options | Implemented; Debug/Release search-tools 32 passed |
| S02 | Regex/escaped search and Hex wildcards/typed numeric patterns | Implemented; core patterns 11 and Debug/Release search-tools 32 passed |
| S03 | Find All/open-document/folder result lists and navigation | Implemented; Debug/Release cross-search 18 passed, including 100,005 total matches, retained result cap and cancellation |
| E01 | Line gutter/status/Go To Line/wrap/whitespace/Tab settings | Implemented; Debug/Release code-editor 26 passed |
| E02 | Decode versus output encoding/BOM/EOL conversion | Implemented as explicit converted output copies; core merge-tools 19 and Debug/Release format-tools 11 passed |
| E03 | Language coloring/indent/brackets/bookmarks/folding/column/multi-edit | Implemented selected language families; Debug/Release code-editor 26 passed |
| H01 | Streaming range/file CRC32 and SHA-256 | Implemented; core analysis 24 and native result-dialog acceptance passed |
| H02 | Intel HEX/S-record checksums/address maps/BIN conversion | Implemented; core firmware 20 and Debug/Release product conversion acceptance passed |
| H03 | Extract/fill/swap/split/join/source-array range tools | Implemented; core analysis 24 and UI code-editor 26 passed |
| H04 | Bytes per row/grouping/base address/offset-range comparisons | Implemented; core regions plus default diff regression, Debug/Release regions UI 6, report/merge guards and workspace persistence passed |
| F01 | Size/time/content/hash criteria, newness, sorting/filtering/settings | Implemented; folder-tools core 31 and Debug/Release native 12 passed |
| F02 | Explicit newer/update/mirror/both policies and baseline conflicts | Implemented; same focused acceptance plus existing merge/sync regressions passed |
| F03 | Per-operation exclusion, persistent journal/retry/backup recovery | Implemented; same focused acceptance includes content changes with unchanged metadata after preview |
| U01 | New/clipboard/recent/reopen/close-many/middle-click/reordered tabs | Implemented; Debug/Release documents 14 passed, affected async-save and Debug workspace passed; recent persistence follows U03 |
| U02 | Opt-in session restore and isolated recoverable draft snapshots | Implemented; recovery core 11 and Debug/Release native 13 passed; connected preferences/documents/workspace passed |
| U03 | Fonts/themes/shortcuts/persisted views and relevant DPI validation | Implemented; core preferences 9, Debug native 14 and Release native 15 including screenshot passed; affected zoom/save/workspace/advanced passed; physical DPI remains NOT RUN |
| X01 | Headless compare/report options and documented exit codes | Implemented; core CLI 14 and exact Debug/Release formal process cases 7 each passed |
| X02 | CSV/TSV/JSON comparison evaluation; archive/remote extension assessment | Implemented native formats; core 14 and Debug/Release native 6 passed; independent extensions assessed in EXTENSION_ASSESSMENT.md |
| X03 | Field templates/structure tree and entropy; independent extension assessment | Implemented native templates/tree; core 10, Debug inspection 11 and Release inspection 13 with captures passed; independent extensions assessed |
| X04 | Native command recording/batch evaluation; plugin feasibility | Evaluation complete in EXTENSION_ASSESSMENT.md; tested CLI adopted for comparisons/reports; no macro recorder, script compatibility or DLL plugin support claimed |
| P01 | Paged large text, bounded/incremental work and measured stress limits | Implemented pager and binary result mailbox; core paging/stress 11 in three measured runs, native inspection and Debug/Release progressive-compare 4 passed |

## Validation

Component design is recorded in ALIGNMENT_NEXT_COMPONENTS.md. X02-X04 extension
decisions are recorded in EXTENSION_ASSESSMENT.md. Rows describe implemented
workflows or the original evaluation deliverable; they do not imply complete
competitor parity. Canonical Debug/Release, exact formal GUI/CLI, PE/output and
clean preservation passed. See verification/ALIGNMENT_ACCEPTANCE.md and its JSON
for changed components, actual commands, artifact identity and explicit gaps.

Current focused evidence: core analysis 24, firmware 20, patterns 11,
merge-tools 19, presentation 12, regions 14 plus existing diff assertions,
text-reports 22 (latest HTML source/EOL labels also rerun successfully).
Debug/Release code-editor 26, search-tools 32, format-tools 11, regions 6,
presentation 8 (Release screenshot run 9) passed. Debug/Release text-reports UI 13
passed. Folder-tools core 31, Debug/Release native 12, cross-search 18 and
documents 14 passed. The changed save locks/close continuation also passed
Debug/Release async-save and Debug workspace 35. Native drop-target checks passed
in both configurations (10 each), preserving drafts and rejecting malformed data.

The shared native text-control migration passed the existing Release editor /
zoom / async-save bundle after fixing encoded caret offsets for CP950 and the
EM_SETREADONLY style bridge. Debug workspace 35, zoom 34 and async-save passed.
Both configurations passed advanced comparison/merge/folder/session checks;
Release zoom was also rerun after overview/range integration. Counts below are
earlier retained evidence and explicit platform limits.

Core `--binary-tools`: 12 assertions passed, including 1,000,005 streamed
differences, hard-link/source guards, cancellation and commit-boundary conflicts.
UI `--hex-tools`: Release 25 assertions passed (Debug earlier 20 before the added
cases). Clipboard tests use a per-control isolated store. A separate station was
refused by Windows (error 5), so native OS clipboard transport was not exercised.
No user clipboard was changed. Exact formal EXE startup/forwarding, dependency /
output inspection and clean-preservation verification passed in final delivery.
Physical DPI, remote-server
and real-device coverage are separate from synthetic/native message tests.

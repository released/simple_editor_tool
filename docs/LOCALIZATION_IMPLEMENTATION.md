# English / Traditional Chinese UI

User scope: lang_en.ini, lang_tw.ini, immediate language switching, Traditional
Chinese as first-run default; docs/USER_GUIDE.md is the Traditional Chinese
default guide and docs/USER_GUIDE_EN.md is the complete English counterpart.
No existing USER_GUIDE.md was found at entry; create those exact two guides.

Implementation plan:
1. Add a bounded native language-catalog parser with stable keys, validated
   metadata/escapes, exact UI lookups and parameter-preserving message templates.
2. Add UI-thread localization for menus, static/button captions, fixed dropdowns,
   internal page names, column headers and explicitly identified status controls.
   Never translate Scintilla/Edit buffers, file paths, workspace document tabs,
   search history, raw result cells, numeric values, schema tokens or licenses.
3. Add English / Traditional Chinese commands to both welcome/session menus,
   persist selection through portable preferences, and refresh existing UI
   without recreating editors, clearing Undo or starting/cancelling operations.
4. Ship UTF-8 lang_en.ini/lang_tw.ini beside the EXE and retain source catalogs
   under src/res. Missing/invalid catalogs retain usable built-in English UI.
5. Add matching complete user guides and product-facing README links. Verify
   catalog/preferences logic plus real Debug/Release language switching,
   active drafts/Undo/search/compare state, first-run/persisted formal startup,
   packaging and documented fallback behavior.
6. Run verified safe compiler cleanup and retire all batch-owned tests/scratch.
   Preserve original INIs and unrelated edits; finish below 20,000,000 bytes.

Architecture stays native Unicode x64 MFC with static CRT/MFC and pinned native
editor libraries. No external runtime/driver, third-party source incorporation,
Git publication or unrelated feature implementation.

Scratch: artifacts/localization-20261026, fresh owner marker, project-local TEMP
and actual byte backups of the three original runtime INIs. Never restore over
newer user changes or delete outside this project.

Implementation and selected verification are complete: 624 unique paired entries,
20 core and 17 UI localization checks per configuration, connected preference/UI
regressions, 24 formal startup/language cases, 12 formal process cases and seven
CLI cases. The runtime switch never replaces native document buffers. A Debug
lifecycle failure exposed an uncreated advanced-view invalidation; the HWND guard
was corrected and the affected language tests passed. Source code/tests, the two
complete guides and catalog sources remain; disposable builds/fixtures are retired
through the final scoped cleanup. Exact hashes/results/bytes and limitations:
verification/localization-20261026.json.

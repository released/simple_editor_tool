# Native extension assessment

Engineering decision record, 2026-10-25. This closes the evaluation parts of
X02–X04 in the original alignment review. It does not claim identical feature
coverage to the referenced products. No optional extension below is advertised
as an implemented feature.

## Additional sources and formats (X02)

CSV/TSV field comparison and normalized JSON comparison are implemented in the
native core and existing comparison session. The bounded parsers need no runtime
installation; syntax and numeric precision rules are in STRUCTURED_DATA.md.

| Extension | Feasible native approach | Decision and acceptance conditions |
| --- | --- | --- |
| ZIP/archive browsing | Explicit archive adapter, static reviewed decompressor | Separate extension. Require compressed/uncompressed/entry quotas, path traversal and symlink refusal, CRC checks, cancellation, password policy and snapshot lifetime. Repacking needs its own guarded write workflow. |
| Image comparison | Windows Imaging Component decoding and bounded pixel buffers | Separate extension. WIC is an OS component; dimensions, decoded memory, alpha/color profile/orientation rules and visual acceptance need an explicit contract. |
| FTP/SFTP | Snapshot provider with a reviewed native transport; SFTP requires SSH support | Separate extension. Require host-key verification, credentials outside portable INI, cancellation/timeouts, download quotas and remote-version checks. No transport library or server dependency was added. |

The folder comparison architecture can consume materialized read-only snapshots,
but network identity/versioning cannot safely be represented by a local pathname
alone. This is a design inference from the current source guards. Windows'
[WIC overview](https://learn.microsoft.com/en-us/windows/win32/wic/-wic-about-windows-imaging-codec)
documents the native image framework; its existence does not implement an image
comparison rule or imply a codec-installation requirement for this tool.

## Structured binary inspection (X03)

Fixed-layout field templates, a native structure tree, numeric/endian inspection,
histogram and block entropy are implemented. The local declarative format is
intentionally bounded and independently specified. ImHex's
[Pattern Language](https://docs.werwolv.net/pattern-language) includes expressions,
functions and dynamic structures; those capabilities and format compatibility
are not part of this implementation.

| Extension | Decision and acceptance conditions |
| --- | --- |
| Disassembly | Separate optional component. Select architecture/mode/base explicitly; review decoder license/version; bound output and label undecodable bytes. Fixed-width numeric inspection is not disassembly. |
| Process memory | Separate read-only provider. Requires process identity/permissions, inaccessible-region handling and explicit snapshot consistency; editing needs a separate authorization and recovery model. |
| Physical disks/volumes | Separate read-only provider with explicit device selection. Windows CreateFile supports devices, but permissions/alignment/sharing and live-volume consistency differ from ordinary files. No raw-device writes or driver installation were introduced. |

[Microsoft CreateFile documentation](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilew)
describes file/device handles. That API alone does not provide the tool's normal
atomic file replacement or rollback guarantees on devices.

## Automation and plugins (X04)

[Notepad++ macros](https://github.com/notepad-plus-plus/npp-usermanual/blob/master/content/docs/macros.md)
record/play editor actions; Beyond Compare provides a
[script command language](https://www.scootersoftware.com/v5help/scripting_reference.html)
for comparisons, reports and file operations. These are distinct models.

The adopted native automation surface is explicit headless comparison/reporting
with documented options and exit codes (COMMAND_LINE.md). It can be invoked
repeatedly by an existing shell or build runner without adding an installed
runtime to the tool. It never performs unattended synchronization or saves
editor drafts. Workspace files retain comparison settings for reviewed GUI use.

| Candidate | Assessment / decision |
| --- | --- |
| Record every window/key message | Not adopted. Mouse positions, modal file dialogs and clipboard state do not form a reproducible document command contract. |
| Bounded native editor command profiles | Feasible separate editor extension: record semantic edits/searches against a generation, preview on a private buffer, reject stale sources, cap steps/repeats/output and apply one Undo. Requires a schema and command coverage contract; no recorder/profile player is implemented or claimed. |
| Python/Lua automation | Not adopted for this zero-install native scope. No interpreter or script packages were added. |
| Automatic DLL/plugin discovery | Not adopted. It adds an executable trust/ABI/lifetime boundary and can break single-EXE portability. A future opt-in plugin API would require explicit loading, versioning, diagnostics and licensing review. |

The X04 recommendation requested evaluation first. Its deliverable is this
decision and the tested CLI surface, not a claim that Notepad++ macro recording
or Beyond Compare scripts are compatible with this application.

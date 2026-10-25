# Command-line comparison

Use `--compare` for a headless, read-only comparison. Positional file paths still
open the desktop workspace. Headless commands operate independently of an open
desktop instance and do not load or update portable preferences.

```powershell
.\SimpleEditorTool.exe --compare --left "old.bin" --right "new.bin" --mode binary --report "differences.csv"
$LASTEXITCODE
.\SimpleEditorTool.exe --compare --left "old.cpp" --right "new.cpp" --mode text --report "changes.patch" --format patch
.\SimpleEditorTool.exe --compare --left "old folder" --right "new folder" --mode folders --criterion sha256 --include "*.cpp;*.h" --report "folder-report.csv"
.\SimpleEditorTool.exe --help
```

| Mode | Report formats | Additional options |
| --- | --- | --- |
| `binary` (default) | `csv` (default), `tsv`, `html` | Equal byte offsets; `--region` |
| `binary-aligned` | `csv`, `tsv`, `html` | Nearby insertion/deletion alignment; `--region` |
| `text` | `html` (default), `summary`, `patch` | `--encoding` |
| `threeway` | `summary` | Required `--base`; `--encoding` |
| `folders` | `csv` | `--criterion`, `--include`, `--exclude`, `--no-recursive` |

`--left` and `--right` are required. Omit `--report` to compare without writing
an output. `--overwrite` explicitly permits replacing an existing report;
without it an existing output is refused, including one created while the
comparison is running. Reports cannot replace input paths or their hard links.
Folder reports must be outside both compared roots. A cancelled or failed
uncommitted report retains the existing output.

Text encoding names are `auto`, `utf8`, `utf16le`, `utf16be`, `big5`, `system`.
Text inputs are bounded to 16 MiB and 200,000 lines each. Three-way commands
report changes/conflicts; they do not write a merged source file. Folder criteria
are `content`, `size`, `sizetime`, `sha256`; size/time criteria do not guarantee
equal bytes. Synchronization remains a reviewed desktop operation.

`--region "left-start,left-length,right-start,right-length;L:start,length;R:start,length"`
uses hexadecimal values by default; prefix decimal values with `d:`. Optional
`L:` and `R:` spans exclude physical input bytes, as in the desktop region dialog.
All paths containing spaces must be quoted. Unknown or incompatible options
produce an error rather than being silently ignored.

| Exit code | Meaning |
| --- | --- |
| 0 | Equal under the selected rules |
| 1 | Differences found; a requested report may have been successfully saved |
| 2 | Invalid options, unreadable input, invalid text or output failure |
| 3 | Cancelled before output commit |

Messages go to the inherited console or redirected streams. Ctrl+C requests
cancellation. Cancellation after an atomic output commit does not undo that
commit. Batch callers should treat code 1 as a comparison result and reserve code
2 for a failed operation.

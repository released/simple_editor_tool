# Reliability and interaction review implementation

Approved scope: R1-R7, 2026-10-31. No new dependencies or framework changes.

| Item | Implementation and acceptance | Status |
| --- | --- | --- |
| R1 | Stage binary metadata/history before commit; allocation failures preserve bytes, size and Undo/Redo. | Release fault injection PASS: 6 operations / 34 failure stages |
| R2 | Guard 15 Advanced worker launches; failures restore controls, path locks and render choices. | Debug/Release reliability PASS |
| R3 | Return the created comparison session; failed creation never configures an existing draft. | Factory-failure and connected caller checks PASS |
| R4 | Track native source versions and disk stamps; retain warnings/probes/output on cancellation and reject late results. | Debug/Release reliability PASS |
| R5 | Retain failed preference revisions; explicit retry and retry/cancel/skip exit decisions. | Debug/Release reliability PASS |
| R6 | Preserve native source buffers, selection and original-line viewport during presentation updates. | Debug/Release reliability PASS |
| R7 | Index source lines; capture controls on the UI thread and prepare snapshots/side copies on the worker, with final version guards. | Timings and mixed-EOL byte equivalence PASS |

Verification selects core editing/file-safety/merge/search/preferences and native
comparison/workspace/preferences/presentation callers by actual changes. Isolated
synthetic inputs are owned by artifacts/reliability-20261031. Personal preferences
and user files are not fixtures. Compact acceptance, binary/source hashes and
cleanup status are recorded in docs/verification/reliability-20261031.json.

Release reference measurements (one local run; not an end-to-end speed claim):
500 lookups over 20,000 rows took 8,620.9 us with the prior linear reference and
1.4 us through the index. An 8,006,128-byte mixed-EOL input required 259.802 ms
for synchronous preparation versus 6.3486 ms for UI capture. Background completion
wall time was 542.699 ms while the test pumped UI events; source bytes matched.
This moves preparation work off the UI thread; it does not promise lower total
CPU or comparison time. Native text/metadata capture and final control application
remain on the UI thread and are bounded by the existing input quotas.

Debug core compiled initially. Its first global-allocation injection aborted
inside the Debug runtime; that test process was stopped by exact PID/path.
Global allocation injection is therefore Release-only (Debug STL may allocate
proxies inside noexcept constructors). Debug still exercises normal document and
native UI regression paths. No acceptance result is claimed for the aborted run.

The Debug CRT report hook now fails immediately on native assertions. It exposed
an uncreated advanced-view GetStyle call in the freshness poll; the HWND guard is
covered by the passing final Debug run. The hook is confined to test executables.
Cleanup blocks are reported independently of functional acceptance; no retention
exception or complete-cleanup claim is implied by the table above.

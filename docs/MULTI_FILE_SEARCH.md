# 跨檔搜尋

使用 **Edit > Find in open files / folder...** 或 **Ctrl+Shift+F** 開啟搜尋視窗。
沒有開啟分頁時，同一命令位於 File 選單。

**All open file editors** 搜尋每個檔案編輯分頁的快照，包含未儲存的 Text／Hex 內容。
搜尋不會先替使用者存檔，也不會替換原來的 buffer 或 Undo。
比較分頁的合併輸出使用一般 Find／Find All 搜尋。

**Folder on disk** 搜尋指定資料夾的磁碟內容。Browse 使用 Windows 資料夾選擇器。
Include／Exclude 接受分號分隔的 `*`、`?` 規則；Subfolders 控制遞迴。
掃描不穿越 reparse point，也排除同步備份目錄。

| 模式 | 說明 |
| --- | --- |
| Text literal | 一般文字，可選大小寫與整字 |
| Text escaped | `\n`、`\r`、`\t`、`\xHH`、`\uHHHH`、`\\` |
| Text regex (POSIX) | 與一般編輯器搜尋相同的 Scintilla 單行 Regex 語法 |
| Hex wildcards | 完整 byte pairs 或 nibble wildcard，例如 `AA ?F ??` |
| Typed numeric | 指定數值與 endian，例如 `u32le:1234`、`i16be:-2`、`f32le:1.5` |

Encoding 控制磁碟／Hex 快照的文字解碼；原生 Text 草稿使用目前已解碼的內容。
Text 結果列出行號與正規化 UTF-8 編輯 buffer 的位置；Hex 結果列出實際 byte address。
兩者都顯示檔案路徑與預覽。雙擊可跳轉到對應位置，開啟新檔仍會保留其他分頁。

結果以有上限的批次送到視窗，其他編輯分頁可繼續操作。最多保留 100,000 筆結果，
但完成搜尋時的命中總數仍計算全部符合項目。Cancel search 會保留已收到的部分結果，
並明確標示搜尋取消。Errors 可查看前 200 筆讀取／解碼錯誤。

跳轉前會檢查搜尋時的分頁識別、buffer 版本或磁碟檔案狀態。原分頁已關閉後再重開、
內容已編輯、或磁碟檔案已變動時，需重新搜尋。磁碟搜尋遇到已開啟的未儲存草稿，
會保留草稿並提示改用開啟文件搜尋。

文字磁碟輸入最多 32 MiB／檔，開啟文件的原生文字快照合計最多 128 MiB；
資料夾掃描最多 200,000 項。大 binary 可用串流 Hex pattern 搜尋。

## Find / Replace in open text documents

Ctrl+F and Ctrl+H open Find / Replace. The Scope selector chooses Current document
or All open text documents (including unsaved). The latter reads native Text drafts,
including untitled documents, without committing them to binary buffers or disk.
Comparison panes are outside this scope; use Current document for their output.
Binary/oversized file editors are skipped and counted in the status message.

Literal, escaped and regex text modes support Find Next/Previous, Count, Find All,
Replace and Replace All. Wrap applies across file tabs. Find All uses the same
file/line result list and rejects navigation after the corresponding draft changes.
Replacement planning uses at most 128 MiB of input snapshots; each resulting Text
buffer remains within 32 MiB. Replacement never writes files. Each changed document
gets one native Undo action, shared with its cloned view. Read-only/busy participants
prevent the replacement batch. Esc or Close / Cancel cancels a pending all-open
operation; a close request for a participating tab also cancels and retains the tab.

Ctrl+Shift+F remains the dedicated open-file/folder search, including binary modes.
Disk-folder replacements use the separate preview/journal workflow, with explicit
review of the proposed file writes.
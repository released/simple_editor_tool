# 資料夾比較、同步與復原

在比較分頁選擇 **Folders**，用左右路徑欄與 Browse 選定兩個資料夾，再按 Compare。
來源是磁碟內容；其他分頁的未儲存草稿不會自動寫入磁碟。

## 比較準則

| Compare | 判定方式 |
| --- | --- |
| Content（預設） | 逐塊比較實際 bytes |
| Size | 只比較大小；相同大小的內容可能不同 |
| Size + time | 比較大小與修改時間；時間可選精確、1 秒或 2 秒容許值 |
| SHA-256 | 逐塊計算兩側的 SHA-256，依內容指紋判定 |

結果顯示狀態、左右大小、本機修改時間、較新側，以及同時間但內容不同的分類。
點欄位標題排序，再點反向；Filter 可篩選狀態或較新側。排序／過濾後須重新選取
同步列，避免舊列號指向另一個檔案。Workspace INI 保存比較準則、時間容許值、
過濾、排序及同步設定。改變比較準則後須重新 Compare。

Include／Exclude 使用分號分隔的 `*`、`?` 規則。Subfolders 控制遞迴；
掃描不穿越 reparse point，最多 200,000 筆。

## 同步政策與 baseline

Selected 處理選取列；Sync all 處理目前可見列。選取資料夾會納入該次掃描的子項目。
箭頭表示 directed 政策的來源與目的。

| Sync | 行為 |
| --- | --- |
| Directed update | 依箭頭複製不同或目的缺少的項目，保留目的獨有檔案 |
| Directed newer only | 只複製較新的來源及目的缺少的檔案 |
| Newer both ways | 逐檔依時間決定方向；同時間不同內容列為衝突 |
| Baseline both ways | 以所選 baseline 的兩側內容指紋判斷變動與方向 |

Mirror 為獨立的單向政策，搭配 Directed update 使用。目的獨有檔案移至備份，
非空目錄不會被整棵移除。

**Save baseline** 將目前兩側指紋存成 `.setb`，位置須在兩個比較根目錄之外。
Choose baseline 指定已存的檔案。Baseline 必須對應同一組根目錄：
只有一側修改時採用該側；兩側都修改、刪除與另一側修改，或 baseline 原本不同
且兩側均未變動時，需使用者決定。刪除傳播另有開關，預設關閉。
Baseline 不自動更新；確認同步結果後可重新 Compare，再另行保存。

## 預覽、journal 與備份

每次同步與復原先顯示完整操作預覽，包括方向、來源／目的根目錄、相對路徑及大小。
**Skip selected / subtree** 排除選取項目及子目錄；**Include all** 重新納入。
衝突須先解決或明確排除，才可套用其他操作。取消預覽不改變檔案。

每組操作在目的根目錄建立：

```text
.SimpleEditorTool.backups/<operation>/operations.setj
.SimpleEditorTool.backups/<operation>/data/<relative path>
```

Journal 在變更前記錄意圖，完成後記錄輸出與備份的狀態／SHA-256，並 flush 到磁碟。
取消或失敗會保留已完成的檔案及備份。提交以個別檔案為單位；雙向同步的各方向
有自己的 journal，狀態列列出路徑。**Browse backups** 開啟最近一組的資料夾。

- **Retry journal**：選擇 `operations.setj`，驗證已完成輸出與待執行的來源／目的，
  再預覽剩餘操作。來源／目的已變動時拒絕重試，應重新 Compare。
- **Recover journal**：驗證目前輸出與備份，再預覽反向操作。恢復原有檔案時，
  此次被取代的輸出會保留於新的備份。雙向同步的 journal 可分別復原。

預覽摘要列出 journal 的完成、未開始及錯誤數量，最多顯示前 20 筆錯誤詳情。
輸出遭外部修改、備份遺失／變動、根目錄被替換，或中斷留下無法確認的輸出時，
復原會停止。損壞或截斷的 journal 不會當成安全操作清單；須保留備份供檢查。

有未儲存編輯或背景作業的開啟文件會阻擋同步。套用前會再次驗證來源／目的；
復原還會重新比對內容指紋。這些保護不等同跨檔案的檔案系統交易或斷電保證。

# 偏好設定與草稿復原

**File > Preferences** 設定內容字型、基準字級、Light／Dark／系統對比配色、
換行顯示、空白／EOL、Tab、各頁預設縮放與快捷鍵。字型基準為 6–32 pt，
縮放另以 50%–300% 計算；更換字型／配色不改變文件內容或 Undo。
內容區使用所選配色；選單、標題列、工具列與 Windows 對話框沿用系統樣式。

可改派 New、Open、Save、Find、Replace、Find in files、Reopen closed 的按鍵。
使用 `Ctrl+L`、`Ctrl+Shift+F12` 等格式；`None` 停用該動作的按鍵。
同一組按鍵不得重複指派，Ctrl+Z/Y/R/G/0 保留原有功能。選單同步顯示新按鍵。

設定與最多 20 個最近檔案保存在 EXE 旁的 `simple_editor_tool.ini`。
既有未知欄位與註解會保留；需要無法用原編碼表示的 Unicode 字元時，INI 轉成
有 BOM 的 UTF-16，保留既有文字值。設定不能寫入時會通知使用者，當次視窗的選擇
仍在記憶體中有效。最近關閉的檔案可用 **Reopen closed file** 重新開啟。

## 新建與分頁

**New text file** 建立未命名文件；**New from Unicode clipboard** 從剪貼簿建立新文件。
首次 Save 使用 Save As。新建、開啟及拖入檔案都保留其他分頁。
從剪貼簿新建的文字最多 32 MiB（以 UTF-8 bytes 計算）。

中鍵關閉指到的分頁；拖曳分頁改變順序，右鍵選單也可向左／向右移動。
Close other／left／right tabs 逐檔處理未儲存決定；選擇儲存時先等儲存成功再繼續。
取消或儲存失敗會停止剩餘關閉動作。已關閉的分頁不會因後續取消自動重開。

## 啟用復原

兩個選項預設都關閉：

- **Restore workspace tabs at startup**：自動還原已保存的路徑與檢視設定；
  比較分頁等待使用者按 Compare，不會自動同步或寫入檔案。
- **Save separate draft recovery snapshots**：定時保留檔案／未命名文件及未儲存合併輸出的
  獨立副本。間隔可設 10–600 秒。編輯器忙碌時會延後快照。

復原資料在使用者的 `%LOCALAPPDATA%\SimpleEditorTool\Recovery\Session-<Windows session>`，
不放在 `build`。不同 Windows 登入工作階段分開保存；同一工作階段使用工具既有的
單一執行個體規則。**Snapshot workspace / drafts now** 可手動建立副本。

快照保留原文字編碼、BOM 與混合換行；原編碼無法表示的新文字以 UTF-8 保存，
避免遺失字元。每個草稿最多 64 MiB、合計 256 MiB；最多 200 個工作階段／草稿。
通常保留目前與上一代，建立新一代時的儲存總量上限為 768 MiB。未知或遭修改的
復原檔案會保留，可能使新快照因空間限制而停止。

快照不是原始檔的自動存檔，也不會保存完整 Undo 歷史。超過上限的草稿需明確 Save；
程式會報告未能建立快照並保留上一份副本。定時快照之間的新編輯、突然斷電、
磁碟／檔案系統故障不保證能復原。

## 還原與清除

異常退出後，啟用草稿復原的啟動流程會顯示副本清單。也可選 **Recover draft snapshots**。
選取部分或全部副本後，工具驗證大小與 SHA-256，再開成獨立的 **Recovered** 未命名
分頁。原本已開啟的分頁、草稿與磁碟檔案都保留；Recovered 第一次儲存使用 Save As。
合併輸出以獨立文字草稿還原，需重新檢查其中的衝突標記。

**Keep / pause snapshots** 保留副本並暫停新的定時快照；可手動 Snapshot 或還原後恢復。
**Clear recovery snapshots** 先確認，再清除經驗證的目前／上一代副本；未知或遭修改的
檔案不會被刪除。**Browse recovery folder** 開啟儲存位置供檢查。

正常退出仍逐檔詢問儲存／捨棄。這些決定完成後，啟動還原資料只包含路徑與設定，
不會自動復活已捨棄的草稿；手動復原可檢查仍保留的上一代副本。
在快照／設定寫入期間開啟新檔會取消待結束流程。Esc 可取消進行中的復原 I/O；
取消前已提交的獨立副本可能仍保留。

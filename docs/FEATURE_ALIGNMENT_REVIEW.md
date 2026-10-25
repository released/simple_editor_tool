# 功能對齊盤點

盤點日期：2026-10-25。依據目前原始碼、專案既有驗收紀錄及參考工具的官方文件。
這是靜態功能分析與後續實作建議，不是新功能完成清單，也不是各工具的完整相容性認證。
README 維持工具用途與操作說明；功能差距與建議保留在本文件。

## 範圍與判定

- 維持 Windows x64、MFC/C++、靜態 runtime 與免額外安裝 driver/runtime 的交付方向。
- 比較 WinMerge、Beyond Compare、Notepad++、MadEdit 家族的相關本機檔案工作流程。
- MadEdit 家族的參考包含 MadEdit-Mod、wxMEdit；分支文件中的功能不推定為所有原版版本皆有。
- 「Hex File Viewer」尚未指定產品，先採 HxD、ImHex 作為 Hex 檢視／編輯功能的參考。
- 「已有」表示找到實作，並沿用適用的歷史驗收；「部分」表示已有基礎但缺完整使用流程；「缺」表示未找到產品入口及對應完整實作。
- P1／P2／P3 是建議實作順序，不是缺陷嚴重度。排序依本工具的 binary/文字比較用途判斷。

## 已有基礎

多檔保留、獨立比較頁、左右路徑及原生 Browse、Hex 位元組修改／插入／刪除、
文件 Undo/Redo、安全儲存、等地址／插入感知 binary compare、差異地址清單、
雙方／三方文字合併、遞迴資料夾內容比較、操作預覽／備份／鏡像、
手動工作階段 INI、背景作業取消與各頁縮放，都已有實作。
不應把這些項目重新列成尚未完成的功能。

## 功能差距

| 編號 | 功能／參考 | 現況 | 可以補強的行為 | 順序 |
| --- | --- | --- | --- | --- |
| B01 | Binary 差異報表；[BC 報表][R3] | 部分：可顯示地址與數值，無報表匯出 | CSV/TSV：左右 address、old/new byte、差異類型；連續差異區段與總數；HTML 檢視報告 | P1 |
| B02 | Hex 範圍選取／剪貼簿；[HxD][R8]、[ImHex][R9] | 部分：有搜尋反白 API，沒有完整使用者範圍選取 | 滑鼠拖選、Shift+方向鍵、Select All；Hex/Text 複製、Cut、Paste insert／overwrite；顯示起訖與長度 | P1 |
| B03 | Hex/ASCII 編輯及輸入模式；[HxD][R8] | 部分：Hex 半位元組覆寫，插入固定 00；ASCII 僅顯示 | ASCII 欄點擊／輸入、多位元組貼入、清楚的 Insert/Overwrite 狀態與唯讀切換 | P1 |
| B04 | Data Inspector；[ImHex][R10] | 缺 | 對游標／選取資料解讀 signed/unsigned 8/16/32/64-bit、float/double；Little/Big Endian；不足位元組提示 | P1 |
| B05 | Hex 差異區段合併；[BC][R16] | 缺：比較的 Hex panes 是唯讀快照，沒有區段取用命令 | 將指定差異區段取用至獨立輸出；支援新增／刪除造成的 offset 變動、預覽與重新比較 | P2 |
| T01 | 行內字元／單字差異；[WinMerge][R1]、[BC][R2] | 部分：已有行對齊及整列底色，尚無行內差異模型 | 只標出變動字元／詞，區分插入與刪除，處理 Unicode 與忽略空白設定 | P1 |
| T02 | 差異總覽與進階對齊；[WinMerge][R1]、[BC][R2] | 缺 | 可點選差異總覽、目前位置、移動區塊提示、類似行對齊；保留原始行號 | P2 |
| T03 | 合併結果編輯；[WinMerge][R1] | 部分：可手動修改，區塊 Undo/Redo 八層；手動編輯採原生 Edit Undo | 對輸出執行 Find/Replace；多步手動 Undo/Redo；手動編輯與區塊取用可銜接的歷史與衝突導覽 | P1 |
| T04 | 文字差異報表及 Patch；[BC][R3] | 缺 | 文字對照 HTML／摘要匯出、unified diff 產生；Patch 套用需檢查來源／context 並先預覽 | P2 |
| S01 | 基本搜尋選項；[Notepad++][R4] | 部分：literal、區分大小寫、向下 Find Next 並繞回；Text/Hex 兩種模式 | Match case 開關、whole word、Find Previous、搜尋範圍／選取內、Count、搜尋歷史 | P1 |
| S02 | Regex／Hex pattern 搜尋；[Notepad++][R4]、[HxD][R8] | 缺：Hex 僅接受完整 byte pairs | 有明確語法的 regex/跳脫字元；Hex byte/nibble wildcard；整數／浮點數與 endian 搜尋 | P2 |
| S03 | 搜尋全部與多檔搜尋；[Notepad++][R4]、[MadEdit-Mod][R6] | 部分：單檔 Replace All 已有，沒有結果清單或跨檔搜尋 | Find All、所有開啟文件／指定資料夾搜尋；結果顯示路徑、行／地址、預覽及跳轉 | P2 |
| E01 | 一般文字編輯基本視圖；[Notepad++][R5]、[wxMEdit][R7] | 部分：一般 Text 頁是原生 CEdit；比較列有行號及基本著色 | 一般編輯頁行號、行／欄狀態、Go To Line、word wrap、空白／Tab/EOL 顯示、Tab 設定 | P1 |
| E02 | 編碼／換行轉換；[wxMEdit][R7] | 部分：有解碼選擇及 BOM/EOL 保留；選擇 Encoding 會重新解碼來源 | 區分「以此編碼開啟」與「轉換輸出編碼」；輸出 BOM 與 CRLF/LF/CR 選項；必要時擴充 code pages | P2 |
| E03 | 程式編輯能力；[Notepad++][R5]、[MadEdit-Mod][R6] | 部分：只在比較來源列套用共通 lexical 著色 | 語言規則、一般編輯頁著色、auto-indent、括號配對、bookmark、folding；矩形選取與多游標另外拆分評估 | P2 |
| H01 | CRC／Hash；[HxD][R8] | 缺 | 全檔／選取區段 CRC32、SHA-256；背景分塊計算、進度與取消、複製結果 | P1 |
| H02 | Intel HEX／Motorola S-record；[HxD][R8] | 缺：目前依原始 byte 開檔，未解析 firmware records | 檢查 record checksum、address extension、稀疏區段／重疊；BIN 匯入匯出與可選填補值 | P2 |
| H03 | 區段工具；[HxD][R8]、[ImHex][R9] | 缺 | 依 start/length 擷取、填值／重複 pattern、byte swap、分割／合併、C byte array 匯出 | P2 |
| H04 | Hex 顯示與區間比較；[ImHex][R9] | 部分：固定每列 16 bytes，地址由 0 起算 | 每列 8/16/32 bytes、byte grouping、base address；比較指定區段／不同起點、忽略特定區段 | P2 |
| F01 | 資料夾比較準則；[BC][R11] | 部分：實際 bytes 判定相同／不同；沒有新舊分類與可選比較準則 | timestamp/size/content/hash 模式、newer/older 分類、欄位排序與狀態過濾、規則設定保存 | P1 |
| F02 | 自動雙向同步；[BC][R12] | 缺：目前選定左→右或右→左；內容不同即依來源方向複製 | 區分較新檔更新、單向鏡像、雙向更新；若要追蹤兩邊自上次同步後皆修改／刪除，另需 baseline 與衝突模型 | P2 |
| F03 | 同步操作管理；[BC][R12] | 部分：有完整預覽、Cancel 與相對路徑備份，沒有操作 journal／復原介面 | 預覽逐項跳過、持久化已完成／失敗紀錄、重試、備份瀏覽及受保護的選項復原 | P1 |
| U01 | 新建文件與分頁操作；[Notepad++][R13]、[MadEdit-Mod][R6] | 部分：已有多檔、Ctrl+W、Ctrl+Tab；無 New file／完整 tab context 操作 | 空白文件、從剪貼簿新建、Recent files、Reopen closed；關閉其他／左側／右側、滑鼠中鍵、分頁排序 | P2 |
| U02 | 自動還原與草稿復原；[Notepad++][R14] | 部分：手動 INI 只有路徑／設定，沒有未存內容 | 可選自動還原、草稿 snapshot、異常退出復原；草稿不得默默覆寫原始檔，需明確保存與清除設定 | P2 |
| U03 | 顯示與操作偏好；[Notepad++][R13] | 部分：有各頁縮放；多數字型／顏色及快捷鍵固定 | 字型／主題／對比、可設定快捷鍵、可保存視圖偏好；DPI/跨螢幕驗證 | P3 |
| X01 | CLI 比較與報表；[BC][R3] | 部分：命令列參數會開啟檔案，沒有獨立 compare/export/exit-code 協定 | 指定 left/right/base/mode、只讀比較、報表路徑、相同／不同／錯誤回傳碼 | P2 |
| X02 | 更多資料來源／格式；[BC][R2] | 缺 | 先評估 CSV/TSV 欄位比較與 JSON 正規化；壓縮檔、圖片、遠端 FTP/SFTP 另列後續範圍 | P3 |
| X03 | 結構化 binary 分析；[ImHex][R15] | 缺 | 格式欄位模板、byte distribution/entropy、結構樹；反組譯、程序記憶體／raw disk 是獨立擴充範圍 | P3 |
| X04 | 自動化／擴充；[MadEdit-Mod][R6]、[BC][R3] | 缺 | 先評估原生命令錄製／批次設定，再評估外掛；實作仍維持本專案原生交付要求 | P3 |
| P01 | 大檔文字與比較效能 | 部分：Hex 可視範圍讀取；Text 32 MiB、文字合併 16 MiB/20 萬行等保護上限 | 分頁／唯讀大文字檢視、增量比較／索引、結果分批取用及實檔壓力測試；以量測決定上限 | P2 |

## 容易混淆的現況

1. **能打開 .hex 不等於解析 Intel HEX。** 目前會看到檔案本身的文字／bytes，尚無 firmware address map。
2. **兩個方向的同步按鈕不等於自動雙向同步。** 現在的 Update 也不是只複製較新檔；凡內容不同就採使用者選定的來源覆蓋，套用前仍有預覽及備份。
3. **比較頁有 syntax 不等於一般 Text editor 有程式編輯器。** 一般編輯頁尚無 lexer、行號 gutter、folding 等。
4. **Workspace INI 不包含未儲存內容。** 目前沒有 crash recovery 或自動草稿還原。
5. **有 SelectRange API 不等於 Hex 互動選取已完整。** 目前 API 主要服務搜尋反白，沒有滑鼠拖選及 Hex 剪貼簿流程。
6. **合併來源搜尋與輸出搜尋不同。** 比較 Find 只查可見來源列／欄位，且預覽字串有長度上限；輸出 Find/Replace 尚未接上。

## 建議實作批次與驗收重點

這是下一輪的建議順序，尚未在此次盤點中實作。各批次先確認設計，再修改及執行受影響測試。

| 批次 | 內容 | 驗收重點 |
| --- | --- | --- |
| 1 | B02/B03 Hex 範圍及剪貼簿；B01 先做 CSV/TSV | 反向／跨列／EOF 選取、insert/overwrite、唯讀與同步鎖、單次 Undo；完整報表另以背景串流產生，地址與 bytes 必須一致；若只匯出 UI 截斷結果須明示，不能冒充全檔總數 |
| 2 | T01 行內差異；T03 合併輸出搜尋 | CJK/emoji、空白與 EOL 規則、長行預覽、取用區塊仍保存完整內容；未解決衝突及手動編輯不得被覆蓋 |
| 3 | S01、E01 基本搜尋與文字視圖 | 方向／範圍／整字／大小寫語義、編碼邊界、Go To Line；縮放、LF/CRLF/CR、原生 buffer/Undo 與儲存狀態保留 |
| 4 | B04/H01 Data Inspector 與 CRC/Hash | 已知向量、endian、signed/float 特例、選取不足；全檔分塊計算並保留取消與來源變動檢查 |
| 5 | F01/F03；之後才加入 F02 | 較新／較舊與同時間不同內容、預覽排除、journal 與備份復原；baseline 衝突不得自動以單一時間戳覆蓋 |
| 6 | H02/H03/H04 firmware 與區段工具；B05/T04 區段合併與 Patch 另拆子批次 | HEX/S-record checksum、稀疏地址、溢位／重疊、填補值、往返 bytes 一致；base address 與檔案 offset 分開；區段取用後的地址映射與 Patch 來源驗證 |
| 7 | S02/S03/E02/E03/U01/U02/P01；其餘 P3 | 依功能拆分；文字控制項能力與歷史模型先設計；增加效能／記憶體量測與草稿復原隔離測試 |

## 實作證據

- [Hex 控制項](../src/hex_view.cpp)：OnLButtonDown/OnChar/OnKeyDown、固定列寬及 SelectRange；沒有 Hex drag-select/clipboard message handlers。
- [搜尋介面](../src/ui/editor_dialogs.h)、[搜尋核心](../src/core/search.h)：SearchRequest 僅 find/replacement/hex/action，ScanMatches 為 byte pattern；沒有 regex／方向／選取範圍產品設定。
- [編輯與搜尋路由](../src/main_frame_edit.cpp)：RefreshText 使用 CEdit；OnEncoding 重新解碼；ExecuteSearch 的 advanced 分支沒有輸出 Replace。
- [比較視圖](../src/ui/advanced_compare.cpp)：OnDraw 為整列背景及 lexical 顏色；Cell 限制來源預覽；Find 透過可見 Cell 字串。
- [合併操作](../src/ui/advanced_compare_actions.cpp)：Take 的歷史上限八層；手動輸出編輯保護與 native Undo。
- [資料夾核心](../src/core/folder_sync.cpp)、[資料模型](../src/core/folder_sync.h)：BuildSyncPlan 是單一方向／mirror；沒有同步 baseline/journal、新舊分類。
- [工作階段](../src/workspace_sessions.cpp)、[INI 資料模型](../src/core/workspace_session.h)：保存 paths/settings，不含 draft/output。
- [產品命令](../src/resource.h)、[選單／控制項](../src/main_frame.cpp)：查核新建文件、匯出、Hash、Data Inspector 等入口的缺口。

## 驗證界線

本次只閱讀程式及官方資料、整理盤點文件；沒有修改應用程式、EXE、INI 或 README，
也沒有重新建置、操作參考軟體或重跑應用程式測試。既有測試結果不視為本次新執行。
大檔上限、完整 UI 操作、實體剪貼簿／DPI 等能力仍須在對應功能實作時實測。

## 官方參考

[R1]: https://manual.winmerge.org/en/Compare_files.html
[R2]: https://www.scootersoftware.com/v5help/viewtext.html
[R3]: https://www.scootersoftware.com/v5help/scripting_reference.html
[R4]: https://github.com/notepad-plus-plus/npp-usermanual/blob/master/content/docs/searching.md
[R5]: https://github.com/notepad-plus-plus/npp-usermanual/blob/master/content/docs/editing.md
[R6]: https://github.com/LiMinggang/madedit-mod
[R7]: https://github.com/wxMEdit/wxMEdit/wiki
[R8]: https://mh-nexus.de/en/hxd/
[R9]: https://docs.werwolv.net/imhex/views/hex-editor
[R10]: https://docs.werwolv.net/imhex/views/data-inspector
[R11]: https://www.scootersoftware.com/v5help/dir_how_to_compare.html
[R12]: https://www.scootersoftware.com/v5help/dir_sync_how_to_sync.html
[R13]: https://npp-user-manual.org/docs/user-interface/
[R14]: https://github.com/notepad-plus-plus/npp-usermanual/blob/master/content/docs/preferences.md#backup
[R15]: https://imhex.werwolv.net/
[R16]: https://www.scootersoftware.com/v5help/commandshex.html

已查閱的 Notepad++ 線上 searching/editing 頁面回傳 403，該兩項改查同專案的官方手冊 GitHub 原文。

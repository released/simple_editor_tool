#include "review_note_dialog.h"
#include "localization.h"
#include "../resource.h"
CReviewNoteDialog::CReviewNoteDialog(CWnd* parent,const simple_editor::core::ReviewNote& value,const std::wstring& location) : CDialog(IDD_TEXT_REVIEW_NOTE,parent),note(value),location_(location) {}
BOOL CReviewNoteDialog::OnInitDialog() { CDialog::OnInitDialog(); SetDlgItemTextW(2902,location_.c_str()); static_cast<CEdit*>(GetDlgItem(2900))->SetLimitText(4096); SetDlgItemTextW(2900,note.text.c_str()); CheckDlgButton(2901,note.reviewed ? BST_CHECKED : BST_UNCHECKED); return TRUE; }
void CReviewNoteDialog::OnOK() { CString text; GetDlgItemTextW(2900,text); std::wstring value(text.GetString(),text.GetLength()),error; if (value.size() > 4096 || !simple_editor::core::ValidateText(value,&error)) { UiMessageBox(L"A review note supports up to 4096 valid Unicode characters.",MB_ICONWARNING); return; } note.text = std::move(value); note.reviewed = IsDlgButtonChecked(2901) == BST_CHECKED; CDialog::OnOK(); }
BOOL CReviewNoteDialog::OnCommand(WPARAM wp,LPARAM lp) { if (LOWORD(wp) == 2903) { note = {}; EndDialog(IDOK); return TRUE; } return CDialog::OnCommand(wp,lp); }

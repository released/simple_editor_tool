#include "statistics_dialog.h"
#include "../resource.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
BEGIN_MESSAGE_MAP(CBinaryPlot,CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()
BEGIN_MESSAGE_MAP(CStatisticsDialog,CDialog)
    ON_WM_SIZE()
END_MESSAGE_MAP()
void CBinaryPlot::OnPaint() {
    CPaintDC dc(this); CRect rect; GetClientRect(rect); dc.FillSolidRect(rect,RGB(42,45,50)); rect.DeflateRect(5,5); if (!data || rect.Width() <= 0 || rect.Height() <= 0) return;
    const auto count = entropy ? data->entropy_blocks.size() : 256; if (!count) return;
    const auto maximum = *std::max_element(data->histogram.begin(),data->histogram.end());
    CPen pen(PS_SOLID,1,RGB(90,195,235)); auto* old = dc.SelectObject(&pen);
    for (std::size_t i = 0; i < count; ++i) { const double value = entropy ? data->entropy_blocks[i] / 8.0 : maximum ? static_cast<double>(data->histogram[i]) / static_cast<double>(maximum) : 0;
        const int x = rect.left + static_cast<int>(i * rect.Width() / count),next = rect.left + static_cast<int>((i + 1) * rect.Width() / count),y = rect.bottom - static_cast<int>(value * rect.Height());
        dc.FillSolidRect(x,y,(std::max)(1,next - x - 1),rect.bottom - y,selected == static_cast<int>(i) ? RGB(250,180,70) : RGB(90,195,235));
    }
    dc.FillSolidRect(rect.left,rect.bottom - 1,rect.Width(),1,RGB(130,135,145));
    if (entropy && selected >= 0 && static_cast<std::size_t>(selected) < count) { const int x = rect.left + static_cast<int>((2 * static_cast<std::size_t>(selected) + 1) * rect.Width() / (2 * count)); dc.FillSolidRect(x,rect.top,2,rect.Height(),RGB(250,180,70)); }
    dc.SelectObject(old);
}
void CBinaryPlot::OnClick(UINT,CPoint point) {
    if (!data) return; CRect rect; GetClientRect(rect); rect.DeflateRect(5,5); if (!rect.PtInRect(point)) return; const auto count = entropy ? data->entropy_blocks.size() : 256; if (!count || rect.Width() <= 0) return;
    selected = static_cast<int>((std::min<std::size_t>)(count - 1,static_cast<std::size_t>(point.x - rect.left) * count / rect.Width())); Invalidate(FALSE); if (choose) choose(static_cast<unsigned>(selected));
}
CStatisticsDialog::CStatisticsDialog(CWnd* owner,const simple_editor::core::BinaryAnalysis& data,std::uint64_t offset) : CDialog(IDD_BINARY_STATISTICS,owner),data_(data),offset_(offset) {}
BOOL CStatisticsDialog::OnInitDialog() {
    CDialog::OnInitDialog();
    std::wostringstream text; text << L"Physical offset 0x" << std::hex << offset_ << L"; " << std::dec << data_.bytes << L" bytes; entropy " << std::fixed << std::setprecision(6) << data_.entropy << L" bits/byte\r\nCRC32 " << std::hex << std::uppercase << data_.crc32 << L"\r\nSHA-256 " << data_.sha256; SetDlgItemTextW(3000,text.str().c_str());
    const auto cls = AfxRegisterWndClass(0,::LoadCursor(nullptr,IDC_CROSS),nullptr,nullptr);
    for (const auto pair : {std::make_pair(&histogram_,3001),std::make_pair(&entropy_,3002)}) { CRect bounds; GetDlgItem(pair.second)->GetWindowRect(bounds); ScreenToClient(bounds); GetDlgItem(pair.second)->ShowWindow(SW_HIDE); pair.first->data = &data_; pair.first->Create(cls,L"",WS_CHILD | WS_VISIBLE,bounds,this,pair.second + 10); }
    entropy_.entropy = true; entropy_.choose = [this](unsigned index) { ChooseBlock(index); };
    histogram_.choose = [this](unsigned index) { wchar_t label[120]{}; swprintf_s(label,L"Byte %02X: %llu occurrence(s).",index,data_.histogram[index]); SetDlgItemTextW(3003,label); };
    GetDlgItem(IDOK)->EnableWindow(FALSE); layout_.Initialize(*this,3011,3012); layout_.Fit(*this); return TRUE;
}
void CStatisticsDialog::ChooseBlock(unsigned index) {
    if (index >= data_.entropy_blocks.size()) return;
    const auto size = (std::max<std::uint64_t>)(1,data_.bytes / 512 + (data_.bytes % 512 != 0)); selected_offset = offset_ + index * size; selected_length = (std::min)(size,data_.bytes - index * size);
    wchar_t text[220]{}; swprintf_s(text,L"Block %u: physical [0x%llX, 0x%llX), %llu bytes, %.6f bits/byte.",index + 1,selected_offset,selected_offset + selected_length,selected_length,data_.entropy_blocks[index]); SetDlgItemTextW(3003,text); GetDlgItem(IDOK)->EnableWindow(selected_length != 0);
}
void CStatisticsDialog::OnOK() { if (selected_length) CDialog::OnOK(); }
void CStatisticsDialog::OnSize(UINT type,int width,int height) { CDialog::OnSize(type,width,height); layout_.Apply(*this); }

#include <afxwin.h>
#include "flowchart_picture.h"
#include "../core/flowchart.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>
namespace simple_editor::ui {
namespace {
struct DeleteObjectHandle { void operator()(void* value) const { if (value) DeleteObject(value); } };
using GdiObject = std::unique_ptr<void,DeleteObjectHandle>;
struct MemoryDC { HDC value = CreateCompatibleDC(nullptr); ~MemoryDC() { if (value) DeleteDC(value); } };
struct Picture {
    HDC dc = nullptr; HENHMETAFILE image = nullptr;
    ~Picture() { if (dc) DeleteEnhMetaFile(CloseEnhMetaFile(dc)); if (image) DeleteEnhMetaFile(image); }
    void Close() { image = CloseEnhMetaFile(dc); dc = nullptr; }
};
struct Box { int x = 0,y = 0,w = 0,h = 0; };
void Require(bool ok) { if (!ok) throw std::runtime_error("flowchart drawing failed"); }
SIZE Measure(HDC dc,const std::wstring& text,int width) {
    RECT rect{0,0,width,0}; Require(DrawTextW(dc,text.c_str(),static_cast<int>(text.size()),&rect,DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX) != 0);
    return {rect.right,rect.bottom};
}
void Label(HDC dc,const std::wstring& text,RECT rect,COLORREF color) {
    SetTextColor(dc,color); Require(DrawTextW(dc,text.c_str(),static_cast<int>(text.size()),&rect,DT_CENTER | DT_WORDBREAK | DT_NOPREFIX) != 0);
}
void Arrow(HDC dc,POINT a,POINT b,int size) {
    const auto angle = std::atan2(static_cast<double>(b.y - a.y),static_cast<double>(b.x - a.x));
    POINT points[3]{{static_cast<LONG>(b.x - size * std::cos(angle - 0.45)),static_cast<LONG>(b.y - size * std::sin(angle - 0.45))},b,
        {static_cast<LONG>(b.x - size * std::cos(angle + 0.45)),static_cast<LONG>(b.y - size * std::sin(angle + 0.45))}};
    Require(Polyline(dc,points,3) != FALSE);
}
}
std::string RenderFlowchartPicture(const std::wstring& text,bool dark,unsigned half_points,int width_twips,const std::atomic_bool& cancel) {
    using namespace simple_editor::core;
    Flowchart graph; if (!ParseFlowchart(text,graph,cancel)) return {};
    try {
        MemoryDC reference; Require(reference.value != nullptr);
        const int dpi = GetDeviceCaps(reference.value,LOGPIXELSX);
        const int font_height = (std::clamp)(MulDiv(static_cast<int>(half_points),dpi,144),10,64);
        GdiObject font(CreateFontW(-font_height,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI"));
        Require(font != nullptr); auto old_font = SelectObject(reference.value,font.get());
        // References are restored even when a measurement allocation fails.
        struct RestoreFont { HDC dc; HGDIOBJ old; ~RestoreFont() { SelectObject(dc,old); } } restore{reference.value,old_font};
        const auto count = graph.nodes.size(); std::vector<Box> boxes(count); std::vector<int> rank(count),state(count),order;
        std::vector<bool> returning(graph.edges.size(),false);
        std::function<void(std::size_t)> visit = [&](std::size_t node) {
            state[node] = 1;
            for (std::size_t e = 0; e < graph.edges.size(); ++e) if (graph.edges[e].from == node) {
                const auto next = graph.edges[e].to;
                if (state[next] == 1) returning[e] = true;
                else if (!state[next]) visit(next);
            }
            state[node] = 2; order.push_back(static_cast<int>(node));
        };
        for (std::size_t n = 0; n < count; ++n) if (!state[n]) visit(n);
        std::reverse(order.begin(),order.end());
        for (const auto node : order) for (std::size_t e = 0; e < graph.edges.size(); ++e) if (!returning[e] && graph.edges[e].from == static_cast<std::size_t>(node))
            rank[graph.edges[e].to] = (std::max)(rank[graph.edges[e].to],rank[node] + 1);
        // Long forward edges need an outside lane too, otherwise they cross nodes.
        for (std::size_t e = 0; e < graph.edges.size(); ++e)
            if (rank[graph.edges[e].to] - rank[graph.edges[e].from] != 1) returning[e] = true;
        // Parallel edges get independent lanes so their labels stay visible.
        for (std::size_t e = 0; e < graph.edges.size(); ++e) for (std::size_t previous = 0; previous < e; ++previous)
            if (graph.edges[e].from == graph.edges[previous].from && graph.edges[e].to == graph.edges[previous].to) returning[e] = true;
        const bool horizontal = graph.direction == FlowDirection::Right || graph.direction == FlowDirection::Left;
        const bool reverse = graph.direction == FlowDirection::Up || graph.direction == FlowDirection::Left;
        const int layers = *std::max_element(rank.begin(),rank.end()) + 1;
        std::vector<int> breadth(layers),depth(layers),offset(layers);
        const int pad = font_height,gap = font_height * 5;
        for (std::size_t n = 0; n < count; ++n) {
            const auto size = Measure(reference.value,graph.nodes[n].label,font_height * 14);
            auto& box = boxes[n]; box.w = (std::max)(font_height * 6,static_cast<int>(size.cx) + pad * 2); box.h = static_cast<int>(size.cy) + pad * 2;
            if (graph.nodes[n].shape == FlowShape::Diamond) { box.w += pad * 2; box.h += pad * 2; }
            if (graph.nodes[n].shape == FlowShape::Circle) box.w = box.h = (std::max)(box.w,box.h);
            breadth[rank[n]] += (horizontal ? box.h : box.w) + pad * 2;
            depth[rank[n]] = (std::max)(depth[rank[n]],horizontal ? box.w : box.h);
        }
        const int cross = *std::max_element(breadth.begin(),breadth.end()); int along = pad * 2;
        for (int r = 0; r < layers; ++r) { offset[r] = along; along += depth[r] + gap; }
        along -= gap; along += pad * 2;
        std::vector<int> used(layers);
        for (std::size_t n = 0; n < count; ++n) {
            auto& box = boxes[n]; const auto r = rank[n];
            const auto across = pad * 2 + (cross - breadth[r]) / 2 + used[r];
            const auto forward = reverse ? along - offset[r] - (horizontal ? box.w : box.h) : offset[r];
            box.x = horizontal ? forward : across; box.y = horizontal ? across : forward;
            used[r] += (horizontal ? box.h : box.w) + pad * 2;
        }
        const auto return_count = static_cast<int>(std::count(returning.begin(),returning.end(),true));
        const int main_width = horizontal ? along : cross + pad * 2;
        const int main_height = horizontal ? cross + pad * 2 : along;
        int tail = 0;
        for (std::size_t e = 0; e < graph.edges.size(); ++e) if (returning[e] && !graph.edges[e].label.empty())
            tail = (std::max)(tail,static_cast<int>(Measure(reference.value,graph.edges[e].label,font_height * 3).cy) + pad);
        const int lane = (std::max)(font_height * 4,horizontal ? tail + pad : 0);
        const int width = main_width + (horizontal ? (return_count ? (std::max)(tail,font_height * 4) : 0) : return_count * lane + pad);
        const int height = main_height + (horizontal ? return_count * lane + tail : tail);
        if (cancel || width > 8192 || height > 16384) return {};
        RECT frame{0,0,MulDiv(width,2540,dpi),MulDiv(height,2540,dpi)};
        const COLORREF background = dark ? RGB(63,63,63) : RGB(255,255,255),foreground = dark ? RGB(234,238,245) : RGB(30,40,55);
        const COLORREF line_color = dark ? RGB(150,185,236) : RGB(65,105,160),fill = dark ? RGB(46,56,72) : RGB(237,244,253);
        GdiObject background_brush(CreateSolidBrush(background)),node_brush(CreateSolidBrush(fill)),pen(CreatePen(PS_SOLID,(std::max)(1,font_height / 10),line_color));
        Require(background_brush && node_brush && pen);
        Picture picture; picture.dc = CreateEnhMetaFileW(reference.value,nullptr,&frame,nullptr); Require(picture.dc != nullptr);
        const auto dc = picture.dc;
        RECT canvas{0,0,width,height}; Require(FillRect(dc,&canvas,static_cast<HBRUSH>(background_brush.get())) != 0);
        SelectObject(dc,font.get()); SelectObject(dc,pen.get()); SelectObject(dc,node_brush.get()); SetBkMode(dc,TRANSPARENT);
        int return_lane = 0;
        for (std::size_t e = 0; e < graph.edges.size(); ++e) {
            if (cancel) return {};
            const auto& edge = graph.edges[e]; const auto a = boxes[edge.from],b = boxes[edge.to]; std::vector<POINT> points; RECT label{};
            if (!returning[e]) {
                if (horizontal) {
                    const int start = reverse ? a.x : a.x + a.w,end = reverse ? b.x + b.w : b.x,mid = (start + end) / 2;
                    points = {{start,a.y + a.h / 2},{mid,a.y + a.h / 2},{mid,b.y + b.h / 2},{end,b.y + b.h / 2}};
                    label = {mid - font_height * 4,a.y + a.h / 2 + 4,mid + font_height * 4,a.y + a.h / 2 + gap - 4};
                } else {
                    const int start = reverse ? a.y : a.y + a.h,end = reverse ? b.y + b.h : b.y,mid = (start + end) / 2;
                    points = {{a.x + a.w / 2,start},{a.x + a.w / 2,mid},{b.x + b.w / 2,mid},{b.x + b.w / 2,end}};
                    const int top = reverse ? start - gap + 4 : start + 4;
                    label = {a.x + a.w / 2 + 5,top,a.x + a.w / 2 + font_height * 9,top + gap - 8};
                }
            } else {
                const int channel = (horizontal ? main_height : main_width) + (++return_lane) * lane;
                const int sign = reverse ? -1 : 1;
                if (horizontal) {
                    const POINT start{reverse ? a.x : a.x + a.w,a.y + a.h / 2},end{reverse ? b.x + b.w : b.x,b.y + b.h / 2};
                    const int leave = start.x + sign * gap / 3,arrive = end.x - sign * gap / 3;
                    points = {start,{leave,start.y},{leave,channel},{arrive,channel},{arrive,end.y},end};
                    label = {leave + 4,channel + 4,leave + font_height * 3,channel + tail};
                } else {
                    const POINT start{a.x + a.w / 2,reverse ? a.y : a.y + a.h},end{b.x + b.w / 2,reverse ? b.y + b.h : b.y};
                    const int leave = start.y + sign * gap / 3,arrive = end.y - sign * gap / 3;
                    points = {start,{start.x,leave},{channel,leave},{channel,arrive},{end.x,arrive},end};
                    label = {channel + 4,leave + 4,channel + font_height * 3,leave + tail};
                }
            }
            Require(Polyline(dc,points.data(),static_cast<int>(points.size())) != FALSE);
            Arrow(dc,points[points.size() - 2],points.back(),font_height * 2 / 3);
            if (!edge.label.empty()) {
                const auto label_size = Measure(reference.value,edge.label,label.right - label.left);
                label.bottom = label.top + label_size.cy; Require(FillRect(dc,&label,static_cast<HBRUSH>(background_brush.get())) != 0);
                Label(dc,edge.label,label,line_color);
            }
        }
        for (std::size_t n = 0; n < count; ++n) {
            if (cancel) return {};
            const auto& box = boxes[n]; const auto shape = graph.nodes[n].shape;
            if (shape == FlowShape::Diamond) { POINT points[4]{{box.x + box.w / 2,box.y},{box.x + box.w,box.y + box.h / 2},{box.x + box.w / 2,box.y + box.h},{box.x,box.y + box.h / 2}}; Require(Polygon(dc,points,4) != FALSE); }
            else if (shape == FlowShape::Circle) Require(Ellipse(dc,box.x,box.y,box.x + box.w,box.y + box.h) != FALSE);
            else if (shape == FlowShape::Rounded) Require(RoundRect(dc,box.x,box.y,box.x + box.w,box.y + box.h,pad * 2,pad * 2) != FALSE);
            else Require(Rectangle(dc,box.x,box.y,box.x + box.w,box.y + box.h) != FALSE);
            const auto inset = shape == FlowShape::Diamond ? pad * 2 : pad;
            RECT label{box.x + inset,box.y + inset,box.x + box.w - inset,box.y + box.h - inset};
            const auto size = Measure(dc,graph.nodes[n].label,label.right - label.left); label.top = box.y + (box.h - size.cy) / 2;
            Label(dc,graph.nodes[n].label,label,foreground);
        }
        // Close releases all selected references before the owned GDI objects.
        picture.Close(); Require(picture.image != nullptr);
        const auto bytes = GetEnhMetaFileBits(picture.image,0,nullptr); if (!bytes || bytes > 262144 || cancel) return {};
        std::vector<BYTE> data(bytes); Require(GetEnhMetaFileBits(picture.image,bytes,data.data()) == bytes);
        const int goal = (std::min)((std::clamp)(width_twips,1200,24000),MulDiv(width,1440,dpi));
        std::string rtf = "{\\pict\\emfblip\\picw" + std::to_string(frame.right) + "\\pich" + std::to_string(frame.bottom) + "\\picwgoal" + std::to_string(goal) + "\\pichgoal" + std::to_string(MulDiv(height,goal,width)) + " ";
        rtf.reserve(rtf.size() + data.size() * 2 + 1); const char digits[] = "0123456789abcdef";
        for (std::size_t i = 0; i < data.size(); ++i) { if (!(i % 4096) && cancel) return {}; rtf += digits[data[i] >> 4]; rtf += digits[data[i] & 15]; }
        rtf += '}'; return rtf;
    } catch (const std::runtime_error&) { return {}; }
}
}

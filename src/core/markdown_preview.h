#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <vector>
namespace simple_editor::core {
struct MarkdownPreviewAnchor { std::size_t source_line = 0,text_offset = 0; };
struct MarkdownPreview {
    std::string rtf;
    bool truncated = false;
    std::wstring scroll_text;
    std::vector<MarkdownPreviewAnchor> anchors;
    std::size_t source_lines = 0;
};
using MarkdownDiagramRenderer = std::function<std::string(const std::wstring&,bool,unsigned,int,const std::atomic_bool&)>;
MarkdownPreview RenderMarkdownPreview(const std::wstring&,bool dark,unsigned half_points,int width_twips,const std::wstring& notice,const std::atomic_bool& cancel,
    const MarkdownDiagramRenderer& diagram = {},const std::wstring& diagram_notice = {});
}

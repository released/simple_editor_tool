#pragma once
#include <atomic>
#include <string>
namespace simple_editor::ui {
// Generates only our own in-memory GDI picture; no external metafiles are read.
std::string RenderFlowchartPicture(const std::wstring&,bool dark,unsigned half_points,int width_twips,const std::atomic_bool&);
}

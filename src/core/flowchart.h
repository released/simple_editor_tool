#pragma once
#include <atomic>
#include <string>
#include <vector>
namespace simple_editor::core {
enum class FlowShape { Rectangle, Rounded, Diamond, Circle };
enum class FlowDirection { Down, Up, Right, Left };
struct FlowNode { std::wstring id,label; FlowShape shape = FlowShape::Rectangle; };
struct FlowEdge { std::size_t from = 0,to = 0; std::wstring label; };
struct Flowchart { FlowDirection direction = FlowDirection::Down; std::vector<FlowNode> nodes; std::vector<FlowEdge> edges; };
// Deliberately bounded, non-executing flowchart subset; failure leaves output empty.
bool ParseFlowchart(const std::wstring&,Flowchart&,const std::atomic_bool&);
}

#pragma once
#include "line_merge.h"
#include <atomic>
namespace simple_editor::core {
struct MarkdownHeading { std::wstring title; std::size_t line = 0; unsigned level = 1; };
bool MarkdownOutline(const std::wstring& text,const std::atomic_bool& cancel,std::vector<MarkdownHeading>* headings,bool* truncated);
struct MovedBlock { std::size_t left = 0,right = 0,lines = 0; };
bool FindMovedBlocks(const TextMergeModel& model,const std::atomic_bool& cancel,std::vector<MovedBlock>* moves,bool* truncated,unsigned minimum_lines = 2);
}

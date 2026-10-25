#pragma once
#include <atomic>
#include <string>
bool TransformEditorLines(const std::string& input,int operation,const std::atomic_bool& cancel,std::string* output,std::wstring* error);
std::string CompleteDocumentWords(const std::string& input,const std::string& prefix,const std::string& keywords,const std::atomic_bool& cancel);

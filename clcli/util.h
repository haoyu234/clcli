#pragma once

#include <string>
#include <vector>
#include <clang-c/Index.h>

std::string GetTypeSpelling(CXType type);
std::string GetCursorSpelling(CXCursor cursor);
std::string GetCursorDisplayName(CXCursor cursor);

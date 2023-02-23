#include <fmt/format.h>

#include "util.h"

std::string GetTypeSpelling(CXType type)
{
    CXString spelling = clang_getTypeSpelling(type);
    std::string spellingStr = clang_getCString(spelling);
    clang_disposeString(spelling);

    return spellingStr;
}

std::string GetCursorSpelling(CXCursor cursor)
{
    CXString spelling = clang_getCursorSpelling(cursor);
    std::string spellingStr = clang_getCString(spelling);
    clang_disposeString(spelling);

    return spellingStr;
}

std::string GetCursorDisplayName(CXCursor cursor)
{
    CXString spelling = clang_getCursorDisplayName(cursor);
    std::string spellingStr = clang_getCString(spelling);
    clang_disposeString(spelling);

    return spellingStr;
}

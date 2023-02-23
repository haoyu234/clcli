#include <unistd.h>
#include <fmt/format.h>

#include "visit.h"

void processFile(
    const char *path, 
    int argc, 
    const char *argv[])
{
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit tu = clang_parseTranslationUnit(
        index,
        path,
        argv, argc,
        0, 0,
        CXTranslationUnit_SkipFunctionBodies);

    uint32_t flags = 0;

    size_t num  = clang_getNumDiagnostics(tu);
    for (size_t i = 0; i < num; ++ i)
    {
        CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);
        CXString message = clang_getDiagnosticSpelling(diagnostic);
        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diagnostic);
        CXSourceLocation loc = clang_getDiagnosticLocation(diagnostic);

        CXString name;
        unsigned int line = 0, col = 0;
        clang_getPresumedLocation(loc, &name, &line, &col);

        fmt::print(
            stderr,
            "{}:{}:{} message: {}\n",
            clang_getCString(name),
            line,
            col,
            clang_getCString(message));

        clang_disposeString(name);
        clang_disposeString(message);
        clang_disposeDiagnostic(diagnostic);

        flags |= severity;
    }

    VisitTranslationUnit(tu);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
}

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        return EXIT_FAILURE;
    }

    int result = access(
        argv[1],
        R_OK
    );

    if (result)
    {
        return EXIT_FAILURE;
    }
    
    processFile(
        argv[1],
        argc - 2,
        argv + 2);

    return EXIT_SUCCESS;
}
#include <unistd.h>
#include <fmt/format.h>

#include "visit.h"
#include "builder.h"

void processFile(
    const char *path, 
    int argc, 
    const char *argv[])
{
    auto builder = NewBuilder();
    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index,
        path,
        argv, argc,
        0, 0,
        CXTranslationUnit_SkipFunctionBodies);

    uint32_t flags = 0;

    size_t num  = clang_getNumDiagnostics(translationUnit);
    for (size_t i = 0; i < num; ++ i)
    {
        CXDiagnostic diagnostic = clang_getDiagnostic(translationUnit, i);
        CXString message = clang_getDiagnosticSpelling(diagnostic);
        CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diagnostic);
        CXSourceLocation location = clang_getDiagnosticLocation(diagnostic);

        CXString name;
        unsigned int line = 0, column = 0;
        clang_getPresumedLocation(location, &name, &line, &column);

        fmt::print(
            stderr,
            "{}:{}:{} message: {}\n",
            clang_getCString(name),
            line,
            column,
            clang_getCString(message));

        clang_disposeString(name);
        clang_disposeString(message);
        clang_disposeDiagnostic(diagnostic);

        flags |= severity;
    }

    VisitTranslationUnit(
        builder, 
        translationUnit);

    clang_disposeTranslationUnit(translationUnit);
    clang_disposeIndex(index);

    builder->Output(stdout);
    FreeBuilder(builder);
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

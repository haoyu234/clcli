#include <set>
#include <string>
#include <vector>
#include <unistd.h>
#include <fmt/format.h>

#include "visit.h"
#include "builder.h"

struct ClcliOptions
{
    bool isCpp = false;
    std::string workdir = ".";
    std::string standard = "";
    std::string output = "messages";
    std::vector<std::string> inputs;
    std::vector<std::string> includeDirs;
};

void ProcessFile(
    Builder *builder,
    struct ClcliOptions *options,
    uint32_t inputPos)
{
    std::vector<std::string> storage;
    storage.reserve(10 + options->includeDirs.size());

    std::vector<const char *> clangArgs;

    clangArgs.push_back("-x");
    clangArgs.push_back(options->isCpp ? "c++" : "c");

    if (!options->standard.empty())
    {
        storage.push_back(fmt::format("-std={}", options->standard));
        clangArgs.push_back(storage.back().c_str());
    }

    for (const auto& includeDir : options->includeDirs)
    {
        storage.push_back(fmt::format("-I{}", includeDir));
        clangArgs.push_back(storage.back().c_str());
    }

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit translationUnit = clang_parseTranslationUnit(
        index,
        options->inputs[inputPos - 1].c_str(),
        clangArgs.data(),
        clangArgs.size(),
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
}

bool ParseOptions(
    struct ClcliOptions *options,
    int argc,
    char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "C:I:s:n:"))!= -1)
    {
        switch (opt)
        {
            case 'C':
                options->workdir = optarg;
                break;
            case 'I':
                options->includeDirs.push_back(optarg);
                break;
            case 's':
                options->standard = optarg;
                options->isCpp = options->standard.find('+') != std::string::npos;
                break;
            case 'n':
                options->output = optarg;
                break;
        }
    }

    for (int i = optind; i < argc; ++ i)
    {
        options->inputs.push_back(
            argv[i]
        );
    }

    return !options->inputs.empty();
}

void Write(std::string path, const std::string& contents)
{
    FILE *f = fopen(path.c_str(), "wb+");
    if (f)
    {
        fwrite(
            contents.data(),
            contents.size(),
            1,
            f
        );
        fclose(f);
    }
}

int main(int argc, char *argv[])
{
    struct ClcliOptions options;
    const bool success = ParseOptions(&options, argc, argv);

    if (!success)
    {
        return EXIT_FAILURE;
    }
    
    if (!options.workdir.empty())
    {
        chdir(options.workdir.c_str());
    }

    for (const auto& input : options.inputs)
    {
        if (access(input.c_str(), R_OK))
        {
            fmt::print(stderr, "{}: No such file or directory.\n", input);
            return EXIT_FAILURE;
        }
    }

    auto outputHeaderName = fmt::format("{}.h", options.output);
    auto outputSourceName = fmt::format("{}.{}", options.output, options.isCpp ? "cpp" : "c");
    
    auto builder = NewBuilder();
    builder->Include(outputHeaderName);

    for (int i = 0; i < (int) options.inputs.size(); ++ i)
    {
        ProcessFile(builder, &options, i + 1);
    }

    Write(outputSourceName, builder->GetSource());
    Write(outputHeaderName, builder->GetSourceHeader());
    FreeBuilder(builder);
    return EXIT_SUCCESS;
}

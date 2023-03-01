#include <vector>
#include <string>
#include <cassert>
#include <fmt/format.h>
#include <boost/algorithm/string/predicate.hpp>

#include "builder.h"
#include "util.h"

std::string StripPrefixDot(const std::string& path)
{
    if (boost::algorithm::istarts_with(path, "./")
        || boost::algorithm::istarts_with(path, ".\\"))
    {
        return {
            path.data() + 2,
            path.size() - 2
        };
    }
    else
    {
        return path;
    }
}

struct BuilderV1 : public Builder
{
    void EnterObject(CXCursor cursor) override;
    void LeaveObject() override;

    void DefineNumberField(CXCursor cursor) override;
    void DefineArrayField(CXCursor cursor, CXCursor element) override;
    void DefineObjectField(CXCursor cursor) override;

    void LineInfo(CXCursor cursor);
    void DefineFixedArrayField(CXCursor cursor, CXCursor elementType);
    void DefineFlexableArrayField(CXCursor cursor, CXCursor elementType);
    void Include(std::string header) override;

    std::string GetSource() override;
    std::string GetSourceHeader() override;

    std::string currentObjectType;
    std::string currentObjectDisplayName;

    bool isUnion;
    bool inObject;
    fmt::memory_buffer sourceBuffer;
    fmt::memory_buffer headerBuffer;

    bool prevFieldIsNumber;
    std::string prevFieldDisplayName;

    std::vector<std::string> includedFiles;
};

void BuilderV1::EnterObject(CXCursor cursor)
{
    assert(!inObject);

    CXType type = clang_getCursorType(cursor);
    CXCursor elementType = clang_getTypeDeclaration(
        clang_getCanonicalType(type));

    inObject = true;
    isUnion = elementType.kind == CXCursor_UnionDecl;
    currentObjectType = GetTypeSpelling(type);
    currentObjectDisplayName = GetCursorDisplayName(cursor);

    prevFieldIsNumber = false;
    prevFieldDisplayName.clear();
    
    fmt::format_to(
        std::back_inserter(sourceBuffer),
        "\n"
    );
    
    LineInfo(cursor);

    fmt::format_to(
        std::back_inserter(sourceBuffer),
        "static const clColumn {}Columns[] = {{\n",
        currentObjectDisplayName
    );
}

void BuilderV1::LeaveObject()
{
    assert(inObject);
    
    fmt::format_to(
        std::back_inserter(sourceBuffer),
        "}};\n"
    );
    
    if (!isUnion)
    {
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "const clColumn {}Object[] = {{\n",
            currentObjectDisplayName
        );
        
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_OBJECT({}, {}Columns),\n",
            currentObjectType,
            currentObjectDisplayName
        );
        
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "}};\n"
        );

        fmt::format_to(
            std::back_inserter(headerBuffer),
            "extern const struct clColumn {}Object[];\n",
            currentObjectDisplayName
        );
    }

    inObject = false;
    isUnion = false;
    currentObjectType.clear();
    currentObjectDisplayName.clear();
    prevFieldIsNumber = false;
    prevFieldDisplayName.clear();
}

void BuilderV1::DefineNumberField(CXCursor cursor)
{
    prevFieldIsNumber = true;
    prevFieldDisplayName = GetCursorDisplayName(cursor);

    fmt::format_to(
        std::back_inserter(sourceBuffer),
        "    DEFINE_COLUMN_NUMBER({}, {}),\n",
        currentObjectType,
        prevFieldDisplayName
    );
}

void BuilderV1::DefineArrayField(CXCursor cursor, CXCursor elementType)
{
    bool isFixedArray = true;
    static const char *keywordStr[] = {
        "len",
        "num",
        "size",
    };

    if (prevFieldIsNumber)
    {
        for (const auto kw : keywordStr)
        {
            if (boost::algorithm::iends_with(
                prevFieldDisplayName,
                kw))
            {
                isFixedArray = false;
            }
        }
    }

    if (isFixedArray)
    {
        DefineFixedArrayField(cursor, elementType);
    }
    else
    {
        DefineFlexableArrayField(cursor, elementType);
    }
}

void BuilderV1::DefineFixedArrayField(CXCursor cursor, CXCursor elementType)
{
    assert(inObject);
    prevFieldIsNumber = false;
    prevFieldDisplayName = GetCursorDisplayName(cursor);

    if (elementType.kind != CXCursor_NoDeclFound)
    {
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_COLUMN_OBJECT_FIXED_ARRAY({}, {}, {}Object),\n",
            currentObjectType,
            prevFieldDisplayName,
            GetCursorDisplayName(elementType)
        );
    }
    else
    {
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_COLUMN_FIXED_ARRAY({}, {}),\n",
            currentObjectType,
            prevFieldDisplayName
        );
    }
}

void BuilderV1::DefineFlexableArrayField(CXCursor cursor, CXCursor elementType)
{
    assert(inObject);
    prevFieldIsNumber = false;
    prevFieldDisplayName = GetCursorDisplayName(cursor);
    
    if (elementType.kind != CXCursor_NoDeclFound)
    {
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_COLUMN_OBJECT_FLEXIBLE_ARRAY({}, {}, {}Object),\n",
            currentObjectType,
            prevFieldDisplayName,
            GetCursorDisplayName(elementType)
        );
    }
    else
    {
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_COLUMN_FLEXIBLE_ARRAY({}, {}),\n",
            currentObjectType,
            prevFieldDisplayName
        );
    }
}

void BuilderV1::DefineObjectField(CXCursor cursor)
{
    assert(inObject);
    prevFieldIsNumber = false;
    prevFieldDisplayName = GetCursorDisplayName(cursor);
    
    CXType type = clang_getCursorType(cursor);
    CXCursor elementType = clang_getTypeDeclaration(
        clang_getCanonicalType(type));
    
    if (elementType.kind != CXCursor_UnionDecl)
    {
        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_COLUMN_OBJECT({}, {}, {}Object),\n",
            currentObjectType,
            prevFieldDisplayName,
            GetCursorDisplayName(elementType)
        );
    }
    else
    {
        CXType type = clang_getCursorType(cursor);
        CXCursor elementType = clang_getTypeDeclaration(
            clang_getCanonicalType(type));

        fmt::format_to(
            std::back_inserter(sourceBuffer),
            "    DEFINE_COLUMN_UNION({}, {}, {}Columns),\n",
            currentObjectType,
            prevFieldDisplayName,
            GetCursorDisplayName(elementType)
        );
    }
}
    
void BuilderV1::LineInfo(CXCursor cursor)
{
    CXSourceLocation location = clang_getCursorLocation(cursor);
    
    CXString name;
    unsigned int line = 0, column = 0;
    clang_getPresumedLocation(location, &name, &line, &column);

    fmt::format_to(
        std::back_inserter(sourceBuffer),
        "// line {}:{}:{}\n",
        StripPrefixDot(clang_getCString(name)),
        line,
        column
    );
    
    clang_disposeString(name);
}

void BuilderV1::Include(std::string name)
{
    includedFiles.push_back(
        StripPrefixDot(name)
    );
}

std::string BuilderV1::GetSource()
{
    fmt::memory_buffer source;
    fmt::format_to(
        std::back_inserter(source),
        "// generated by the clcli. DO NOT EDIT!\n\n"
    );

    fmt::format_to(
        std::back_inserter(source),
        "#include <columns.h>\n"
    );

    for (const auto& includedFile : includedFiles)
    {
        fmt::format_to(
            std::back_inserter(source),
            "\n#include \"{}\"",
            includedFile
        );
    }
    
    fmt::format_to(
        std::back_inserter(source),
        "\n{:.{}}", 
        sourceBuffer.data(), 
        sourceBuffer.size()
    );

    return {
        source.data(),
        source.size(),
    };
}

std::string BuilderV1::GetSourceHeader()
{
    fmt::memory_buffer sourceHeader;
    fmt::format_to(
        std::back_inserter(sourceHeader),
        "// generated by the clcli. DO NOT EDIT!\n\n"
    );

    fmt::format_to(
        std::back_inserter(sourceHeader),
        "#pragma once\n\n"
    );
    
    fmt::format_to(
        std::back_inserter(sourceHeader),
        "struct clColumn;\n"
    );
    
    fmt::format_to(
        std::back_inserter(sourceHeader),
        "\n{:.{}}", 
        headerBuffer.data(), 
        headerBuffer.size()
    );

    return {
        sourceHeader.data(),
        sourceHeader.size(),
    };
}

Builder *NewBuilder()
{
    return new BuilderV1();
}

void FreeBuilder(Builder *builder)
{
    delete builder;
}

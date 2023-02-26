#include <vector>
#include <string>
#include <cassert>
#include <fmt/format.h>

#include "builder.h"
#include "util.h"

struct BuilderV1 : public Builder
{
    void EnterObject(CXCursor cursor) override;
    void LeaveObject() override;

    void DefineNumberField(CXCursor cursor) override;
    void DefineFixedArrayField(CXCursor cursor, CXCursor elementType) override;
    void DefineFlexableArrayField(CXCursor cursor, CXCursor elementType) override;
    void DefineObjectField(CXCursor cursor) override;

    void LineInfo(CXCursor cursor);
    
    void Output(FILE *pOut) override;

    std::string currentObjectType;
    std::string currentObjectDisplayName;

    bool isUnion;
    bool inObject;
    fmt::memory_buffer buffer;
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
    
    fmt::format_to(
        std::back_inserter(buffer),
        "\n"
    );
    
    LineInfo(cursor);

    fmt::format_to(
        std::back_inserter(buffer),
        "static const clColumn {}Columns[] = {{\n",
        currentObjectDisplayName
    );
}

void BuilderV1::LeaveObject()
{
    assert(inObject);
    
    fmt::format_to(
        std::back_inserter(buffer),
        "}};\n"
    );
    
    if (!isUnion)
    {
        fmt::format_to(
            std::back_inserter(buffer),
            "const clColumn {}Object[] = {{\n",
            currentObjectDisplayName
        );
        
        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_OBJECT({}, {}Columns)\n",
            currentObjectType,
            currentObjectDisplayName
        );
        
        fmt::format_to(
            std::back_inserter(buffer),
            "}};\n"
        );
    }

    inObject = false;
    isUnion = false;
    currentObjectType.clear();
    currentObjectDisplayName.clear();
}

void BuilderV1::DefineNumberField(CXCursor cursor)
{
    fmt::format_to(
        std::back_inserter(buffer),
        "    DEFINE_COLUMN_NUMBER({}, {}),\n",
        currentObjectType,
        GetCursorDisplayName(cursor)
    );
}

void BuilderV1::DefineFixedArrayField(CXCursor cursor, CXCursor elementType)
{
    assert(inObject);

    if (elementType.kind != CXCursor_NoDeclFound)
    {
        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_COLUMN_OBJECT_FIXED_ARRAY({}, {}, {}Object),\n",
            currentObjectType,
            GetCursorDisplayName(cursor),
            GetCursorDisplayName(elementType)
        );
    }
    else
    {
        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_COLUMN_FIXED_ARRAY({}, {}),\n",
            currentObjectType,
            GetCursorDisplayName(cursor)
        );
    }
}

void BuilderV1::DefineFlexableArrayField(CXCursor cursor, CXCursor elementType)
{
    assert(inObject);
    
    if (elementType.kind != CXCursor_NoDeclFound)
    {
        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_COLUMN_OBJECT_FLEXIBLE_ARRAY({}, {}, {}Object),\n",
            currentObjectType,
            GetCursorDisplayName(cursor),
            GetCursorDisplayName(elementType)
        );
    }
    else
    {
        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_COLUMN_FLEXIBLE_ARRAY({}, {}),\n",
            currentObjectType,
            GetCursorDisplayName(cursor)
        );
    }
}

void BuilderV1::DefineObjectField(CXCursor cursor)
{
    assert(inObject);
    
    CXType type = clang_getCursorType(cursor);
    CXCursor elementType = clang_getTypeDeclaration(
        clang_getCanonicalType(type));
    
    if (elementType.kind != CXCursor_UnionDecl)
    {
        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_COLUMN_OBJECT({}, {}, {}Object),\n",
            currentObjectType,
            GetCursorDisplayName(cursor),
            GetCursorDisplayName(elementType)
        );
    }
    else
    {
        CXType type = clang_getCursorType(cursor);
        CXCursor elementType = clang_getTypeDeclaration(
            clang_getCanonicalType(type));

        fmt::format_to(
            std::back_inserter(buffer),
            "    DEFINE_COLUMN_UNION({}, {}, {}Columns),\n",
            currentObjectType,
            GetCursorDisplayName(cursor),
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
        std::back_inserter(buffer),
        "//line {}:{}:{}\n",
        clang_getCString(name),
        line,
        column
    );
    
    clang_disposeString(name);
}

void BuilderV1::Output(FILE *pOut)
{
    fmt::print(pOut, "#pragma once\n\n");
    fmt::print(pOut, "#include <columns.h>\n");
    fmt::print(pOut, "{:{}}\n", buffer.data(), buffer.size());
}

Builder *NewBuilder()
{
    return new BuilderV1();
}

void FreeBuilder(Builder *builder)
{
    delete builder;
}

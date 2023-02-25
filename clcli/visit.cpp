#include <assert.h>
#include <fmt/format.h>

#include "visit.h"
#include "util.h"


void VisitUnionOrStruct(CXCursor cursor);
void VisitUnionOrStructField(CXCursor cursor, CXCursor parent);

void LineInfo(CXCursor cursor)
{
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    
    CXString name;
    unsigned int line = 0, col = 0;
    clang_getPresumedLocation(loc, &name, &line, &col);

    fmt::print(
        "//line {}:{}:{}\n",
        clang_getCString(name),
        line,
        col);
    
    clang_disposeString(name);
}

void LineError(CXCursor cursor)
{
    CXType type = clang_getCursorType(cursor);
    CXType type2 = clang_getCanonicalType(type);

    const bool is_definition = clang_isCursorDefinition(cursor);
    if (!is_definition)
    {
        fmt::print(
            "#error {} {}\n",
            GetTypeSpelling(type2),
            GetCursorDisplayName(cursor));
        return ;
    }

    CXSourceLocation loc = clang_getCursorLocation(cursor);

    CXString name;
    unsigned int line = 0, col = 0;
    clang_getPresumedLocation(loc, &name, &line, &col);

    fmt::print(
        "#error {} {}:{}:{}\n",
        GetTypeSpelling(type2),
        clang_getCString(name),
        line,
        col);

    clang_disposeString(name);
}

bool CheckHasField(CXCursor cursor)
{
    bool hasField = false;

    visitChildren(
        cursor,
        [&](const CXCursor cursor) {
            if (!hasField)
            {
                switch (cursor.kind)
                {
                    case CXCursor_CXXBaseSpecifier:
                        hasField = CheckHasField(cursor);
                        break;
                    case CXCursor_FieldDecl:
                        hasField = true;
                        break;
                    default:
                        break;
                }
            }
        }
    );

    return hasField;
}

void HandleFieldNumber(CXCursor cursor, CXCursor parent)
{
    fmt::print(
        "    DEFINE_COLUMN_NUMBER({}, {}),\n",
        GetTypeSpelling(clang_getCursorType(parent)),
        GetCursorDisplayName(cursor));
}

void HandleFieldArray(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXType elementType = clang_getArrayElementType(type);
    CXCursor elementTypeDecl = clang_getTypeDeclaration(
        clang_getCanonicalType(elementType));

    if (elementTypeDecl.kind != CXCursor_NoDeclFound)
    {
        fmt::print(
            "    DEFINE_COLUMN_OBJECT_FIXED_ARRAY({}, {}, {}Object),\n",
            GetTypeSpelling(clang_getCursorType(parent)),
            GetCursorDisplayName(cursor),
            GetCursorDisplayName(elementTypeDecl));
    }
    else
    {
        fmt::print(
            "    DEFINE_COLUMN_FIXED_ARRAY({}, {}),\n",
            GetTypeSpelling(clang_getCursorType(parent)),
            GetCursorDisplayName(cursor));
    }
}

void HandleFieldStruct(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXCursor elementTypeDecl = clang_getTypeDeclaration(
        clang_getCanonicalType(type));

    fmt::print(
        "    DEFINE_COLUMN_OBJECT({}, {}, {}Object),\n",
        GetTypeSpelling(clang_getCursorType(parent)),
        GetCursorDisplayName(cursor),
        GetCursorDisplayName(elementTypeDecl));
}

void HandleFieldUnion(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXCursor elementTypeDecl = clang_getTypeDeclaration(
        clang_getCanonicalType(type));

    CXType parentType = clang_getCursorType(parent);
    
    fmt::print(
        "    DEFINE_COLUMN_UNION({}, {}, {}Columns),\n",
        GetTypeSpelling(parentType),
        GetCursorDisplayName(cursor),
        GetCursorDisplayName(elementTypeDecl));
}

void HandleField(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXType type2 = clang_getCanonicalType(type);

    CXCursor fieldTypeDecl = clang_getTypeDeclaration(type2);
    switch (fieldTypeDecl.kind)
    {
        case CXCursor_StructDecl:
            return HandleFieldStruct(cursor, parent);
        case CXCursor_UnionDecl:
            return HandleFieldUnion(cursor, parent);
        default:
            break;
    }

    switch (type2.kind)
    {
        case CXType_Bool:
        case CXType_Char_U:
        case CXType_UChar:
        case CXType_Char16:
        case CXType_Char32:
        case CXType_UShort:
        case CXType_UInt:
        case CXType_ULong:
        case CXType_ULongLong:
        case CXType_UInt128:
        case CXType_Char_S:
        case CXType_SChar:
        case CXType_Short:
        case CXType_Int:
        case CXType_Long:
        case CXType_LongLong:
        case CXType_Int128:
        case CXType_Float:
        case CXType_Double:
        case CXType_LongDouble:
        case CXType_Float128:
        case CXType_Half:
        case CXType_Float16:
        case CXType_Enum:
            return HandleFieldNumber(cursor, parent);
        case CXType_ConstantArray:
            return HandleFieldArray(cursor, parent);
        default:
            LineError(cursor);
            break;
    }
}

void HandleBaseClass(CXCursor cursor, CXCursor parent)
{
    visitChildren(
        cursor,
        VisitUnionOrStructField,
        parent
    );
}

void VisitUnionOrStructField(CXCursor cursor, CXCursor parent)
{
    switch (cursor.kind)
    {
        case CXCursor_CXXBaseSpecifier:
            return HandleBaseClass(
                clang_getCursorDefinition(cursor),
                parent);
        case CXCursor_FieldDecl:
            return HandleField(cursor, parent);
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            return LineError(cursor);
        default:
            return ;
    }
}

void HandleStruct(CXCursor cursor)
{
    auto name = GetCursorDisplayName(cursor);
    const CXType type = clang_getCursorType(cursor);

    fmt::print(
        "static const clColumn {}Object[] = {{\n",
        name);
    fmt::print(
        "    DEFINE_OBJECT({}, {}Columns)\n",
        GetTypeSpelling(type),
        name);
    fmt::print("}};\n");
}

void VisitUnionOrStruct(CXCursor cursor)
{
    const auto name = GetCursorDisplayName(cursor);
    if (name.empty())
    {
        return;
    }

    if (!CheckHasField(cursor))
    {
        return ;
    }

    fmt::print("\n");

    LineInfo(cursor);

    fmt::print(
        "static const clColumn {}Columns[] = {{\n",
        name);

    visitChildren(
        cursor,
        VisitUnionOrStructField,
        cursor
    );

    fmt::print("}};\n");

    if (cursor.kind == CXCursor_StructDecl 
        || cursor.kind == CXCursor_ClassDecl)
    {
        HandleStruct(cursor);
    }
}

void SearchNamespaceOrUnionOrStruct(CXCursor cursor)
{
    visitChildren(
        cursor,
        [](const CXCursor cursor) {
            if (cursor.kind != CXCursor_StructDecl 
                && cursor.kind != CXCursor_Namespace
                && cursor.kind != CXCursor_ClassDecl
                && cursor.kind != CXCursor_UnionDecl)
            {
                return;
            }

            CXSourceLocation loc = clang_getCursorLocation(cursor);
            bool systemHeader = clang_Location_isInSystemHeader(loc);
            if (systemHeader)
            {
                return;
            }

            if (cursor.kind == CXCursor_Namespace)
            {
                SearchNamespaceOrUnionOrStruct(cursor);
                return;
            }

            const bool is_definition = clang_isCursorDefinition(cursor);
            if (!is_definition)
            {
                return;
            }

            VisitUnionOrStruct(cursor);
        }
    );
}

void VisitTranslationUnit(CXTranslationUnit unit)
{
    fmt::print("#pragma once\n\n");
    fmt::print("#include <columns.h>\n");

    SearchNamespaceOrUnionOrStruct(
        clang_getTranslationUnitCursor(unit)
    );
}

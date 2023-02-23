#include <assert.h>
#include <fmt/format.h>

#include "visit.h"
#include "util.h"

void VisitStruct(CXCursor cursor);
void VisitStructField(CXCursor cursor, CXCursor parent);

void LineInfo(CXCursor cursor)
{
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    
    CXString name;
    unsigned int line = 0, col = 0;
    clang_getPresumedLocation(loc, &name, &line, &col);

    fmt::print(
        "// line {}:{}:{}\n",
        clang_getCString(name),
        line,
        col
    );
    
    clang_disposeString(name);
}

void HandleStructFieldNumber(CXCursor cursor, CXCursor parent)
{
    fmt::print(
        "    DEFINE_OBJECT_FIELD_NUMBER({}, {}),\n",
        GetTypeSpelling(clang_getCursorType(parent)),
        GetCursorDisplayName(cursor));
}

void HandleStructFieldArray(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXType elementType = clang_getArrayElementType(type);
    CXCursor elementTypeDecl = clang_getTypeDeclaration(
        clang_getCanonicalType(elementType));

    if (elementTypeDecl.kind != CXCursor_NoDeclFound)
    {
        fmt::print(
            "    DEFINE_OBJECT_FIELD_OBJECT_FIXED_ARRAY({}, {}, {}Object),\n",
            GetTypeSpelling(clang_getCursorType(parent)),
            GetCursorDisplayName(cursor),
            GetCursorDisplayName(elementTypeDecl));
    }
    else
    {
        fmt::print(
            "    DEFINE_OBJECT_FIELD_FIXED_ARRAY({}, {}),\n",
            GetTypeSpelling(clang_getCursorType(parent)),
            GetCursorDisplayName(cursor));
    }
}

void HandleStructFieldRecord(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXCursor elementTypeDecl = clang_getTypeDeclaration(
        clang_getCanonicalType(type));

    fmt::print(
        "    DEFINE_OBJECT_FIELD_OBJECT({}, {}, {}Object),\n",
        GetTypeSpelling(clang_getCursorType(parent)),
        GetCursorDisplayName(cursor),
        GetCursorDisplayName(elementTypeDecl));
}

void HandleStructField(CXCursor cursor, CXCursor parent)
{
    CXType type = clang_getCursorType(cursor);
    CXType type2 = clang_getCanonicalType(type);

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
            return HandleStructFieldNumber(cursor, parent);
        case CXType_ConstantArray:
            return HandleStructFieldArray(cursor, parent);
        case CXType_Record:
            return HandleStructFieldRecord(cursor, parent);
        default:
            fmt::print(
                "#error unsupported type {} {}\n",
                GetTypeSpelling(type2),
                GetCursorDisplayName(cursor));
            return ;
    }
}

void HandleStructBase(CXCursor cursor, CXCursor parent)
{
    visitChildren(
        cursor,
        VisitStructField,
        parent
    );
}

void VisitStructField(CXCursor cursor, CXCursor parent)
{
    switch (cursor.kind)
    {
        case CXCursor_CXXBaseSpecifier:
            return HandleStructBase(
                clang_getCursorDefinition(cursor),
                parent);
        case CXCursor_FieldDecl:
            return HandleStructField(cursor, parent);
        default:
            return ;
    }
}

bool CheckStructHasField(CXCursor cursor)
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
                        hasField = CheckStructHasField(cursor);
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

void VisitStruct(CXCursor cursor)
{
    auto name = GetCursorDisplayName(cursor);
    if (name.empty())
    {
        return;
    }

    const CXType type = clang_getCursorType(cursor);
    if (CheckStructHasField(cursor))
    {
        fmt::print("\n");

        LineInfo(cursor);

        fmt::print(
            "static const clColumn {}Columns[] = {{\n",
            name);

        visitChildren(
            cursor,
            VisitStructField,
            cursor
        );

        fmt::print("}};\n");
        fmt::print(
            "static const clColumn {}Object[] = {{\n",
            name);
        fmt::print(
            "    DEFINE_OBJECT({}, {}Columns)\n",
            GetTypeSpelling(type),
            name);
        fmt::print("}};\n");
    }
}

void SearchNamespaceOrStruct(CXCursor cursor)
{
    visitChildren(
        cursor,
        [](const CXCursor cursor) {
            if (cursor.kind != CXCursor_StructDecl 
                && cursor.kind != CXCursor_Namespace
                && cursor.kind != CXCursor_ClassDecl)
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
                SearchNamespaceOrStruct(cursor);
            }
            else
            {
                bool is_definition = clang_isCursorDefinition(cursor);
                if (!is_definition)
                {
                    return;
                }

                VisitStruct(cursor);
            }
        }
    );
}

void VisitTranslationUnit(CXTranslationUnit unit)
{
    fmt::print("#pragma once\n\n");
    fmt::print("#include <columns.h>\n");

    SearchNamespaceOrStruct(
        clang_getTranslationUnitCursor(unit)
    );
}

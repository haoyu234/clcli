#include <assert.h>
#include <fmt/format.h>

#include "visit.h"
#include "util.h"
#include "builder.h"

void VisitUnionOrStruct(Builder *builder, CXCursor cursor);
void VisitUnionOrStructField(Builder *builder, CXCursor cursor);

void LineError(CXCursor cursor)
{
    CXCursor definition = clang_getCursorDefinition(cursor);
    CXSourceLocation location = clang_getCursorLocation(cursor);

    CXString name;
    unsigned int line = 0, column = 0;
    clang_getPresumedLocation(location, &name, &line, &column);

    fmt::print(
        stderr,
        "{}:{}:{} message: not supported {}\n",
        clang_getCString(name),
        line,
        column,
        GetTypeSpelling(clang_getCursorType(definition)));

    clang_disposeString(name);
}

void HandleFieldArray(Builder *builder, CXCursor cursor)
{
    CXType type = clang_getCursorType(cursor);
    CXType elementType = clang_getArrayElementType(type);

    builder->DefineFixedArrayField(
        cursor,
        clang_getTypeDeclaration(
            clang_getCanonicalType(elementType))
    );
}

void HandleField(Builder *builder, CXCursor cursor)
{
    CXType type = clang_getCursorType(cursor);
    CXType canonicalType = clang_getCanonicalType(type);

    CXCursor declaration = clang_getTypeDeclaration(canonicalType);
    if (declaration.kind == CXCursor_StructDecl ||
        declaration.kind == CXCursor_UnionDecl ||
        declaration.kind == CXCursor_ClassDecl)
    {
        builder->DefineObjectField(cursor);
        return ;
    }

    auto name = GetCursorSpelling(cursor);
    if (name == "skins")
    {
        int i = 0;
    }

    switch (canonicalType.kind)
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
            builder->DefineNumberField(cursor);
            return ;
        case CXType_ConstantArray:
        case CXType_VariableArray:
            HandleFieldArray(builder, cursor);
            return ;
        default:
            LineError(cursor);
            return ;
    }
}

void HandleBaseClass(Builder *builder, CXCursor cursor)
{
    visitChildren(
        builder,
        cursor,
        VisitUnionOrStructField
    );
}

void VisitUnionOrStructField(Builder *builder, CXCursor cursor)
{
    switch (cursor.kind)
    {
        case CXCursor_CXXBaseSpecifier:
            HandleBaseClass(
                builder,
                clang_getCursorDefinition(cursor));
            return ;
        case CXCursor_FieldDecl:
            HandleField(
                builder,
                cursor);
            return ;
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
            LineError(cursor);
            return ;
        default:
            return ;
    }
}

void VisitUnionOrStruct(Builder *builder, CXCursor cursor)
{
    const auto name = GetCursorDisplayName(cursor);
    if (name.empty())
    {
        return;
    }

    builder->EnterObject(cursor);

    visitChildren(
        builder,
        cursor,
        VisitUnionOrStructField
    );

    builder->LeaveObject();
}

void SearchNamespaceOrUnionOrStruct(Builder *builder, CXCursor cursor)
{
    visitChildren(
        builder,
        cursor,
        [](Builder *builder, CXCursor cursor) {
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
                SearchNamespaceOrUnionOrStruct(builder, cursor);
                return;
            }

            const bool isDefinition = clang_isCursorDefinition(cursor);
            if (!isDefinition)
            {
                return;
            }

            VisitUnionOrStruct(builder, cursor);
        }
    );
}

void VisitTranslationUnit(Builder *builder, CXTranslationUnit unit)
{
    SearchNamespaceOrUnionOrStruct(
        builder,
        clang_getTranslationUnitCursor(unit)
    );
}

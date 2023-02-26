#pragma once

#include <clang-c/Index.h>

struct Builder
{
    virtual void EnterObject(CXCursor cursor) = 0;
    virtual void LeaveObject() = 0;

    virtual void DefineNumberField(CXCursor cursor) = 0;
    virtual void DefineFixedArrayField(CXCursor cursor, CXCursor element) = 0;
    virtual void DefineFlexableArrayField(CXCursor cursor, CXCursor element) = 0;
    virtual void DefineObjectField(CXCursor cursor) = 0;

    virtual void Output(FILE *pOut) = 0;

    virtual ~Builder() = default;
};

Builder *NewBuilder();
void FreeBuilder(Builder *builder);

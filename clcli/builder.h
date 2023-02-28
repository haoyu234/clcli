#pragma once

#include <string>
#include <clang-c/Index.h>

struct Builder
{
    virtual void EnterObject(CXCursor cursor) = 0;
    virtual void LeaveObject() = 0;

    virtual void DefineNumberField(CXCursor cursor) = 0;
    virtual void DefineArrayField(CXCursor cursor, CXCursor element) = 0;
    virtual void DefineObjectField(CXCursor cursor) = 0;

    virtual void Include(std::string name) = 0;

    virtual std::string GetSource() = 0;
    virtual std::string GetSourceHeader() = 0;

    virtual ~Builder() = default;
};

Builder *NewBuilder();
void FreeBuilder(Builder *builder);

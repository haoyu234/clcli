#pragma once

#include <functional>
#include <clang-c/Index.h>

struct Builder;

void VisitTranslationUnit(Builder *builder, CXTranslationUnit unit);

namespace internal
{
    template <class ...ARGS>
    struct Handler {
    private:
        std::function<void (ARGS...)> handler;
    public:

        template <typename F>
            Handler(F&& handler) : 
                handler(std::forward<F>(handler))
            {
                
            }

        static enum CXChildVisitResult visitChildrenHandler(
            CXCursor cursor,
            CXCursor parent,
            CXClientData context)
        {
            reinterpret_cast<Handler<ARGS...> *>(context)->handler(
                cursor
            );
            return CXChildVisit_Continue;
        }

        static enum CXVisitorResult visitFieldHandler(
            CXCursor cursor,
            CXClientData context)
        {
            reinterpret_cast<Handler<ARGS...> *>(context)->handler(
                cursor
            );
            return CXVisit_Continue;
        }

        static void visitInclusionHandler(
            CXFile includedFile,
            CXSourceLocation *inclusionStack,
            unsigned int includeLen,
            CXClientData context)
        {
            reinterpret_cast<Handler<ARGS...> *>(context)->handler(
                includeLen,
                includedFile
            );
        }
    };
}

template <typename T, typename ...ARGS>
void visitChildren(Builder *builder, CXCursor cursor, T&& handler, ARGS&& ...args)
{
    internal::Handler<CXCursor> callback(
        std::bind(
            std::forward<T>(handler),
            builder,
            std::placeholders::_1,
            std::forward<ARGS>(args)...)
    );

    clang_visitChildren(
        cursor,
        internal::Handler<CXCursor>::visitChildrenHandler,
        &callback
    );
}

template <typename T, typename ...ARGS>
void visitField(Builder *builder, CXType type, T&& handler, ARGS&& ...args)
{
    internal::Handler<CXCursor> callback(
        std::bind(
            std::forward<T>(handler),
            builder,
            std::placeholders::_1,
            std::forward<ARGS>(args)...)
    );

    clang_Type_visitFields(
        type,
        internal::Handler<CXCursor>::visitFieldHandler,
        &callback
    );
}

template <typename T, typename ...ARGS>
void visitInclusion(Builder *builder, CXTranslationUnit unit, T&& handler, ARGS&& ...args)
{
    internal::Handler<uint32_t, CXFile> callback(
        std::bind(
            std::forward<T>(handler),
            builder,
            std::placeholders::_1,
            std::placeholders::_2,
            std::forward<ARGS>(args)...)
    );

    clang_getInclusions(
        unit,
        internal::Handler<uint32_t, CXFile>::visitInclusionHandler,
        &callback
    );
}
#pragma once

#include <functional>
#include <clang-c/Index.h>

void VisitStruct(CXCursor cursor);
void VisitTranslationUnit(CXTranslationUnit unit);

namespace internal
{
    template <typename T>
    struct Handler {
    private:
        std::function<void (T)> handler;
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
            reinterpret_cast<Handler<T> *>(context)->handler(cursor);
            return CXChildVisit_Continue;
        }

        static enum CXVisitorResult visitFieldHandler(
            CXCursor cursor,
            CXClientData context)
        {
            reinterpret_cast<Handler<T> *>(context)->handler(cursor);
            return CXVisit_Continue;
        }
    };
}

template <typename T, typename ...ARGS>
void visitChildren(CXCursor cursor, T&& handler, ARGS&& ...args)
{
    internal::Handler<CXCursor> callback(
        std::bind(
            std::forward<T>(handler),
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
void visitField(CXType type, T&& handler, ARGS&& ...args)
{
    internal::Handler<CXCursor> callback(
        std::bind(
            std::forward<T>(handler),
            std::placeholders::_1,
            std::forward<ARGS>(args)...)
    );

    clang_Type_visitFields(
        type,
        internal::Handler<CXCursor>::visitFieldHandler,
        &callback
    );
}
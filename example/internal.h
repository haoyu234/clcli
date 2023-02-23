#pragma once

#define DEREF(ptr, type) (*((type *) (ptr)))

#define CHECK_COND_ERROR(ctx, cond) \
    do { \
        if (!(cond)) { \
            ctx->hasError = true; \
            return ; \
        } \
    } while(false)

#define CHECK_CTX_ERROR(ctx) \
    do { \
        if (ctx->hasError) { \
            return ; \
        } \
    } while(false)

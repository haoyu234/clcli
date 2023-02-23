#include <columns.h>
#include <msgpack.h>

#include "internal.h"

struct Encoder
{
    bool hasError;

    const uint8_t *baseAddr;
    ptrdiff_t offset;

    size_t capacity;
    size_t wpos;
    uint8_t *buf;

    msgpack_packer packer;
};

static inline int WriteBuf(void* context, const char* buf, size_t len)
{
    struct Encoder *encoder = (struct Encoder *) context;

    if (encoder->wpos + len > encoder->capacity)
    {
        return -1;
    }

    memcpy(
        encoder->buf + encoder->wpos,
        buf,
        len
    );

    encoder->wpos += len;
    return 0;
}

static inline uint32_t ReadArraySize(uint8_t kind, const void *addr)
{
    switch (kind)
    {
        case cl_COLUMN_INT8:
            return DEREF(addr, int8_t) > 0 ?
                DEREF(addr, int8_t):
                0;
        case cl_COLUMN_INT16:
            return DEREF(addr, int16_t) > 0 ?
                DEREF(addr, int16_t):
                0;
        case cl_COLUMN_INT32:
            return DEREF(addr, int32_t) > 0 ?
                DEREF(addr, int32_t):
                0;
        case cl_COLUMN_INT64:
            return DEREF(addr, int64_t) > 0 ?
                DEREF(addr, int64_t):
                0;
        case cl_COLUMN_UINT8:
            return DEREF(addr, uint8_t);
        case cl_COLUMN_UINT16:
            return DEREF(addr, uint16_t);
        case cl_COLUMN_UINT32:
            return DEREF(addr, uint32_t);
        case cl_COLUMN_UINT64:
            return DEREF(addr, uint64_t);
        default:
            assert(false);
    }
    return 0;
}

static inline void VisitNumber(
    const clHandler *handler, 
    uint8_t kind,
    struct Encoder *encoder)
{
    const ptrdiff_t offset = encoder->offset;

    switch (kind)
    {
        case cl_COLUMN_INT8:
            msgpack_pack_int8(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, int8_t));
            break ;
        case cl_COLUMN_INT16:
            msgpack_pack_int16(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, int16_t));
            break ;
        case cl_COLUMN_INT32:
            msgpack_pack_int32(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, int32_t));
            break ;
        case cl_COLUMN_INT64:
            msgpack_pack_int64(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, int64_t));
            break ;
        case cl_COLUMN_UINT8:
            msgpack_pack_uint8(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, uint8_t));
            break ;
        case cl_COLUMN_UINT16:
            msgpack_pack_uint16(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, uint16_t));
            break ;
        case cl_COLUMN_UINT32:
            msgpack_pack_uint32(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, uint32_t));
            break ;
        case cl_COLUMN_UINT64:
            msgpack_pack_uint64(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, uint64_t));
            break ;
        case cl_COLUMN_FLOAT32:
            msgpack_pack_float(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, float));
            break ;
        case cl_COLUMN_FLOAT64:
            msgpack_pack_double(
                &encoder->packer,
                DEREF(encoder->baseAddr + offset, double));
            break ;
        case cl_COLUMN_BOOL:
            DEREF(encoder->baseAddr + offset, bool) ?
                msgpack_pack_true(
                    &encoder->packer) :
                msgpack_pack_false(
                    &encoder->packer);
            break ;
        default:
            assert(false);
            break ;
    }
}

static inline void VisitArray(
    const clHandler *handler, 
    uint32_t num,
    uint8_t kind,
    const clColumn *element,
    struct Encoder *encoder)
{
    const ptrdiff_t offset = encoder->offset;

    CHECK_COND_ERROR(
        encoder,
        msgpack_pack_array(
            &encoder->packer,
            num) >= 0);

    uint32_t i = 0;

    if (element != NULL)
    {
        const uint32_t stride = element->size;

        for (i = 0; i < num; ++ i)
        {
            encoder->offset = offset + stride * i;

            clVisitChildren(
                handler, 
                element, 
                encoder);

            CHECK_CTX_ERROR(encoder);
        }
    }
    else
    {
        static uint32_t NUMBER_SIZES[] = {
            0, // cl_COLUMN_NONE
            sizeof(int8_t), // cl_COLUMN_INT8
            sizeof(int16_t), // cl_COLUMN_INT16
            sizeof(int32_t), // cl_COLUMN_INT32
            sizeof(int64_t), // cl_COLUMN_INT64
            0, // cl_COLUMN_INT128
            0, // cl_COLUMN_INT256
            sizeof(uint8_t), // cl_COLUMN_UINT8
            sizeof(uint16_t), // cl_COLUMN_UINT16
            sizeof(uint32_t), // cl_COLUMN_UINT32
            sizeof(uint64_t), // cl_COLUMN_UINT64
            0, // cl_COLUMN_UINT128
            0, // cl_COLUMN_UINT256
            0, // cl_COLUMN_FLOAT8
            0, // cl_COLUMN_FLOAT16
            #ifdef __HAVE_FLOAT32
                sizeof(_Float32), // cl_COLUMN_FLOAT32
            #else
                4,
            #endif
            #ifdef __HAVE_FLOAT64
                sizeof(_Float64), // cl_COLUMN_FLOAT64
            #else
                8,
            #endif
            0, // cl_COLUMN_FLOAT128
            0, // cl_COLUMN_FLOAT256,
            sizeof(bool), // cl_COLUMN_BOOL
        };

        assert(kind < sizeof(NUMBER_SIZES) / sizeof(NUMBER_SIZES[0]));
        assert(NUMBER_SIZES[kind] > 0);

        for (i = 0; i < num; ++ i)
        {
            encoder->offset = offset + NUMBER_SIZES[kind] * i;

            VisitNumber(
                handler, 
                kind,
                encoder);

            CHECK_CTX_ERROR(encoder);
        }
    }
}

static void VisitNumberHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Encoder *encoder)
{
    VisitNumber(
        handler,
        column->kind,
        encoder);
}

static inline void VisitObjectHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Encoder *encoder)
{
    const ptrdiff_t offset = encoder->offset;

    const uint32_t num = column->object.num;
    const clColumn *columns = column->object.element;

    CHECK_COND_ERROR(
        encoder,
        msgpack_pack_array(
            &encoder->packer,
            num) >= 0);

    uint32_t i = 0;

    for (i = 0; i < num; ++ i)
    {
        encoder->offset = offset + columns[i].offset;

        clVisitChildren(
            handler, 
            columns + i,
            encoder);

        CHECK_CTX_ERROR(encoder);
    }
}

static inline void VisitFixedArrayHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Encoder *encoder)
{
    VisitArray(
        handler,
        column->fixedArray.capacity,
        column->fixedArray.flags,
        column->fixedArray.element,
        encoder);
}

static inline void VisitFlexibleArrayHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Encoder *encoder)
{
    const ptrdiff_t offset = encoder->offset;

    const clColumn *prev_column = column - 1;

    const uint32_t num = ReadArraySize(
        prev_column->kind, 
        encoder->baseAddr + offset + prev_column->offset - column->offset);

    VisitArray(
        handler,
        num,
        column->flexibleArray.flags,
        column->flexibleArray.element,
        encoder);
}

static struct clHandler handler = {
    .visitNumber = (clVisitHandler) VisitNumberHandler,
    .visitObject = (clVisitHandler) VisitObjectHandler,
    .visitFixedArray = (clVisitHandler) VisitFixedArrayHandler,
    .visitFlexibleArray = (clVisitHandler) VisitFlexibleArrayHandler,
};

size_t cToBuf(
    const clColumn *column, 
    const void *baseAddr, 
    uint8_t *buf, 
    size_t size)
{
    struct Encoder encoder = {
        .hasError = false,
        .baseAddr = (const uint8_t *) baseAddr,
        .offset = 0,
        .capacity = size,
        .wpos = 0,
        .buf = buf
    };

    msgpack_packer_init(
        &encoder.packer, 
        &encoder, 
        WriteBuf);

    clVisitChildren(
        &handler,
        column,
        &encoder);

    return !encoder.hasError ?
        encoder.wpos :
        0;
}

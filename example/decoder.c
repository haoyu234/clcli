#include <columns.h>
#include <msgpack.h>

#include "internal.h"

struct Decoder
{
    bool hasError;

    uint8_t *baseAddr;
    ptrdiff_t offset;

    msgpack_object *object;
};

static inline void WriteArraySize(uint8_t kind, const void *addr, uint32_t size)
{
    switch (kind)
    {
        case cl_COLUMN_INT8:
            DEREF(addr, int8_t) = size;
            break;
        case cl_COLUMN_INT16:
            DEREF(addr, int16_t) = size;
            break;
        case cl_COLUMN_INT32:
            DEREF(addr, int32_t) = size;
            break;
        case cl_COLUMN_INT64:
            DEREF(addr, int64_t) = size;
            break;
        case cl_COLUMN_UINT8:
            DEREF(addr, uint8_t) = size;
            break;
        case cl_COLUMN_UINT16:
            DEREF(addr, uint16_t) = size;
            break;
        case cl_COLUMN_UINT32:
            DEREF(addr, uint32_t) = size;
            break;
        case cl_COLUMN_UINT64:
            DEREF(addr, uint64_t) = size;
            break;
        default:
            assert(false);
    }
}

static inline void VisitNumber(
    const clHandler *handler, 
    uint8_t kind,
    struct Decoder *decoder)
{
    const msgpack_object *object = decoder->object;
    const ptrdiff_t offset = decoder->offset;

    CHECK_COND_ERROR(
        decoder,
        object->type == MSGPACK_OBJECT_POSITIVE_INTEGER 
        || object->type == MSGPACK_OBJECT_NEGATIVE_INTEGER
        || object->type == MSGPACK_OBJECT_BOOLEAN
        || object->type == MSGPACK_OBJECT_FLOAT32
        || object->type == MSGPACK_OBJECT_FLOAT64
        || object->type == MSGPACK_OBJECT_FLOAT);

    switch (kind)
    {
        case cl_COLUMN_INT8:
            DEREF(decoder->baseAddr + offset, int8_t) = object->via.i64;
            break ;
        case cl_COLUMN_INT16:
            DEREF(decoder->baseAddr + offset, int16_t) = object->via.i64;
            break ;
        case cl_COLUMN_INT32:
            DEREF(decoder->baseAddr + offset, int32_t) = object->via.i64;
            break ;
        case cl_COLUMN_INT64:
            DEREF(decoder->baseAddr + offset, int64_t) = object->via.i64;
            break ;
        case cl_COLUMN_UINT8:
            DEREF(decoder->baseAddr + offset, uint8_t) = object->via.u64;
            break ;
        case cl_COLUMN_UINT16:
            DEREF(decoder->baseAddr + offset, uint16_t) = object->via.u64;
            break ;
        case cl_COLUMN_UINT32:
            DEREF(decoder->baseAddr + offset, uint32_t) = object->via.u64;
            break ;
        case cl_COLUMN_UINT64:
            DEREF(decoder->baseAddr + offset, uint64_t) = object->via.u64;
            break ;
        case cl_COLUMN_FLOAT32:
            DEREF(decoder->baseAddr + offset, float) = object->via.f64;
            break;
        case cl_COLUMN_FLOAT64:
            DEREF(decoder->baseAddr + offset, double) = object->via.f64;
            break;
        case cl_COLUMN_BOOL:
            DEREF(decoder->baseAddr + offset, bool) = object->via.boolean;
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
    struct Decoder *decoder)
{
    const msgpack_object *object = decoder->object;
    const ptrdiff_t offset = decoder->offset;
    
    uint32_t i = 0;

    if (element != NULL)
    {
        const uint32_t stride = element->size;

        for (i = 0; i < num; ++ i)
        {
            decoder->object = object->via.array.ptr + i;
            decoder->offset = offset + stride * i;

            clVisitChildren(
                handler, 
                element, 
                decoder);

            CHECK_CTX_ERROR(decoder);
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
            decoder->object = object->via.array.ptr + i;
            decoder->offset = offset + NUMBER_SIZES[kind] * i;

            VisitNumber(
                handler, 
                kind,
                decoder);

            CHECK_CTX_ERROR(decoder);
        }
    }
}

static void VisitNumberHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Decoder *decoder)
{
    VisitNumber(
        handler,
        column->kind,
        decoder);
}

static inline void VisitObjectHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Decoder *decoder)
{
    const msgpack_object *object = decoder->object;
    const ptrdiff_t offset = decoder->offset;

    CHECK_COND_ERROR(
        decoder,
        object->type == MSGPACK_OBJECT_ARRAY
        && object->via.array.size == column->object.num);

    const uint32_t num = column->object.num;
    const clColumn *columns = column->object.element;

    uint32_t i = 0;

    for (i = 0; i < num; ++ i)
    {
        decoder->object = object->via.array.ptr + i;
        decoder->offset = offset + columns[i].offset;

        clVisitChildren(
            handler, 
            columns + i, 
            decoder);

        CHECK_CTX_ERROR(decoder);
    }
}

static inline void VisitFixedArrayHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Decoder *decoder)
{
    const msgpack_object *object = decoder->object;

    CHECK_COND_ERROR(
        decoder,
        object->type == MSGPACK_OBJECT_ARRAY
        && object->via.array.size == column->fixedArray.capacity);

    VisitArray(
        handler,
        column->fixedArray.capacity,
        column->fixedArray.flags,
        column->fixedArray.element,
        decoder);
}

static inline void VisitFlexibleArrayHandler(
    const clHandler *handler, 
    const clColumn *column, 
    struct Decoder *decoder)
{
    const msgpack_object *object = decoder->object;
    const ptrdiff_t offset = decoder->offset;

    CHECK_COND_ERROR(
        decoder,
        object->type == MSGPACK_OBJECT_ARRAY);
    
    const clColumn *prev_column = column - 1;

    WriteArraySize(
        prev_column->kind, 
        decoder->baseAddr + offset + prev_column->offset - column->offset,
        object->via.array.size);

    VisitArray(
        handler,
        object->via.array.size,
        column->flexibleArray.flags,
        column->flexibleArray.element,
        decoder);
}

static struct clHandler handler = {
    .visitNumber = (clVisitHandler) VisitNumberHandler,
    .visitObject = (clVisitHandler) VisitObjectHandler,
    .visitFixedArray = (clVisitHandler) VisitFixedArrayHandler,
    .visitFlexibleArray = (clVisitHandler) VisitFlexibleArrayHandler,
};

size_t cFromBuf(
    const clColumn *column, 
    void *baseAddr, 
    const uint8_t *buf, 
    size_t size)
{
    msgpack_unpacked unpacked;
    msgpack_unpacked_init(&unpacked);

    size_t offset = 0;
    msgpack_unpack_return ret = msgpack_unpack_next(
        &unpacked, 
        (const char *) buf, 
        size, 
        &offset);

    struct Decoder decoder = {
        .hasError = (ret != MSGPACK_UNPACK_SUCCESS),
        .baseAddr = (uint8_t *) baseAddr,
        .offset = 0,
        .object = &unpacked.data
    };

    if (!decoder.hasError) 
    {
        clVisitChildren(
            &handler,
            column,
            &decoder);
    }

    msgpack_unpacked_destroy(
        &unpacked);

    return !decoder.hasError ?
        offset:
        0;
}

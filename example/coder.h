#pragma once

#include <columns.h>

size_t cToBuf(
    const clColumn *column,
    const void *addr,
    size_t size,
    uint8_t *buf,
    size_t len);

size_t cFromBuf(
    const clColumn *column,
    void *addr,
    size_t size,
    const uint8_t *buf,
    size_t len);

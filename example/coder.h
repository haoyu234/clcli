#pragma once

#include <columns.h>

size_t cToBuf(const clColumn *column, const void *baseAddr, uint8_t *buf, size_t size);
size_t cFromBuf(const clColumn *column, void *baseAddr, const uint8_t *buf, size_t size);

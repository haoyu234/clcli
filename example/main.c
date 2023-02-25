#include <stdio.h>
#include <string.h>
#include <columns.h>

#include "coder.h"
#include "generated.h"

struct stJson json = {
    .kind = 3,
    .via = {
        .f64 = 3.14159265358979323846
    },
};

int runExample(
    const clColumn *column, 
    void *object,
    uint32_t size)
{
    char object2[size];
    uint8_t buf[size * 2];

    const size_t size1 = cToBuf(column, object, size, buf, sizeof(buf));
    const size_t size2 = cFromBuf(column, object2, size, buf, sizeof(buf));

    printf("size1[%lu] size2[%lu].\n", size1, size2);
    return size1 - size2;
}

int main()
{
    return runExample(
        stJsonObject,
        &json,
        sizeof(json));
}


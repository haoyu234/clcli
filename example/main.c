#include <assert.h>
#include <columns.h>
#include <string.h>
#include <stdio.h>

#include "coder.h"
#include "generated.h"

int main()
{
    uint8_t buf[1024];

    uv_statfs_t cObject1 = {};
    uv_statfs_t cObject2 = {
        1,2,3,4,5,6,7,{8,9,10,11}
    };

    const size_t size1 = cToBuf(uv_statfs_sObject, &cObject2, buf, sizeof(buf));
    const size_t size2 = cFromBuf(uv_statfs_sObject, &cObject1, buf, sizeof(buf));
    assert(size1 == size2);

    const bool same = !memcmp(&cObject2, &cObject1, sizeof(uv_statfs_t));
    assert(same);
    return 0;
}


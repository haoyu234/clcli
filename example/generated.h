#pragma once

#include <columns.h>

union stJsonUnion
{
  int64_t i64;
  uint64_t u64;
  double f64;
};

struct stJson
{
  uint8_t kind;
  union stJsonUnion via;
};

//line example/generated.h:5:7
static const clColumn stJsonUnionColumns[] = {
    DEFINE_COLUMN_NUMBER(union stJsonUnion, i64),
    DEFINE_COLUMN_NUMBER(union stJsonUnion, u64),
    DEFINE_COLUMN_NUMBER(union stJsonUnion, f64),
};

//line example/generated.h:12:8
static const clColumn stJsonColumns[] = {
    DEFINE_COLUMN_NUMBER(struct stJson, kind),
    DEFINE_COLUMN_UNION(struct stJson, via, stJsonUnionColumns),
};
static const clColumn stJsonObject[] = {
    DEFINE_OBJECT(struct stJson, stJsonColumns)
};

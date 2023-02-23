#pragma once

#include <columns.h>
  
typedef struct uv_statfs_s uv_statfs_t;
struct uv_statfs_s {
  uint64_t f_type;
  uint64_t f_bsize;
  uint64_t f_blocks;
  uint64_t f_bfree;
  uint64_t f_bavail;
  uint64_t f_files;
  uint64_t f_ffree;
  uint64_t f_spare[4];
};

// ./example/generated.h:6:8
static const clColumn uv_statfs_sColumns[] = {
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_type),
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_bsize),
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_blocks),
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_bfree),
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_bavail),
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_files),
    DEFINE_OBJECT_FIELD_NUMBER(struct uv_statfs_s, f_ffree),
    DEFINE_OBJECT_FIELD_FIXED_ARRAY(struct uv_statfs_s, f_spare),
};
static const clColumn uv_statfs_sObject[] = {
    DEFINE_OBJECT(struct uv_statfs_s, uv_statfs_sColumns)
};

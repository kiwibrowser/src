/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_METADATA_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_METADATA_H__

#include <stddef.h>
#include <time.h>

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Note: this values in this enum are written to the cache, so changing them
 * will implicitly invalidate cache entries.
 */
typedef enum NaClCodeIdentityType {
  NaClCodeIdentityData = 0,
  NaClCodeIdentityFile = 1,
  NaClCodeIdentityMax
} NaClCodeIdentityType;

/*
 * file_name is not guaranteed to be a null-terminated string, its length is
 * explicitly specified by file_name_length.  By convention, if file_name
 * happens to be a null-terminated string, file_name_length equals
 * strlen(file_name).  In other words, the terminating null character is not
 * part of the name.
 */
struct NaClValidationMetadata {
  NaClCodeIdentityType identity_type;
  int64_t code_offset;
  char* file_name;
  size_t file_name_length;
  uint64_t device_id;
  uint64_t file_id;
  int64_t file_size;
  time_t mtime;
  time_t ctime;
};

/* Note: copies file_name.  This copy is deallocated by DestroyMetadata. */
extern void NaClMetadataFromFDCtor(struct NaClValidationMetadata *metadata,
                                   int file_desc,
                                   const char* file_name,
                                   size_t file_name_length);

extern
void NaClMetadataFromNaClDescCtor(struct NaClValidationMetadata *metadata,
                                  struct NaClDesc *desc);

extern void NaClMetadataDtor(struct NaClValidationMetadata *metadata);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_VALIDATION_METADATA_H__ */

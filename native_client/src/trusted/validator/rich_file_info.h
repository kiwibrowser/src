/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RICH_FILE_INFO_C_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RICH_FILE_INFO_C_H__

#include <stddef.h>

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClDesc;

struct NaClRichFileInfo {
  uint8_t known_file;
  const char *file_path;
  uint32_t file_path_length;
};

void NaClRichFileInfoCtor(struct NaClRichFileInfo *info);
void NaClRichFileInfoDtor(struct NaClRichFileInfo *info);

int NaClSetFileOriginInfo(struct NaClDesc *desc, struct NaClRichFileInfo *info);
int NaClGetFileOriginInfo(struct NaClDesc *desc, struct NaClRichFileInfo *info);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_RICH_FILE_INFO_C_H__ */

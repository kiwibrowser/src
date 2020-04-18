// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory.h>
#include <stddef.h>
#include <stdint.h>

#include "third_party/minizip/src/ioapi.h"
#include "third_party/minizip/src/ioapi_mem.h"
#include "third_party/minizip/src/unzip.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  zlib_filefunc_def filefunc32 = {};
  ourmemory_t unzmem = {};
  unzFile handle;
  unzmem.size = size;
  unzmem.base = reinterpret_cast<char*>(const_cast<uint8_t*>(data));
  unzmem.grow = 0;
  int err = UNZ_OK;
  unz_file_info64 file_info = {};
  char filename_inzip[256] = {};

  fill_memory_filefunc(&filefunc32, &unzmem);
  handle = unzOpen2(nullptr, &filefunc32);
  err = unzGoToFirstFile(handle);
  while (err == UNZ_OK) {
    err = unzGetCurrentFileInfo64(handle, &file_info, filename_inzip,
                                  sizeof(filename_inzip), NULL, 0, NULL, 0);
    if (err != UNZ_OK) {
      break;
    }
    err = unzOpenCurrentFile(handle);
    if (err != UNZ_OK) {
      break;
    }

    unzCloseCurrentFile(handle);
    err = unzGoToNextFile(handle);
  }
  unzClose(handle);

  return 0;
}

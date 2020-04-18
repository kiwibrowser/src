// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory.h>
#include <stdlib.h>
#include <string>

#include "third_party/minizip/src/ioapi.h"
#include "third_party/minizip/src/ioapi_mem.h"
#include "third_party/minizip/src/zip.h"

const char kTestFileName[] = "test.zip";
const zip_fileinfo kZipFileInfo = {};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  zlib_filefunc_def filefunc32 = {};
  ourmemory_t zmem = {};
  zmem.grow = 1;

  fill_memory_filefunc(&filefunc32, &zmem);

  zipFile zip_file = zipOpen2(nullptr /* pathname */, APPEND_STATUS_CREATE,
                              nullptr /* global comment */, &filefunc32);

  if (zip_file) {
    int open_result = zipOpenNewFileInZip(
        zip_file, kTestFileName, &kZipFileInfo, nullptr /* local extra field */,
        0u /* local extra field size*/, nullptr /* global extra field */,
        0u /* global extra field size */, nullptr /* comment */, Z_DEFLATED,
        Z_DEFAULT_COMPRESSION);

    if (open_result == ZIP_OK) {
      zipWriteInFileInZip(zip_file, data, size);
      zipCloseFileInZip(zip_file);
    }

    zipClose(zip_file, nullptr /* global comment */);
  }

  free(zmem.base);

  return 0;
}

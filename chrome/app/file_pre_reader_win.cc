// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/file_pre_reader_win.h"

#include <windows.h>

#include "base/files/file.h"

void PreReadFile(const base::FilePath& file_path) {
  base::File file(file_path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                                 base::File::FLAG_SEQUENTIAL_SCAN);
  if (!file.IsValid())
    return;

  // This could be replaced with ::PrefetchVirtualMemory once we drop support
  // for Win7. The performance of ::PrefetchVirtualMemory is roughly equivalent
  // to these buffered reads.
  const DWORD kStepSize = 1024 * 1024;
  char* buffer = reinterpret_cast<char*>(
      ::VirtualAlloc(nullptr, kStepSize, MEM_COMMIT, PAGE_READWRITE));
  if (!buffer)
    return;

  while (file.ReadAtCurrentPos(buffer, kStepSize) > 0) {}

  ::VirtualFree(buffer, 0, MEM_RELEASE);
}

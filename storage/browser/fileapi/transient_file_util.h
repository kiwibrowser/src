// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_TRANSIENT_FILE_UTIL_H_
#define STORAGE_BROWSER_FILEAPI_TRANSIENT_FILE_UTIL_H_

#include <memory>

#include "base/macros.h"
#include "storage/browser/fileapi/local_file_util.h"
#include "storage/browser/storage_browser_export.h"

namespace storage {

class FileSystemOperationContext;

class STORAGE_EXPORT TransientFileUtil : public LocalFileUtil {
 public:
  TransientFileUtil() {}
  ~TransientFileUtil() override {}

  // LocalFileUtil overrides.
  storage::ScopedFile CreateSnapshotFile(
      FileSystemOperationContext* context,
      const FileSystemURL& url,
      base::File::Error* error,
      base::File::Info* file_info,
      base::FilePath* platform_path) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TransientFileUtil);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_TRANSIENT_FILE_UTIL_H_

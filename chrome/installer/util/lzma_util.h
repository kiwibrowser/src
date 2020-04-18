// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_LZMA_UTIL_H_
#define CHROME_INSTALLER_UTIL_LZMA_UTIL_H_

#include <windows.h>

#include <set>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/string16.h"

// The error status of LzmaUtil::Unpack which is used to publish metrics. Do not
// change the order.
enum UnPackStatus {
  UNPACK_NO_ERROR = 0,
  UNPACK_ARCHIVE_NOT_FOUND = 1,
  UNPACK_ARCHIVE_CANNOT_OPEN = 2,
  UNPACK_SZAREX_OPEN_ERROR = 3,
  UNPACK_EXTRACT_ERROR = 4,
  UNPACK_EXTRACT_EXCEPTION = 5,
  UNPACK_NO_FILENAME_ERROR = 6,
  UNPACK_CREATE_FILE_ERROR = 7,
  UNPACK_WRITE_FILE_ERROR = 8,
  UNPACK_SET_FILE_TIME_ERROR = 9,
  UNPACK_CLOSE_FILE_ERROR = 10,
  UNPACK_STATUS_COUNT,
};

// Unpacks the contents of |archive| into |output_dir|. |output_file|, if not
// null, is populated with the name of the last (or only) member extracted from
// the archive. Returns ERROR_SUCCESS on success. Otherwise, returns a Windows
// error code and populates |unpack_status| (if not null) with a status value
// indicating the operation that failed.
DWORD UnPackArchive(const base::FilePath& archive,
                    const base::FilePath& output_dir,
                    base::FilePath* output_file,
                    UnPackStatus* unpack_status,
                    int32_t* ntstatus);

// A utility class that wraps LZMA SDK library. Prefer UnPackArchive over using
// this class directly.
class LzmaUtilImpl {
 public:
  LzmaUtilImpl();
  ~LzmaUtilImpl();

  DWORD OpenArchive(const base::FilePath& archivePath);

  // Unpacks the archive to the given location
  DWORD UnPack(const base::FilePath& location);

  void CloseArchive();

  // Unpacks the archive to the given location and returns the last file
  // extracted from archive.
  DWORD UnPack(const base::FilePath& location, base::FilePath* output_file);

  UnPackStatus GetUnPackStatus() { return unpack_status_; }
  int32_t GetNTSTATUSCode() { return ntstatus_; }

 protected:
  bool CreateDirectory(const base::FilePath& dir);

 private:
  HANDLE archive_handle_ = nullptr;
  std::set<base::string16> directories_created_;
  UnPackStatus unpack_status_ = UNPACK_NO_ERROR;
  // Can't include ntstatus.h as it's conflicted with winnt.h
  int32_t ntstatus_ = 0;  // STATUS_SUCCESS.

  DISALLOW_COPY_AND_ASSIGN(LzmaUtilImpl);
};

#endif  // CHROME_INSTALLER_UTIL_LZMA_UTIL_H_

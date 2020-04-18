// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the common variables shared between the fake
// implementation of the libarchive API and the other test files.

#ifndef CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_UNPACKER_TEST_CPP_FAKE_LIB_ARCHIVE_H_
#define CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_UNPACKER_TEST_CPP_FAKE_LIB_ARCHIVE_H_

#include <limits>

#include "archive.h"

// Define variables used to control the flow of the libarchive API.
namespace fake_lib_archive_config {

// A fake error returned by libarchive in case of failures.
const char kArchiveError[] = "An archive error.";

// A fake path name for libarchive entries.
const char kPathName[] = "path/to/file";  // Archives contain paths
                                          // without root "/".

// The fake size for libarchive entries. Bigger than int32_t.
const int64_t kSize = std::numeric_limits<int64_t>::max() - 50;

// The fake modification time for libarchive entries.
const time_t kModificationTime = 500;

// The data returned by archive_read_data. The pointer can be changed to other
// addresses. In case of NULL archive_read_data returns failure.
// archive_data should have archive_data_size available bytes in memory,
// else unexpected behavior can occur.
// By default it is set to NULL.
extern const char* archive_data;

// The size of archive_data.
// By default it is set to 0, which forces failure for archive_read_data.
extern int64_t archive_data_size;

// Bool variables used to force failure responses for libarchive API.
// By default all should be set to false.
extern bool fail_archive_read_new;
extern bool fail_archive_rar_support;
extern bool fail_archive_zip_seekable_support;
extern bool fail_archive_set_read_callback;
extern bool fail_archive_set_skip_callback;
extern bool fail_archive_set_seek_callback;
extern bool fail_archive_set_close_callback;
extern bool fail_archive_set_passphrase_callback;
extern bool fail_archive_set_callback_data;
extern bool fail_archive_read_open;
extern bool fail_archive_read_free;
extern bool fail_archive_set_options;

// Return value for archive_read_next_header.
// By default it should be set to ARCHIVE_OK.
extern int archive_read_next_header_return_value;

// Return value for archive_read_seek_header.
// By default it should be set to ARCHIVE_OK.
extern int archive_read_seek_header_return_value;

// Return value for archive_entry_filetype.
// By default it should be set to regular file.
extern mode_t archive_entry_filetype_return_value;

// Resets all variables to default values.
void ResetVariables();

}  // namespace fake_lib_archive_config

#endif  // CHROME_BROWSER_RESOURCES_CHROMEOS_ZIP_ARCHIVER_UNPACKER_TEST_CPP_FAKE_LIB_ARCHIVE_H_

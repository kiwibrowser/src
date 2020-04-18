// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fake_lib_archive.h"

#include <algorithm>
#include <cstring>

#include "archive_entry.h"
#include "ppapi/cpp/logging.h"
#include "testing/gtest/gtest.h"

// Define fake libarchive API functions. Tests are not linked with libarchive
// library in order to control the flow of the API functions inside the unit
// tests.

// Define libarchive structures. libarchive headers only declare them,
// but do not define it.
struct archive {
  // Used by archive_read_data to know how many bytes were read from
  // fake_lib_archive_config::kArchiveData during last call.
  int64_t data_offset;
};

struct archive_entry {
  // Content not needed.
};

namespace {

// The archive returned by archive_read_new in case of success.
archive test_archive;
archive_entry test_archive_entry;

}  // namespace

// Initialize the variables from fake_lib_archive_config namespace defined in
// fake_lib_archive.h.
namespace fake_lib_archive_config {

const char* archive_data = NULL;
int64_t archive_data_size = 0;

// By default libarchive API functions will return success.
bool fail_archive_read_new = false;
bool fail_archive_rar_support = false;
bool fail_archive_zip_seekable_support = false;
bool fail_archive_set_read_callback = false;
bool fail_archive_set_skip_callback = false;
bool fail_archive_set_seek_callback = false;
bool fail_archive_set_close_callback = false;
bool fail_archive_set_passphrase_callback = false;
bool fail_archive_set_callback_data = false;
bool fail_archive_read_open = false;
bool fail_archive_read_free = false;
bool fail_archive_set_options = false;

int archive_read_next_header_return_value = ARCHIVE_OK;
int archive_read_seek_header_return_value = ARCHIVE_OK;
mode_t archive_entry_filetype_return_value = S_IFREG;  // Regular file.

void ResetVariables() {
  archive_data = NULL;
  archive_data_size = 0;

  fail_archive_read_new = false;
  fail_archive_rar_support = false;
  fail_archive_zip_seekable_support = false;
  fail_archive_set_read_callback = false;
  fail_archive_set_skip_callback = false;
  fail_archive_set_seek_callback = false;
  fail_archive_set_close_callback = false;
  fail_archive_set_passphrase_callback = false;
  fail_archive_set_callback_data = false;
  fail_archive_read_open = false;
  fail_archive_read_free = false;
  fail_archive_set_options = false;

  archive_read_next_header_return_value = ARCHIVE_OK;
  archive_entry_filetype_return_value = S_IFREG;
}

}  // namespace fake_lib_archive_config

archive* archive_read_new() {
  test_archive.data_offset = 0;  // Reset data_offset.
  return fake_lib_archive_config::fail_archive_read_new ? NULL : &test_archive;
}

const char* archive_error_string(archive* archive_object) {
  return fake_lib_archive_config::kArchiveError;
}

void archive_set_error(struct archive*, int error_code, const char* fmt, ...) {
  // Nothing to do.
}

int archive_read_support_format_rar(archive* archive_object) {
  return fake_lib_archive_config::fail_archive_rar_support ? ARCHIVE_FATAL
                                                           : ARCHIVE_OK;
}

int archive_read_support_format_zip_seekable(archive* archive_object) {
  return fake_lib_archive_config::fail_archive_zip_seekable_support
             ? ARCHIVE_FATAL
             : ARCHIVE_OK;
}

int archive_read_set_read_callback(archive* archive_object,
                                   archive_read_callback* client_reader) {
  return fake_lib_archive_config::fail_archive_set_read_callback ? ARCHIVE_FATAL
                                                                 : ARCHIVE_OK;
}

int archive_read_set_skip_callback(archive* archive_object,
                                   archive_skip_callback* client_skipper) {
  return fake_lib_archive_config::fail_archive_set_skip_callback ? ARCHIVE_FATAL
                                                                 : ARCHIVE_OK;
}

int archive_read_set_seek_callback(archive* archive_object,
                                   archive_seek_callback* client_seeker) {
  return fake_lib_archive_config::fail_archive_set_seek_callback ? ARCHIVE_FATAL
                                                                 : ARCHIVE_OK;
}

int archive_read_set_close_callback(archive* archive_object,
                                    archive_close_callback* client_closer) {
  return fake_lib_archive_config::fail_archive_set_close_callback
             ? ARCHIVE_FATAL
             : ARCHIVE_OK;
}

int archive_read_set_passphrase_callback(
    archive* archive_object,
    void* client_data,
    archive_passphrase_callback* client_passphraser) {
  return fake_lib_archive_config::fail_archive_set_passphrase_callback
             ? ARCHIVE_FATAL
             : ARCHIVE_OK;
}

int archive_read_set_callback_data(archive* archive_object, void* client_data) {
  return fake_lib_archive_config::fail_archive_set_callback_data ? ARCHIVE_FATAL
                                                                 : ARCHIVE_OK;
}

int archive_read_open1(archive* archive_object) {
  return fake_lib_archive_config::fail_archive_read_open ? ARCHIVE_FATAL
                                                         : ARCHIVE_OK;
}

int archive_read_next_header(archive* archive_object, archive_entry** entry) {
  *entry = &test_archive_entry;
  return fake_lib_archive_config::archive_read_next_header_return_value;
}

int archive_read_seek_header(archive* archive_object, size_t index) {
  return fake_lib_archive_config::archive_read_seek_header_return_value;
}

const char* archive_entry_pathname(archive_entry* entry) {
  return fake_lib_archive_config::kPathName;
}

int64_t archive_entry_size(archive_entry* entry) {
  return fake_lib_archive_config::kSize;
}

time_t archive_entry_mtime(archive_entry* entry) {
  return fake_lib_archive_config::kModificationTime;
}

mode_t archive_entry_filetype(archive_entry* entry) {
  return fake_lib_archive_config::archive_entry_filetype_return_value;
}

int archive_read_free(archive* archive_object) {
  return fake_lib_archive_config::fail_archive_read_free ? ARCHIVE_FATAL
                                                         : ARCHIVE_OK;
}

int archive_read_set_options(archive* archive_object, const char* options) {
  return fake_lib_archive_config::fail_archive_set_options ? ARCHIVE_FATAL
                                                           : ARCHIVE_OK;
}

ssize_t archive_read_data(archive* archive_object,
                          void* buffer,
                          size_t length) {
  // TODO(cmihail): Instead of returning the archive data directly here use the
  // callback function set with archive_read_set_read_callback.
  // See crbug.com/415871.
  int64_t archive_data_size = fake_lib_archive_config::archive_data_size;
  if (fake_lib_archive_config::archive_data == NULL || archive_data_size == 0)
    return ARCHIVE_FATAL;

  PP_DCHECK(archive_data_size >= archive_object->data_offset);

  int64_t read_bytes = std::min(archive_data_size - archive_object->data_offset,
                                static_cast<int64_t>(length));
  PP_DCHECK(archive_data_size >= read_bytes);
  PP_DCHECK(read_bytes <
            static_cast<int64_t>(std::numeric_limits<ssize_t>::max()));

  // Copy data content.
  const char* source =
      fake_lib_archive_config::archive_data + archive_object->data_offset;
  PP_DCHECK(archive_data_size >= archive_object->data_offset + read_bytes);
  memcpy(buffer, source, read_bytes);

  archive_object->data_offset += read_bytes;
  return read_bytes;
}

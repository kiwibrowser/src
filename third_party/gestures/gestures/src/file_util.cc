// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gestures/include/file_util.h"

#include <fcntl.h>
#include <limits>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gestures/include/eintr_wrapper.h"

namespace gestures {

FILE* OpenFile(const char* filename, const char* mode) {
  FILE* result = NULL;
  do {
    result = fopen(filename, mode);
  } while (!result && errno == EINTR);
  return result;
}

bool CloseFile(FILE* file) {
  if (file == NULL)
    return true;
  return fclose(file) == 0;
}

bool ReadFileToString(const char* path,
                      std::string* contents,
                      size_t max_size) {
  if (contents)
    contents->clear();
  FILE* file = OpenFile(path, "rb");
  if (!file) {
    return false;
  }

  char buf[1 << 16];
  size_t len;
  size_t size = 0;
  bool read_status = true;

  // Many files supplied in |path| have incorrect size (proc files etc).
  // Hence, the file is read sequentially as opposed to a one-shot read.
  while ((len = fread(buf, 1, sizeof(buf), file)) > 0) {
    if (contents)
      contents->append(buf, std::min(len, max_size - size));

    if ((max_size - size) < len) {
      read_status = false;
      break;
    }

    size += len;
  }
  CloseFile(file);

  return read_status;
}

bool ReadFileToString(const char* path, std::string* contents) {
  return ReadFileToString(path, contents, std::numeric_limits<size_t>::max());
}

int WriteFileDescriptor(const int fd, const char* data, int size) {
  // Allow for partial writes.
  ssize_t bytes_written_total = 0;
  for (ssize_t bytes_written_partial = 0; bytes_written_total < size;
       bytes_written_total += bytes_written_partial) {
    bytes_written_partial =
        HANDLE_EINTR(write(fd, data + bytes_written_total,
                           size - bytes_written_total));
    if (bytes_written_partial < 0)
      return -1;
  }

  return bytes_written_total;
}

int WriteFile(const char *filename, const char* data, int size) {
  int fd = HANDLE_EINTR(creat(filename, 0666));
  if (fd < 0)
    return -1;

  int bytes_written = WriteFileDescriptor(fd, data, size);
  if (int ret = IGNORE_EINTR(close(fd)) < 0)
    return ret;
  return bytes_written;
}

}  // namespace gestures

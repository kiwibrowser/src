// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CRAZY_LINKER_LINE_READER_H
#define CRAZY_LINKER_LINE_READER_H

#include <string.h>

#include "crazy_linker_system.h"

namespace crazy {

// A class used to read text files line-by-line.
// Usage:
//    LineReader reader("/path/to/file");
//    while (reader.GetNextLine()) {
//       const char* line = reader.line();
//       size_t line_len = reader.length();
//       ... line is not necessarily zero-terminated.
//    }

class LineReader {
 public:
  LineReader();
  explicit LineReader(const char* path);
  ~LineReader();

  // Open a new file for testing. Doesn't fail. If there was an error
  // opening the file, GetNextLine() will simply return false.
  void Open(const char* file_path);

  // Grab next line. Returns true on success, or false otherwise.
  bool GetNextLine();

  // Return the start of the current line, this is _not_ zero-terminated
  // and always contains a final newline (\n).
  // Only call this after a successful GetNextLine().
  const char* line() const;

  // Return the line length, this includes the final \n.
  // Only call this after a successful GetNextLine().
  size_t length() const;

 private:
  void Reset(bool eof);

  FileDescriptor fd_;
  bool eof_;
  size_t line_start_;
  size_t line_len_;
  size_t buff_size_;
  size_t buff_capacity_;
  char* buff_;
};

}  // namespace crazy

#endif  // CRAZY_LINKER_LINE_READER_H

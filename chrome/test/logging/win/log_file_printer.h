// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Prints log files produced by Event Tracing for Windows (by way of the
// FileLogger class) that contain events generated from a select few supported
// providers; see file_logger_win.h for the list.

#ifndef CHROME_TEST_LOGGING_WIN_LOG_FILE_PRINTER_H_
#define CHROME_TEST_LOGGING_WIN_LOG_FILE_PRINTER_H_

#include <iosfwd>

namespace base {
class FilePath;
}

namespace logging_win {

// Reads |log_file|, emitting messages to |out|.  Although it is safe to call
// this from multiple threads, only one file may be read at a time; other
// threads trying to read other log files will be blocked waiting.
void PrintLogFile(const base::FilePath& log_file, std::ostream* out);

}  // namespace logging_win

#endif  // CHROME_TEST_LOGGING_WIN_LOG_FILE_PRINTER_H_

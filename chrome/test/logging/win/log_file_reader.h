// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A log file reader can read log files produced by Event Tracing for Windows
// (by way of the FileLogger class) that contain events generated from a select
// few supported providers; see file_logger_win.h for the list.

#ifndef CHROME_TEST_LOGGING_WIN_LOG_FILE_READER_H_
#define CHROME_TEST_LOGGING_WIN_LOG_FILE_READER_H_

#include <stddef.h>
#include <windows.h>
#include <wmistr.h>
#include <evntrace.h>

#include "base/logging.h"
#include "base/strings/string_piece.h"

namespace base {
class FilePath;
}

namespace logging_win {

// An interface to classes interested in taking action based on events parsed
// out of a log file created by the FileLogger.
class LogFileDelegate {
 public:
  virtual ~LogFileDelegate();

  // Invoked for event types not currently handled by the parser.
  virtual void OnUnknownEvent(const EVENT_TRACE* event) = 0;

  // Invoked for events of known types that cannot be parsed due to unexpected
  // data.
  virtual void OnUnparsableEvent(const EVENT_TRACE* event) = 0;

  // Invoked for the header at the front of all log files.
  virtual void OnFileHeader(const EVENT_TRACE* event,
                            const TRACE_LOGFILE_HEADER* header) = 0;

  // Invoked for simple log messages produced by LogEventProvider.
  virtual void OnLogMessage(const EVENT_TRACE* event,
                            logging::LogSeverity severity,
                            const base::StringPiece& message) = 0;

  // Invoked for full log messages produced by LogEventProvider.
  virtual void OnLogMessageFull(const EVENT_TRACE* event,
                                logging::LogSeverity severity,
                                DWORD stack_depth,
                                const intptr_t* backtrace,
                                int line,
                                const base::StringPiece& file,
                                const base::StringPiece& message) = 0;

 protected:
  LogFileDelegate();
};

// Reads |log_file|, invoking appropriate methods on |delegate| as events are
// parsed.  Although it is safe to call this from multiple threads, only one
// file may be read at a time; other threads trying to read other log files will
// be blocked waiting.
void ReadLogFile(const base::FilePath& log_file, LogFileDelegate* delegate);

}  // namespace logging_win

#endif  // CHROME_TEST_LOGGING_WIN_LOG_FILE_READER_H_

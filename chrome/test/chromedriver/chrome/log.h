// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_CHROMEDRIVER_CHROME_LOG_H_
#define CHROME_TEST_CHROMEDRIVER_CHROME_LOG_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/time/time.h"

namespace base {
class Value;
}

// Abstract class for logging entries with a level, timestamp, string message.
class Log {
 public:
  // Log entry severity level.
  enum Level {
    kAll,
    kDebug,
    kInfo,
    kWarning,
    kError,
    kOff
  };

  virtual ~Log() {}

  // Adds an entry to the log.
  virtual void AddEntryTimestamped(const base::Time& timestamp,
                                   Level level,
                                   const std::string& source,
                                   const std::string& message) = 0;

  virtual bool Emptied() const = 0;

  // Adds an entry to the log, timestamped with the current time.
  void AddEntry(Level level,
                const std::string& source,
                const std::string& message);

  // Adds an entry to the log, timestamped with the current time and no source.
  void AddEntry(Level level, const std::string& message);
};

typedef bool (*IsVLogOnFunc)(int vlog_level);
void InitLogging(IsVLogOnFunc is_vlog_on_func);

// Returns whether the given VLOG level is on.
bool IsVLogOn(int vlog_level);

std::string PrettyPrintValue(const base::Value& value);

// Returns a pretty printed value, after truncating long strings.
std::string FormatValueForDisplay(const base::Value& value);

// Returns a pretty printed json string, after truncating long strings.
std::string FormatJsonForDisplay(const std::string& json);

#endif  // CHROME_TEST_CHROMEDRIVER_CHROME_LOG_H_

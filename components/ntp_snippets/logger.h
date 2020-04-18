// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_LOGGER_H_
#define COMPONENTS_NTP_SNIPPETS_LOGGER_H_

#include <string>

#include "base/containers/circular_deque.h"
#include "base/location.h"
#include "base/time/time.h"

namespace ntp_snippets {

class Logger {
 public:
  Logger();
  ~Logger();

  // The call is ignored if the logging is disabled. However, if constructing a
  // message is not cheap, do it only if |IsLoggingEnabled| returns true.
  void Log(const base::Location& from_here, const std::string& message);
  std::string GetHumanReadableLog() const;

  static bool IsLoggingEnabled();
  static std::string TimeToString(const base::Time& time);

 private:
  struct LogItem {
    std::string message;
    base::Time time;
    base::Location from_where;

    LogItem(const std::string& message,
            const base::Time time,
            const base::Location& from_where);

    std::string ToString() const;
  };

  // New items should be pushed back.
  base::circular_deque<LogItem> logged_items_;
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_LOGGER_H_

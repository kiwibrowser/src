// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_LEVELDATABASE_CHROMIUM_LOGGER_H_
#define THIRD_PARTY_LEVELDATABASE_CHROMIUM_LOGGER_H_

#include <stdint.h>

#include <utility>

#include "base/files/file.h"
#include "base/format_macros.h"
#include "base/time/time.h"

namespace leveldb {

class ChromiumLogger : public Logger {
 public:
  explicit ChromiumLogger(base::File f) : file_(std::move(f)) {}
  virtual ~ChromiumLogger() {}
  virtual void Logv(const char* format, va_list ap) {
    const base::PlatformThreadId thread_id = base::PlatformThread::CurrentId();

    // We try twice: the first time with a fixed-size stack allocated buffer,
    // and the second time with a much larger dynamically allocated buffer.
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
      char* base;
      int bufsize;
      if (iter == 0) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 30000;
        base = new char[bufsize];
      }
      char* p = base;
      char* limit = base + bufsize;

      base::Time::Exploded t;
      base::Time::Now().LocalExplode(&t);

      p += base::snprintf(p, limit - p,
                    "%04d/%02d/%02d-%02d:%02d:%02d.%03d %" PRIu64 " ",
                    t.year,
                    t.month,
                    t.day_of_month,
                    t.hour,
                    t.minute,
                    t.second,
                    t.millisecond,
                    static_cast<uint64_t>(thread_id));

      // Print the message
      if (p < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        p += vsnprintf(p, limit - p, format, backup_ap);
        va_end(backup_ap);
      }

      // Truncate to available space if necessary
      if (p >= limit) {
        if (iter == 0) {
          continue;       // Try again with larger buffer
        } else {
          p = limit - 1;
        }
      }

      // Add newline if necessary
      if (p == base || p[-1] != '\n') {
        *p++ = '\n';
      }

      assert(p <= limit);
      file_.WriteAtCurrentPos(base, p - base);
      if (base != buffer) {
        delete[] base;
      }
      break;
    }
  }

 private:
  base::File file_;
};

}  // namespace leveldb

#endif  // THIRD_PARTY_LEVELDATABASE_CHROMIUM_LOGGER_H_

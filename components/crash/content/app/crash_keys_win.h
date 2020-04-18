// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRASH_CONTENT_APP_CRASH_KEYS_WIN_H_
#define COMPONENTS_CRASH_CONTENT_APP_CRASH_KEYS_WIN_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/synchronization/lock.h"

namespace base {
class CommandLine;
}  // namespace base

namespace crash_reporter {
class CrashReporterClient;
}

namespace google_breakpad {
struct CustomClientInfo;
struct CustomInfoEntry;
}

namespace breakpad {

// Manages the breakpad key/value pair stash, there may only be one instance
// of this class per process at one time.
class CrashKeysWin {
 public:
  CrashKeysWin();
  ~CrashKeysWin();

  // May only be called once.
  // |exe_path| is the path to the executable running, which may be used
  // to figure out whether this is a user or system install.
  // |type| is the process type, or mode this process is running in e.g.
  // something like "browser" or "renderer".
  // |profile_type| is a string describing the kind of the user's Windows
  // profile, e.g. "mandatory", or "roaming" or similar.
  // |cmd_line| is the current process' command line consulted for explicit
  // crash reporting flags.
  // |crash_client| is consulted for crash reporting settings.
  google_breakpad::CustomClientInfo* GetCustomInfo(
        const std::wstring& exe_path,
        const std::wstring& type,
        const std::wstring& profile_type,
        base::CommandLine* cmd_line,
        crash_reporter::CrashReporterClient* crash_client);

  void SetCrashKeyValue(const std::wstring& key, const std::wstring& value);
  void ClearCrashKeyValue(const std::wstring& key);

  const std::vector<google_breakpad::CustomInfoEntry>& custom_info_entries()
      const {
    return *custom_entries_;
  }

  static CrashKeysWin* keeper() { return keeper_; }

 private:
  // One-time initialization of private key/value pairs.
  void SetPluginPath(const std::wstring& path);
  void SetBreakpadDumpPath(crash_reporter::CrashReporterClient* crash_client);

  // Must not be resized after GetCustomInfo is invoked.
  std::unique_ptr<std::vector<google_breakpad::CustomInfoEntry>>
      custom_entries_;

  typedef std::map<std::wstring, google_breakpad::CustomInfoEntry*>
      DynamicEntriesMap;
  base::Lock lock_;
  // Keeps track of the next index for a new dynamic entry.
  size_t dynamic_keys_offset_;  // Under lock_.
  // Maintains key->entry information for dynamic key/value entries
  // in custom_entries_.
  DynamicEntriesMap dynamic_entries_;  // Under lock_.

  // Stores the sole instance of this class allowed per process.
  static CrashKeysWin* keeper_;

  DISALLOW_COPY_AND_ASSIGN(CrashKeysWin);
};

}  // namespace breakpad

#endif  // COMPONENTS_CRASH_CONTENT_APP_CRASH_KEYS_WIN_H_

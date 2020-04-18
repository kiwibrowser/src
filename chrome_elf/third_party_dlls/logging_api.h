// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This source set defines shared expectations for:
// 1. The packed-data format used to pass information from chrome.dll
//    to chrome_elf.dll across restarts.
// 2. The APIs exported by chrome_elf.dll to share logs of module load attempts.

#ifndef CHROME_ELF_THIRD_PARTY_DLLS_LOGGING_API_H_
#define CHROME_ELF_THIRD_PARTY_DLLS_LOGGING_API_H_

#include <windows.h>

#include <stdint.h>

namespace third_party_dlls {

//------------------------------------------------------------------------------
// chrome_elf log API
//------------------------------------------------------------------------------

// Load-attempt log types.
enum LogType : uint8_t {
  kBlocked,
  kAllowed,
};

// Define a flat log entry for any attempted module load.
// The total size in bytes of a log entry is returned by GetLogEntrySize().
// - Note: If this is a |blocked| entry, |path_len| will be 0.
//   (Full path not required for a blacklisted load attempt log.)
struct LogEntry {
  LogType type;
  uint8_t basename_hash[20];
  uint8_t code_id_hash[20];
  // Number of characters in |path| string, not including null terminator.
  uint32_t path_len;
  // UTF-8 full module path, null termination guaranteed.
  char path[1];
};

static_assert(sizeof(LogEntry) == 52,
              "Ensure expectations for padding and alignment are correct.  "
              "If this changes, double check GetLogEntrySize() calculation.");

// Returns the full size for a LogEntry, given the LogEntry.path_len.
// - Always use this function over manual calculation, as it handles padding
//   and alignment.
// - This function will be built into the caller binary.
// - Example of how to use this function to iterate through a buffer returned
//   from DrainLog():
//
//  uint8_t* tracker = buffer;
//  while (tracker < buffer + buffer_bytes_written) {
//    LogEntry* entry = reinterpret_cast<LogEntry*>(tracker);
//    // Do work.
//    tracker += GetLogEntrySize(entry->path_len);
//  }
uint32_t GetLogEntrySize(uint32_t path_len);

}  // namespace third_party_dlls

// Exported API for calling from outside chrome_elf.dll.
// Drains the load attempt LogEntries into the provided buffer.
// - Returns the number of bytes written.  See comments above for LogEntry
//   details.
// - If provided, |log_remaining| receives the number of bytes remaining in the
//   module log, that didn't fit in |buffer|.
// - |buffer_size| can be 0, in which case this simply queries the size of the
//   module log.
extern "C" uint32_t DrainLog(uint8_t* buffer,
                             uint32_t buffer_size,
                             uint32_t* log_remaining);

// Exported API for calling from outside chrome_elf.dll.
// Register an event to be notified when load-attempt logs are available
// via DrainLog API.
// - Pass in a HANDLE to an event created via ::CreateEvent(), or nullptr to
//   clear.
// - This function will duplicate |event_handle|, and call ::SetEvent() when any
//   new load-attempt log is added.
extern "C" bool RegisterLogNotification(HANDLE event_handle);

#endif  // CHROME_ELF_THIRD_PARTY_DLLS_LOGGING_API_H_

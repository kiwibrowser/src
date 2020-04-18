// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/third_party_dlls/logs.h"

#include <windows.h>

#include <assert.h>

#include <vector>

#include "chrome_elf/sha1/sha1.h"

namespace third_party_dlls {
namespace {

enum { kMaxLogEntries = 100, kMaxMutexWaitMs = 5000 };

// Mutex for log access;
HANDLE g_log_mutex = nullptr;

// An event handle that can be registered by outside modules via
// RegisterLogNotification().
HANDLE g_notification_event = nullptr;

// This structure will be translated into LogEntry when draining log.
struct LogEntryInternal {
  uint8_t basename_hash[elf_sha1::kSHA1Length];
  uint8_t code_id_hash[elf_sha1::kSHA1Length];
  std::string full_path;
};

// Converts a given LogEntryInternal into a LogEntry.
void TranslateEntry(LogType log_type,
                    const LogEntryInternal& src,
                    LogEntry* dst) {
  dst->type = log_type;
  ::memcpy(dst->basename_hash, src.basename_hash, elf_sha1::kSHA1Length);
  ::memcpy(dst->code_id_hash, src.code_id_hash, elf_sha1::kSHA1Length);

  // Sanity check - there should be no LogEntryInternal with a too long path.
  // LogLoadAttempt() ensures this.
  assert(src.full_path.size() < std::numeric_limits<uint32_t>::max());
  dst->path_len = static_cast<uint32_t>(src.full_path.size());
  ::memcpy(dst->path, src.full_path.c_str(), dst->path_len + 1);
}

// Class wrapper for internal logging events of module load attempts.
// - A Log instance will either track 'block' events, or 'allow' events.
class Log {
 public:
  // Move constructor
  Log(Log&&) noexcept = default;
  // Move assignment
  Log& operator=(Log&&) noexcept = default;
  // Constructor - |log_type| indicates what LogType this instance holds.
  explicit Log(LogType log_type) : log_type_(log_type) {}

  // Returns the size in bytes of the full log, in terms of LogEntry structs.
  // I.e. how many bytes would a provided buffer need to be to DrainLog().
  uint32_t GetFullLogSize() const {
    uint32_t size = 0;
    for (auto entry : entries_) {
      size += GetLogEntrySize(static_cast<uint32_t>(entry.full_path.size()));
    }
    return size;
  }

  // Add a LogEntryInternal to the log.  Take ownership of the argument.
  void AddEntry(LogEntryInternal&& entry) {
    // Sanity checks.  If load blocked, do not add duplicate logs.
    if (entries_.size() == kMaxLogEntries ||
        (log_type_ == LogType::kBlocked &&
         ContainsEntry(entry.basename_hash, entry.code_id_hash))) {
      return;
    }
    entries_.push_back(std::move(entry));

    // Fire the global notification event - if any is registered.
    if (g_notification_event)
      ::SetEvent(g_notification_event);
  }

  // Writes entries from the start of this Log into |buffer| until either all
  // entries have been written or until no more will fit within |buffer_size|.
  // - Emitted entries are removed from the log.
  // - The number of bytes_written is returned.
  uint32_t Drain(uint8_t* buffer, uint32_t buffer_size) {
    uint32_t remaining_buffer_size = buffer_size;
    uint32_t pop_count = 0;
    uint32_t entry_size = 0;
    uint32_t bytes_written = 0;
    // Drain as many entries as possible.
    for (auto entry : entries_) {
      entry_size =
          GetLogEntrySize(static_cast<uint32_t>(entry.full_path.size()));
      if (remaining_buffer_size < entry_size)
        break;
      LogEntry* temp = reinterpret_cast<LogEntry*>(buffer + bytes_written);
      TranslateEntry(log_type_, entry, temp);
      // Update counters.
      remaining_buffer_size -= entry_size;
      bytes_written += entry_size;
      ++pop_count;
    }
    DequeueEntries(pop_count);
    return bytes_written;
  }

  // Empty the log.
  void Reset() { DequeueEntries(static_cast<uint32_t>(entries_.size())); }

 private:
  // Logs are currently unordered, so just loop.
  // - Returns true if the given hashes already exist in the log.
  bool ContainsEntry(const uint8_t* basename_hash,
                     const uint8_t* code_id_hash) const {
    for (auto entry : entries_) {
      if (!elf_sha1::CompareHashes(basename_hash, entry.basename_hash) &&
          !elf_sha1::CompareHashes(code_id_hash, entry.code_id_hash)) {
        return true;
      }
    }
    return false;
  }

  // Remove |count| entries from start of log vector.
  // - More efficient to take a chunk off the vector once, instead of one entry
  //   at a time.
  void DequeueEntries(uint32_t count) {
    assert(count <= entries_.size());
    entries_.erase(entries_.begin(), entries_.begin() + count);
  }

  LogType log_type_;
  std::vector<LogEntryInternal> entries_;

  // DISALLOW_COPY_AND_ASSIGN(Log);
  Log(const Log&) = delete;
  Log& operator=(const Log&) = delete;
};

// NOTE: these "globals" are only initialized once during InitLogs().
// NOTE: they are wrapped in functions to prevent exit-time dtors.
// *These returned Log instances must only be accessed under g_log_mutex.
Log& GetBlockedLog() {
  static Log* const blocked_log = new Log(LogType::kBlocked);
  return *blocked_log;
}

Log& GetAllowedLog() {
  static Log* const allowed_log = new Log(LogType::kAllowed);
  return *allowed_log;
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

// This is called from inside a hook shim, so don't bother with return status.
void LogLoadAttempt(LogType log_type,
                    const std::string& basename_hash,
                    const std::string& code_id_hash,
                    const std::string& full_image_path) {
  assert(g_log_mutex);
  assert(!basename_hash.empty() && !code_id_hash.empty());
  assert(basename_hash.length() == elf_sha1::kSHA1Length &&
         code_id_hash.length() == elf_sha1::kSHA1Length);

  if (::WaitForSingleObject(g_log_mutex, kMaxMutexWaitMs) != WAIT_OBJECT_0)
    return;

  // Build the new log entry.
  LogEntryInternal entry;
  ::memcpy(&entry.basename_hash[0], basename_hash.data(),
           elf_sha1::kSHA1Length);
  ::memcpy(&entry.code_id_hash[0], code_id_hash.data(), elf_sha1::kSHA1Length);

  // Only store full path if the module was allowed to load.
  if (log_type == LogType::kAllowed) {
    entry.full_path = full_image_path;
    // Edge condition.  Ensure the path length is <= max(uint32_t) - 1.
    if (entry.full_path.size() > std::numeric_limits<uint32_t>::max() - 1)
      entry.full_path.resize(std::numeric_limits<uint32_t>::max() - 1);
  }

  // Add the new entry.
  Log& log =
      (log_type == LogType::kBlocked ? GetBlockedLog() : GetAllowedLog());
  log.AddEntry(std::move(entry));

  ::ReleaseMutex(g_log_mutex);
}

LogStatus InitLogs() {
  // Debug check: InitLogs should not be called more than once.
  assert(!g_log_mutex);

  // Create unnamed mutex for log access.
  g_log_mutex = ::CreateMutex(nullptr, false, nullptr);
  if (!g_log_mutex)
    return LogStatus::kCreateMutexFailure;

  return LogStatus::kSuccess;
}

void DeinitLogs() {
  if (g_log_mutex)
    ::CloseHandle(g_log_mutex);
  g_log_mutex = nullptr;

  GetBlockedLog().Reset();
  GetAllowedLog().Reset();
}

}  // namespace third_party_dlls

//------------------------------------------------------------------------------
// Exports
// - Function definition in logging_api.h
// - Export declared in chrome_elf_[x64|x86].def
//------------------------------------------------------------------------------
using namespace third_party_dlls;

uint32_t DrainLog(uint8_t* buffer,
                  uint32_t buffer_size,
                  uint32_t* log_remaining) {
  if (!g_log_mutex ||
      ::WaitForSingleObject(g_log_mutex, kMaxMutexWaitMs) != WAIT_OBJECT_0)
    return 0;

  Log& blocked = GetBlockedLog();
  Log& allowed = GetAllowedLog();

  uint32_t bytes_written = blocked.Drain(buffer, buffer_size);
  bytes_written +=
      allowed.Drain(buffer + bytes_written, buffer_size - bytes_written);

  // If requested, return the remaining logs size.
  if (log_remaining) {
    // Edge case: maximum 32-bit value.
    uint64_t full_size = blocked.GetFullLogSize() + allowed.GetFullLogSize();
    if (full_size > std::numeric_limits<uint32_t>::max())
      *log_remaining = std::numeric_limits<uint32_t>::max();
    else
      *log_remaining = static_cast<uint32_t>(full_size);
  }

  ::ReleaseMutex(g_log_mutex);

  return bytes_written;
}

bool RegisterLogNotification(HANDLE event_handle) {
  if (!g_log_mutex)
    return false;

  // Duplicate the new handle, if not clearing with nullptr.
  HANDLE temp = nullptr;
  if (event_handle && !::DuplicateHandle(::GetCurrentProcess(), event_handle,
                                         ::GetCurrentProcess(), &temp, 0, FALSE,
                                         DUPLICATE_SAME_ACCESS)) {
    return false;
  }

  // Close any existing registered handle.
  if (g_notification_event)
    ::CloseHandle(g_notification_event);

  g_notification_event = temp;

  return true;
}

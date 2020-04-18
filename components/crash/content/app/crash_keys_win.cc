// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/crash_keys_win.h"

#include <algorithm>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "components/crash/content/app/crash_reporter_client.h"
#include "third_party/breakpad/breakpad/src/client/windows/common/ipc_protocol.h"

namespace breakpad {

using crash_reporter::CrashReporterClient;

namespace {

const size_t kMaxPluginPathLength = 256;
const size_t kMaxDynamicEntries = 256;

}  // namespace

CrashKeysWin* CrashKeysWin::keeper_;

CrashKeysWin::CrashKeysWin()
    : custom_entries_(new std::vector<google_breakpad::CustomInfoEntry>),
      dynamic_keys_offset_(0) {
  DCHECK(!keeper_);
  keeper_ = this;
}

CrashKeysWin::~CrashKeysWin() {
  DCHECK_EQ(this, keeper_);
  keeper_ = nullptr;
}

// Appends the plugin path to |g_custom_entries|.
void CrashKeysWin::SetPluginPath(const std::wstring& path) {
  if (path.size() > kMaxPluginPathLength) {
    // If the path is too long, truncate from the start rather than the end,
    // since we want to be able to recover the DLL name.
    SetPluginPath(path.substr(path.size() - kMaxPluginPathLength));
    return;
  }

  // The chunk size without terminator.
  const size_t kChunkSize = static_cast<size_t>(
      google_breakpad::CustomInfoEntry::kValueMaxLength - 1);

  int chunk_index = 0;
  size_t chunk_start = 0;  // Current position inside |path|

  for (chunk_start = 0; chunk_start < path.size(); chunk_index++) {
    size_t chunk_length = std::min(kChunkSize, path.size() - chunk_start);

    custom_entries_->push_back(google_breakpad::CustomInfoEntry(
        base::StringPrintf(L"plugin-path-chunk-%i", chunk_index + 1).c_str(),
        path.substr(chunk_start, chunk_length).c_str()));

    chunk_start += chunk_length;
  }
}

// Appends the breakpad dump path to |g_custom_entries|.
void CrashKeysWin::SetBreakpadDumpPath(CrashReporterClient* crash_client) {
  base::string16 crash_dumps_dir_path;
  if (crash_client->GetAlternativeCrashDumpLocation(&crash_dumps_dir_path)) {
    custom_entries_->push_back(google_breakpad::CustomInfoEntry(
        L"breakpad-dump-location", crash_dumps_dir_path.c_str()));
  }
}

// Returns the custom info structure based on the dll in parameter and the
// process type.
google_breakpad::CustomClientInfo*
CrashKeysWin::GetCustomInfo(const std::wstring& exe_path,
                            const std::wstring& type,
                            const std::wstring& profile_type,
                            base::CommandLine* cmd_line,
                            CrashReporterClient* crash_client) {
  base::string16 version, product;
  base::string16 special_build;
  base::string16 channel_name;

  crash_client->GetProductNameAndVersion(
      exe_path,
      &product,
      &version,
      &special_build,
      &channel_name);

  // We only expect this method to be called once per process.
  // Common enties
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(L"ver", version.c_str()));
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(L"prod", product.c_str()));
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(L"plat", L"Win32"));
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(L"ptype", type.c_str()));
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(
      L"pid", base::IntToString16(::GetCurrentProcessId()).c_str()));
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(L"channel", channel_name.c_str()));
  custom_entries_->push_back(
      google_breakpad::CustomInfoEntry(L"profile-type", profile_type.c_str()));

  if (!special_build.empty()) {
    custom_entries_->push_back(
        google_breakpad::CustomInfoEntry(L"special", special_build.c_str()));
  }

  if (type == L"plugin" || type == L"ppapi") {
    std::wstring plugin_path = cmd_line->GetSwitchValueNative("plugin-path");
    if (!plugin_path.empty())
      SetPluginPath(plugin_path);
  }

  // Check whether configuration management controls crash reporting.
  bool crash_reporting_enabled = true;
  bool controlled_by_policy = crash_client->ReportingIsEnforcedByPolicy(
      &crash_reporting_enabled);
  bool use_crash_service = !controlled_by_policy &&
      (cmd_line->HasSwitch(switches::kNoErrorDialogs) ||
          crash_client->IsRunningUnattended());
  if (use_crash_service)
    SetBreakpadDumpPath(crash_client);

  // Create space for dynamic ad-hoc keys. The names and values are set using
  // the API defined in base/debug/crash_logging.h.
  dynamic_keys_offset_ = custom_entries_->size();
  for (size_t i = 0; i < kMaxDynamicEntries; ++i) {
    // The names will be mutated as they are set. Un-numbered since these are
    // merely placeholders. The name cannot be empty because Breakpad's
    // HTTPUpload will interpret that as an invalid parameter.
    custom_entries_->push_back(
        google_breakpad::CustomInfoEntry(L"unspecified-crash-key", L""));
  }

  static google_breakpad::CustomClientInfo custom_client_info;
  custom_client_info.entries = &custom_entries_->front();
  custom_client_info.count = custom_entries_->size();

  return &custom_client_info;
}

void CrashKeysWin::SetCrashKeyValue(
    const std::wstring& key, const std::wstring& value) {
  // CustomInfoEntry limits the length of key and value. If they exceed
  // their maximum length the underlying string handling functions raise
  // an exception and prematurely trigger a crash. Truncate here.
  std::wstring safe_key(std::wstring(key).substr(
      0, google_breakpad::CustomInfoEntry::kNameMaxLength  - 1));
  std::wstring safe_value(std::wstring(value).substr(
      0, google_breakpad::CustomInfoEntry::kValueMaxLength - 1));

  // If we already have a value for this key, update it; otherwise, insert
  // the new value if we have not exhausted the pre-allocated slots for dynamic
  // entries.
  base::AutoLock lock(lock_);

  DynamicEntriesMap::iterator it = dynamic_entries_.find(safe_key);
  google_breakpad::CustomInfoEntry* entry = nullptr;
  if (it == dynamic_entries_.end()) {
    if (dynamic_entries_.size() >= kMaxDynamicEntries)
      return;
    entry = &(*custom_entries_)[dynamic_keys_offset_++];
    dynamic_entries_.insert(std::make_pair(safe_key, entry));
  } else {
    entry = it->second;
  }

  entry->set(safe_key.data(), safe_value.data());
}

void CrashKeysWin::ClearCrashKeyValue(const std::wstring& key) {
  base::AutoLock lock(lock_);

  std::wstring key_string(key);
  DynamicEntriesMap::iterator it = dynamic_entries_.find(key_string);
  if (it == dynamic_entries_.end())
    return;

  it->second->set_value(nullptr);
}

}  // namespace breakpad

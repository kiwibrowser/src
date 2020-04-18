// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/single_app_install_event_log.h"

#include <stddef.h>
#include <stdint.h>

#include "base/files/file.h"

namespace em = enterprise_management;

namespace policy {

namespace {
static const int kLogCapacity = 1024;
static const int kMaxBufferSize = 65536;
}  // namespace

SingleAppInstallEventLog::SingleAppInstallEventLog(const std::string& package)
    : package_(package) {}

SingleAppInstallEventLog::~SingleAppInstallEventLog() {}

void SingleAppInstallEventLog::Add(const em::AppInstallReportLogEvent& event) {
  events_.push_back(event);
  if (events_.size() > kLogCapacity) {
    incomplete_ = true;
    if (serialized_entries_ > -1) {
      --serialized_entries_;
    }
    events_.pop_front();
  }
}

bool SingleAppInstallEventLog::Load(
    base::File* file,
    std::unique_ptr<SingleAppInstallEventLog>* log) {
  log->reset();

  if (!file->IsValid()) {
    return false;
  }

  ssize_t size;
  if (file->ReadAtCurrentPos(reinterpret_cast<char*>(&size), sizeof(size)) !=
          sizeof(size) ||
      size > kMaxBufferSize) {
    return false;
  }

  std::unique_ptr<char[]> package_buffer = std::make_unique<char[]>(size);
  if (file->ReadAtCurrentPos(package_buffer.get(), size) != size) {
    return false;
  }

  *log = std::make_unique<SingleAppInstallEventLog>(
      std::string(package_buffer.get(), size));

  int64_t incomplete;
  if (file->ReadAtCurrentPos(reinterpret_cast<char*>(&incomplete),
                             sizeof(incomplete)) != sizeof(incomplete)) {
    return false;
  }
  (*log)->incomplete_ = incomplete;

  ssize_t entries;
  if (file->ReadAtCurrentPos(reinterpret_cast<char*>(&entries),
                             sizeof(entries)) != sizeof(entries)) {
    return false;
  }

  for (ssize_t i = 0; i < entries; ++i) {
    if (file->ReadAtCurrentPos(reinterpret_cast<char*>(&size), sizeof(size)) !=
            sizeof(size) ||
        size > kMaxBufferSize) {
      (*log)->incomplete_ = true;
      return false;
    }

    if (size == 0) {
      // Zero-size entries are written if serialization of a log entry fails.
      // Skip these on read.
      (*log)->incomplete_ = true;
      continue;
    }

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
    if (file->ReadAtCurrentPos(buffer.get(), size) != size) {
      (*log)->incomplete_ = true;
      return false;
    }

    em::AppInstallReportLogEvent event;
    if (event.ParseFromArray(buffer.get(), size)) {
      (*log)->Add(event);
    } else {
      (*log)->incomplete_ = true;
    }
  }

  return true;
}

bool SingleAppInstallEventLog::Store(base::File* file) const {
  if (!file->IsValid()) {
    return false;
  }

  ssize_t size = package_.size();
  if (file->WriteAtCurrentPos(reinterpret_cast<const char*>(&size),
                              sizeof(size)) != sizeof(size)) {
    return false;
  }

  if (file->WriteAtCurrentPos(package_.data(), size) != size) {
    return false;
  }

  const int64_t incomplete = incomplete_;
  if (file->WriteAtCurrentPos(reinterpret_cast<const char*>(&incomplete),
                              sizeof(incomplete)) != sizeof(incomplete)) {
    return false;
  }

  const ssize_t entries = events_.size();
  if (file->WriteAtCurrentPos(reinterpret_cast<const char*>(&entries),
                              sizeof(entries)) != sizeof(entries)) {
    return false;
  }

  for (const em::AppInstallReportLogEvent& event : events_) {
    size = event.ByteSizeLong();
    std::unique_ptr<char[]> buffer;

    if (size > kMaxBufferSize) {
      // Log entry too large. Skip it.
      size = 0;
    } else {
      buffer = std::make_unique<char[]>(size);
      if (!event.SerializeToArray(buffer.get(), size)) {
        // Log entry serialization failed. Skip it.
        size = 0;
      }
    }

    if (file->WriteAtCurrentPos(reinterpret_cast<const char*>(&size),
                                sizeof(size)) != sizeof(size) ||
        (size && file->WriteAtCurrentPos(buffer.get(), size) != size)) {
      return false;
    }
  }

  return true;
}

void SingleAppInstallEventLog::Serialize(em::AppInstallReport* report) {
  report->Clear();
  report->set_package(package_);
  report->set_incomplete(incomplete_);
  for (const auto& event : events_) {
    em::AppInstallReportLogEvent* const log_event = report->add_log();
    *log_event = event;
  }
  serialized_entries_ = events_.size();
}

void SingleAppInstallEventLog::ClearSerialized() {
  if (serialized_entries_ == -1) {
    return;
  }

  events_.erase(events_.begin(), events_.begin() + serialized_entries_);
  serialized_entries_ = -1;
  incomplete_ = false;
}

}  // namespace policy

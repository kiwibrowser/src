// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_SINGLE_APP_INSTALL_EVENT_LOG_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_SINGLE_APP_INSTALL_EVENT_LOG_H_

#include <deque>
#include <memory>
#include <string>

#include "components/policy/proto/device_management_backend.pb.h"

#include "base/macros.h"

namespace base {
class File;
}

namespace policy {

// An event log for a single app's push-install process. The log can be stored
// on disk and serialized for upload to a server. The log is internally held in
// a round-robin buffer. A flag indicates whether any log entries were lost
// (e.g. entry too large or buffer wrapped around). Log entries are pruned and
// the flag is cleared after upload has completed.
class SingleAppInstallEventLog {
 public:
  explicit SingleAppInstallEventLog(const std::string& package);
  ~SingleAppInstallEventLog();

  const std::string& package() const { return package_; }

  int size() const { return events_.size(); }

  bool empty() const { return events_.empty(); }

  // Add a log entry. If the buffer is full, the oldest entry is removed and
  // |incomplete_| is set.
  void Add(const enterprise_management::AppInstallReportLogEvent& event);

  // Restores the event log from |file| into |log|. Returns |true| if the
  // self-delimiting format of the log was parsed successfully and further logs
  // stored in the file may be loaded.
  // |incomplete_| is set to |true| if it was set when storing the log to the
  // file, the buffer wraps around or any log entries cannot be fully parsed. If
  // not even the app name can be parsed, |log| is set to |nullptr|.
  static bool Load(base::File* file,
                   std::unique_ptr<SingleAppInstallEventLog>* log);

  // Stores the event log to |file|. Returns |true| if the log was written
  // successfully in a self-delimiting manner and the file may be used to store
  // further logs.
  bool Store(base::File* file) const;

  // Serializes the log to a protobuf for upload to a server. Records which
  // entries were serialized so that they may be cleared after successful
  // upload.
  void Serialize(enterprise_management::AppInstallReport* report);

  // Clears log entries that were previously serialized. Also clears
  // |incomplete_| if all log entries added since serialization are still
  // present in the log.
  void ClearSerialized();

 private:
  // The app this event log pertains to.
  const std::string package_;

  // The buffer holding log entries.
  std::deque<enterprise_management::AppInstallReportLogEvent> events_;

  // Whether any log entries were lost (e.g. entry too large or buffer wrapped
  // around).
  bool incomplete_ = false;

  // The number of entries that were serialized and can be cleared from the log
  // after successful upload to the server, or -1 if none.
  int serialized_entries_ = -1;

  DISALLOW_COPY_AND_ASSIGN(SingleAppInstallEventLog);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_SINGLE_APP_INSTALL_EVENT_LOG_H_

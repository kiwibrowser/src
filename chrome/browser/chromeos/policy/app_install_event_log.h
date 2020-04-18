// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_APP_INSTALL_EVENT_LOG_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_APP_INSTALL_EVENT_LOG_H_

#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"

namespace enterprise_management {
class AppInstallReportLogEvent;
class AppInstallReportRequest;
}  // namespace enterprise_management

namespace policy {

class SingleAppInstallEventLog;

// An event log for app push-installs. The log entries for each app are kept in
// a separate round-robin buffer. The log can be stored on disk and serialized
// for upload to a server. Log entries are pruned after upload has completed.
class AppInstallEventLog {
 public:
  // Restores the event log from |file_name|. If there is an error parsing the
  // file, as many log entries as possible are restored.
  explicit AppInstallEventLog(const base::FilePath& file_name);
  ~AppInstallEventLog();

  // The current total number of log entries, across all apps.
  int total_size() { return total_size_; }

  // The current maximum number of log entries for a single app.
  int max_size() { return max_size_; }

  // Add a log entry for |package|. If the buffer for that app is full, the
  // oldest entry is removed.
  void Add(const std::string& package,
           const enterprise_management::AppInstallReportLogEvent& event);

  // Stores the event log to the file name provided to the constructor. If the
  // event log has not changed since it was last stored to disk (or initially
  // loaded from disk), does nothing.
  void Store();

  // Serializes the log to a protobuf for upload to a server. Records which
  // entries were serialized so that they may be cleared after successful
  // upload.
  void Serialize(enterprise_management::AppInstallReportRequest* report);

  // Clears log entries that were previously serialized.
  void ClearSerialized();

 private:
  // The round-robin log event buffers for individual apps.
  std::map<std::string, std::unique_ptr<SingleAppInstallEventLog>> logs_;

  const base::FilePath file_name_;

  // The current total number of log entries, across all apps.
  int total_size_ = 0;

  // The current maximum number of log entries for a single app.
  int max_size_ = 0;

  // Whether the event log changed since it was last stored to disk (or
  // initially loaded from disk).
  bool dirty_ = false;

  DISALLOW_COPY_AND_ASSIGN(AppInstallEventLog);
};

}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_APP_INSTALL_EVENT_LOG_H_

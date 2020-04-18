// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SYSTEM_LOGS_DEBUG_DAEMON_LOG_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_SYSTEM_LOGS_DEBUG_DAEMON_LOG_SOURCE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "components/feedback/system_logs/system_logs_source.h"

namespace system_logs {

// Gathers log data from Debug Daemon.
class DebugDaemonLogSource : public SystemLogsSource {
 public:
  explicit DebugDaemonLogSource(bool scrub);
  ~DebugDaemonLogSource() override;

  // SystemLogsSource override:
  // Fetches logs from the daemon over dbus. After the fetch is complete, the
  // results will be forwarded to the request supplied to the constructor and
  // this instance will free itself.
  void Fetch(SysLogsSourceCallback callback) override;

 private:
  typedef std::map<std::string, std::string> KeyValueMap;

  // Callbacks for the 5 different dbus calls to debugd.
  void OnGetRoutes(base::Optional<std::vector<std::string>> routes);
  void OnGetNetworkStatus(base::Optional<std::string> status);
  void OnGetModemStatus(base::Optional<std::string> status);
  void OnGetWiMaxStatus(base::Optional<std::string> status);
  void OnGetLogs(bool succeeded,
                 const KeyValueMap& logs);
  void OnGetUserLogFiles(bool succeeded,
                         const KeyValueMap& logs);

  // Read the contents of the specified user logs files and adds it to
  // the response parameter.
  static void ReadUserLogFiles(
      const KeyValueMap& user_log_files,
      const std::vector<base::FilePath>& profile_dirs,
      SystemLogsResponse* response);

  // Merge the responses from ReadUserLogFiles into the main response dict and
  // invoke the callback_.Run method with the assumption that all other logs
  // have already been collected.
  void MergeUserLogFilesResponse(std::unique_ptr<SystemLogsResponse> response);

  // When all the requests are completed, send one last request to collect the
  // user logs and complete the collection by invoking the callback's Run
  // method.
  void RequestCompleted();

  std::unique_ptr<SystemLogsResponse> response_;
  SysLogsSourceCallback callback_;
  int num_pending_requests_;
  bool scrub_;
  base::WeakPtrFactory<DebugDaemonLogSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DebugDaemonLogSource);
};


}  // namespace system_logs

#endif  // CHROME_BROWSER_CHROMEOS_SYSTEM_LOGS_DEBUG_DAEMON_LOG_SOURCE_H_

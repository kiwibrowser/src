// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system_logs/debug_daemon_log_source.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"

const char kNotAvailable[] = "<not available>";
const char kRoutesKeyName[] = "routes";
const char kNetworkStatusKeyName[] = "network-status";
const char kModemStatusKeyName[] = "modem-status";
const char kWiMaxStatusKeyName[] = "wimax-status";
const char kUserLogFileKeyName[] = "user_log_files";

namespace system_logs {

DebugDaemonLogSource::DebugDaemonLogSource(bool scrub)
    : SystemLogsSource("DebugDemon"),
      response_(new SystemLogsResponse()),
      num_pending_requests_(0),
      scrub_(scrub),
      weak_ptr_factory_(this) {}

DebugDaemonLogSource::~DebugDaemonLogSource() {}

void DebugDaemonLogSource::Fetch(SysLogsSourceCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!callback.is_null());
  DCHECK(callback_.is_null());

  callback_ = std::move(callback);
  chromeos::DebugDaemonClient* client =
      chromeos::DBusThreadManager::Get()->GetDebugDaemonClient();

  client->GetRoutes(true,   // Numeric
                    false,  // No IPv6
                    base::Bind(&DebugDaemonLogSource::OnGetRoutes,
                               weak_ptr_factory_.GetWeakPtr()));
  ++num_pending_requests_;
  client->GetNetworkStatus(base::Bind(&DebugDaemonLogSource::OnGetNetworkStatus,
                                      weak_ptr_factory_.GetWeakPtr()));
  ++num_pending_requests_;
  client->GetModemStatus(base::Bind(&DebugDaemonLogSource::OnGetModemStatus,
                                    weak_ptr_factory_.GetWeakPtr()));
  ++num_pending_requests_;
  client->GetWiMaxStatus(base::Bind(&DebugDaemonLogSource::OnGetWiMaxStatus,
                                    weak_ptr_factory_.GetWeakPtr()));
  ++num_pending_requests_;

  if (scrub_) {
    client->GetScrubbedBigLogs(base::Bind(&DebugDaemonLogSource::OnGetLogs,
                                          weak_ptr_factory_.GetWeakPtr()));
  } else {
    client->GetAllLogs(base::Bind(&DebugDaemonLogSource::OnGetLogs,
                                  weak_ptr_factory_.GetWeakPtr()));
  }
  ++num_pending_requests_;
}

void DebugDaemonLogSource::OnGetRoutes(
    base::Optional<std::vector<std::string>> routes) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  (*response_)[kRoutesKeyName] = routes.has_value()
                                     ? base::JoinString(routes.value(), "\n")
                                     : kNotAvailable;
  RequestCompleted();
}

void DebugDaemonLogSource::OnGetNetworkStatus(
    base::Optional<std::string> status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  (*response_)[kNetworkStatusKeyName] =
      std::move(status).value_or(kNotAvailable);
  RequestCompleted();
}

void DebugDaemonLogSource::OnGetModemStatus(
    base::Optional<std::string> status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  (*response_)[kModemStatusKeyName] = std::move(status).value_or(kNotAvailable);
  RequestCompleted();
}

void DebugDaemonLogSource::OnGetWiMaxStatus(
    base::Optional<std::string> status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  (*response_)[kWiMaxStatusKeyName] = std::move(status).value_or(kNotAvailable);
  RequestCompleted();
}

void DebugDaemonLogSource::OnGetLogs(bool /* succeeded */,
                                     const KeyValueMap& logs) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // We ignore 'succeeded' for this callback - we want to display as much of the
  // debug info as we can even if we failed partway through parsing, and if we
  // couldn't fetch any of it, none of the fields will even appear.
  response_->insert(logs.begin(), logs.end());
  RequestCompleted();
}

void DebugDaemonLogSource::OnGetUserLogFiles(
    bool succeeded,
    const KeyValueMap& user_log_files) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (succeeded) {
    auto response = std::make_unique<SystemLogsResponse>();
    SystemLogsResponse* response_ptr = response.get();

    const user_manager::UserList& users =
        user_manager::UserManager::Get()->GetLoggedInUsers();
    std::vector<base::FilePath> profile_dirs;
    for (user_manager::UserList::const_iterator it = users.begin();
         it != users.end();
         ++it) {
      if ((*it)->username_hash().empty())
        continue;
      profile_dirs.push_back(
          chromeos::ProfileHelper::GetProfilePathByUserIdHash(
              (*it)->username_hash()));
    }

    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
        base::BindOnce(&DebugDaemonLogSource::ReadUserLogFiles, user_log_files,
                       profile_dirs, response_ptr),
        base::BindOnce(&DebugDaemonLogSource::MergeUserLogFilesResponse,
                       weak_ptr_factory_.GetWeakPtr(), std::move(response)));
  } else {
    (*response_)[kUserLogFileKeyName] = kNotAvailable;
    auto response = std::make_unique<SystemLogsResponse>();
    std::swap(response, response_);
    DCHECK(!callback_.is_null());
    std::move(callback_).Run(std::move(response));
  }
}

// static
void DebugDaemonLogSource::ReadUserLogFiles(
    const KeyValueMap& user_log_files,
    const std::vector<base::FilePath>& profile_dirs,
    SystemLogsResponse* response) {
  for (size_t i = 0; i < profile_dirs.size(); ++i) {
    std::string profile_prefix = "Profile[" + base::UintToString(i) + "] ";
    for (KeyValueMap::const_iterator it = user_log_files.begin();
         it != user_log_files.end();
         ++it) {
      std::string key = it->first;
      std::string value;
      std::string filename = it->second;
      bool read_success = base::ReadFileToString(
          profile_dirs[i].Append(filename), &value);

      if (read_success && !value.empty())
        (*response)[profile_prefix + key] = value;
      else
        (*response)[profile_prefix + filename] = kNotAvailable;
    }
  }
}

void DebugDaemonLogSource::MergeUserLogFilesResponse(
    std::unique_ptr<SystemLogsResponse> response) {
  response_->insert(response->begin(), response->end());
  auto response_to_return = std::make_unique<SystemLogsResponse>();
  std::swap(response_to_return, response_);
  DCHECK(!callback_.is_null());
  std::move(callback_).Run(std::move(response_to_return));
}

void DebugDaemonLogSource::RequestCompleted() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!callback_.is_null());

  --num_pending_requests_;
  if (num_pending_requests_ > 0)
    return;
  // When all other logs are collected, fetch the user logs, because any errors
  // fetching the other logs is reported in the user logs.
  chromeos::DebugDaemonClient* client =
      chromeos::DBusThreadManager::Get()->GetDebugDaemonClient();
  client->GetUserLogFiles(base::Bind(&DebugDaemonLogSource::OnGetUserLogFiles,
                                     weak_ptr_factory_.GetWeakPtr()));
}

}  // namespace system_logs

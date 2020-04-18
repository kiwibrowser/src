// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_upstart_client.h"

#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_auth_policy_client.h"
#include "chromeos/dbus/fake_media_analytics_client.h"

namespace chromeos {

FakeUpstartClient::FakeUpstartClient() = default;

FakeUpstartClient::~FakeUpstartClient() = default;

void FakeUpstartClient::Init(dbus::Bus* bus) {}

void FakeUpstartClient::StartAuthPolicyService() {
  static_cast<FakeAuthPolicyClient*>(
      DBusThreadManager::Get()->GetAuthPolicyClient())
      ->set_started(true);
}

void FakeUpstartClient::RestartAuthPolicyService() {
  FakeAuthPolicyClient* authpolicy_client = static_cast<FakeAuthPolicyClient*>(
      DBusThreadManager::Get()->GetAuthPolicyClient());
  DLOG_IF(WARNING, !authpolicy_client->started())
      << "Trying to restart authpolicyd which is not started";
  authpolicy_client->set_started(true);
}

void FakeUpstartClient::StartMediaAnalytics(
    const std::vector<std::string>& /* upstart_env */,
    VoidDBusMethodCallback callback) {
  FakeMediaAnalyticsClient* media_analytics_client =
      static_cast<FakeMediaAnalyticsClient*>(
          DBusThreadManager::Get()->GetMediaAnalyticsClient());
  DLOG_IF(WARNING, media_analytics_client->process_running())
      << "Trying to start media analytics which is already started.";
  media_analytics_client->set_process_running(true);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true));
}

void FakeUpstartClient::RestartMediaAnalytics(VoidDBusMethodCallback callback) {
  FakeMediaAnalyticsClient* media_analytics_client =
      static_cast<FakeMediaAnalyticsClient*>(
          DBusThreadManager::Get()->GetMediaAnalyticsClient());
  media_analytics_client->set_process_running(false);
  media_analytics_client->set_process_running(true);
  media_analytics_client->SetStateSuspended();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true));
}

void FakeUpstartClient::StopMediaAnalytics() {
  FakeMediaAnalyticsClient* media_analytics_client =
      static_cast<FakeMediaAnalyticsClient*>(
          DBusThreadManager::Get()->GetMediaAnalyticsClient());
  DLOG_IF(WARNING, !media_analytics_client->process_running())
      << "Trying to stop media analytics which is not started.";
  media_analytics_client->set_process_running(false);
}

void FakeUpstartClient::StopMediaAnalytics(VoidDBusMethodCallback callback) {
  FakeMediaAnalyticsClient* media_analytics_client =
      static_cast<FakeMediaAnalyticsClient*>(
          DBusThreadManager::Get()->GetMediaAnalyticsClient());
  media_analytics_client->set_process_running(false);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), true));
}

}  // namespace chromeos

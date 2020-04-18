// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/test_background_sync_manager.h"

#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

TestBackgroundSyncManager::TestBackgroundSyncManager(
    scoped_refptr<ServiceWorkerContextWrapper> service_worker_context)
    : BackgroundSyncManager(service_worker_context) {}

TestBackgroundSyncManager::~TestBackgroundSyncManager() {}

void TestBackgroundSyncManager::DoInit() {
  Init();
}

void TestBackgroundSyncManager::ResumeBackendOperation() {
  ASSERT_TRUE(continuation_);
  std::move(continuation_).Run();
}

void TestBackgroundSyncManager::ClearDelayedTask() {
  delayed_task_.Reset();
}

void TestBackgroundSyncManager::StoreDataInBackend(
    int64_t sw_registration_id,
    const GURL& origin,
    const std::string& key,
    const std::string& data,
    ServiceWorkerStorage::StatusCallback callback) {
  EXPECT_FALSE(continuation_);
  if (corrupt_backend_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback), SERVICE_WORKER_ERROR_FAILED));
    return;
  }
  continuation_ =
      base::BindOnce(&TestBackgroundSyncManager::StoreDataInBackendContinue,
                     base::Unretained(this), sw_registration_id, origin, key,
                     data, std::move(callback));
  if (delay_backend_)
    return;

  ResumeBackendOperation();
}

void TestBackgroundSyncManager::GetDataFromBackend(
    const std::string& key,
    ServiceWorkerStorage::GetUserDataForAllRegistrationsCallback callback) {
  EXPECT_FALSE(continuation_);
  if (corrupt_backend_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       std::vector<std::pair<int64_t, std::string>>(),
                       SERVICE_WORKER_ERROR_FAILED));
    return;
  }
  continuation_ =
      base::BindOnce(&TestBackgroundSyncManager::GetDataFromBackendContinue,
                     base::Unretained(this), key, std::move(callback));
  if (delay_backend_)
    return;

  ResumeBackendOperation();
}

void TestBackgroundSyncManager::DispatchSyncEvent(
    const std::string& tag,
    scoped_refptr<ServiceWorkerVersion> active_version,
    bool last_chance,
    ServiceWorkerVersion::StatusCallback callback) {
  ASSERT_TRUE(dispatch_sync_callback_);
  last_chance_ = last_chance;
  dispatch_sync_callback_.Run(active_version, std::move(callback));
}

void TestBackgroundSyncManager::ScheduleDelayedTask(base::OnceClosure callback,
                                                    base::TimeDelta delay) {
  delayed_task_ = std::move(callback);
  delayed_task_delta_ = delay;
}

void TestBackgroundSyncManager::HasMainFrameProviderHost(
    const GURL& origin,
    BoolCallback callback) {
  std::move(callback).Run(has_main_frame_provider_host_);
}

void TestBackgroundSyncManager::StoreDataInBackendContinue(
    int64_t sw_registration_id,
    const GURL& origin,
    const std::string& key,
    const std::string& data,
    ServiceWorkerStorage::StatusCallback callback) {
  BackgroundSyncManager::StoreDataInBackend(sw_registration_id, origin, key,
                                            data, std::move(callback));
}

void TestBackgroundSyncManager::GetDataFromBackendContinue(
    const std::string& key,
    ServiceWorkerStorage::GetUserDataForAllRegistrationsCallback callback) {
  BackgroundSyncManager::GetDataFromBackend(key, std::move(callback));
}

}  // namespace content

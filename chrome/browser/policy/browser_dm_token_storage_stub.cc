// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/browser_dm_token_storage_stub.h"

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"

namespace policy {

namespace {
BrowserDMTokenStorage* g_browser_dm_token_storage = nullptr;
}  // namespace

// static
BrowserDMTokenStorage* BrowserDMTokenStorage::storage_for_testing_ = nullptr;

// static
BrowserDMTokenStorage* BrowserDMTokenStorage::Get() {
  if (storage_for_testing_)
    return storage_for_testing_;
  if (g_browser_dm_token_storage == nullptr)
    g_browser_dm_token_storage = new BrowserDMTokenStorageStub();
  return g_browser_dm_token_storage;
}

std::string BrowserDMTokenStorageStub::RetrieveClientId() {
  return std::string();
}

std::string BrowserDMTokenStorageStub::RetrieveEnrollmentToken() {
  return std::string();
}

void BrowserDMTokenStorageStub::StoreDMToken(const std::string& dm_token,
                                             StoreCallback callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(callback), false));
}

std::string BrowserDMTokenStorageStub::RetrieveDMToken() {
  return std::string();
}

}  // namespace policy

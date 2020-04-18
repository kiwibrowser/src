// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_TEST_UTILS_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_TEST_UTILS_H_

#include <memory>
#include <string>
#include <vector>

#include "build/build_config.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

using sync_preferences::TestingPrefServiceSyncable;

namespace ntp_snippets {

namespace test {

class FakeSyncService : public syncer::FakeSyncService {
 public:
  FakeSyncService();
  ~FakeSyncService() override;

  bool CanSyncStart() const override;
  bool IsSyncActive() const override;
  bool ConfigurationDone() const override;
  bool IsEncryptEverythingEnabled() const override;
  syncer::ModelTypeSet GetActiveDataTypes() const override;

  bool can_sync_start_;
  bool is_sync_active_;
  bool configuration_done_;
  bool is_encrypt_everything_enabled_;
  syncer::ModelTypeSet active_data_types_;
};

// Common utilities for remote suggestion tests, handles initializing fakes for
// sync and signin.
class RemoteSuggestionsTestUtils {
 public:
  RemoteSuggestionsTestUtils();
  ~RemoteSuggestionsTestUtils();

  void ResetSigninManager();

  FakeSyncService* fake_sync_service() { return fake_sync_service_.get(); }
  TestingPrefServiceSyncable* pref_service() { return pref_service_.get(); }

 private:
  std::unique_ptr<FakeSyncService> fake_sync_service_;
  std::unique_ptr<TestingPrefServiceSyncable> pref_service_;
};

}  // namespace test

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_TEST_UTILS_H_

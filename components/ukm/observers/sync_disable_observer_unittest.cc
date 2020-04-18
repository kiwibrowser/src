// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/observers/sync_disable_observer.h"

#include "base/observer_list.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/driver/sync_token_status.h"
#include "components/sync/engine/connection_status.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ukm {

namespace {

class MockSyncService : public syncer::FakeSyncService {
 public:
  MockSyncService() {}
  ~MockSyncService() override { Shutdown(); }

  void SetStatus(bool has_passphrase, bool enabled) {
    initialized_ = true;
    has_passphrase_ = has_passphrase;
    preferred_data_types_ =
        enabled ? syncer::ModelTypeSet(syncer::HISTORY_DELETE_DIRECTIVES)
                : syncer::ModelTypeSet();
    NotifyObserversOfStateChanged();
  }

  void SetConnectionStatus(syncer::ConnectionStatus status) {
    connection_status_ = status;
    NotifyObserversOfStateChanged();
  }

  void Shutdown() override {
    for (auto& observer : observers_) {
      observer.OnSyncShutdown(this);
    }
  }

  void NotifyObserversOfStateChanged() {
    for (auto& observer : observers_) {
      observer.OnStateChanged(this);
    }
  }

 private:
  // syncer::FakeSyncService:
  void AddObserver(syncer::SyncServiceObserver* observer) override {
    observers_.AddObserver(observer);
  }
  void RemoveObserver(syncer::SyncServiceObserver* observer) override {
    observers_.RemoveObserver(observer);
  }
  bool IsEngineInitialized() const override { return initialized_; }
  bool IsUsingSecondaryPassphrase() const override { return has_passphrase_; }
  syncer::ModelTypeSet GetPreferredDataTypes() const override {
    return preferred_data_types_;
  }
  syncer::SyncTokenStatus GetSyncTokenStatus() const override {
    syncer::SyncTokenStatus status;
    status.connection_status = connection_status_;
    return status;
  }

  bool initialized_ = false;
  bool has_passphrase_ = false;
  syncer::ConnectionStatus connection_status_ = syncer::CONNECTION_OK;
  syncer::ModelTypeSet preferred_data_types_;

  // The list of observers of the SyncService state.
  base::ObserverList<syncer::SyncServiceObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(MockSyncService);
};

class TestSyncDisableObserver : public SyncDisableObserver {
 public:
  TestSyncDisableObserver() : purged_(false), notified_(false) {}
  ~TestSyncDisableObserver() override {}

  bool ResetPurged() {
    bool was_purged = purged_;
    purged_ = false;
    return was_purged;
  }

  bool ResetNotified() {
    bool notified = notified_;
    notified_ = false;
    return notified;
  }

 private:
  // SyncDisableObserver:
  void OnSyncPrefsChanged(bool must_purge) override {
    notified_ = true;
    purged_ = purged_ || must_purge;
  }
  bool purged_;
  bool notified_;
  DISALLOW_COPY_AND_ASSIGN(TestSyncDisableObserver);
};

class SyncDisableObserverTest : public testing::Test {
 public:
  SyncDisableObserverTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncDisableObserverTest);
};

}  // namespace

TEST_F(SyncDisableObserverTest, NoProfiles) {
  TestSyncDisableObserver observer;
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, OneEnabled) {
  TestSyncDisableObserver observer;
  MockSyncService sync;
  sync.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync);
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, Passphrase) {
  TestSyncDisableObserver observer;
  MockSyncService sync;
  sync.SetStatus(true, true);
  observer.ObserveServiceForSyncDisables(&sync);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, HistoryDisabled) {
  TestSyncDisableObserver observer;
  MockSyncService sync;
  sync.SetStatus(false, false);
  observer.ObserveServiceForSyncDisables(&sync);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, AuthError) {
  TestSyncDisableObserver observer;
  MockSyncService sync;
  sync.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync);
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
  sync.SetConnectionStatus(syncer::CONNECTION_AUTH_ERROR);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  sync.SetConnectionStatus(syncer::CONNECTION_OK);
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
}

TEST_F(SyncDisableObserverTest, MixedProfiles1) {
  TestSyncDisableObserver observer;
  MockSyncService sync1;
  sync1.SetStatus(false, false);
  observer.ObserveServiceForSyncDisables(&sync1);
  MockSyncService sync2;
  sync2.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync2);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, MixedProfiles2) {
  TestSyncDisableObserver observer;
  MockSyncService sync1;
  sync1.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync1);
  EXPECT_TRUE(observer.ResetNotified());
  MockSyncService sync2;
  sync2.SetStatus(false, false);
  observer.ObserveServiceForSyncDisables(&sync2);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
  sync2.Shutdown();
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, TwoEnabled) {
  TestSyncDisableObserver observer;
  MockSyncService sync1;
  sync1.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync1);
  EXPECT_TRUE(observer.ResetNotified());
  MockSyncService sync2;
  sync2.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync2);
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, OneAddRemove) {
  TestSyncDisableObserver observer;
  MockSyncService sync;
  observer.ObserveServiceForSyncDisables(&sync);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
  sync.SetStatus(false, true);
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
  sync.Shutdown();
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

TEST_F(SyncDisableObserverTest, PurgeOnDisable) {
  TestSyncDisableObserver observer;
  MockSyncService sync;
  sync.SetStatus(false, true);
  observer.ObserveServiceForSyncDisables(&sync);
  EXPECT_TRUE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
  sync.SetStatus(false, false);
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_TRUE(observer.ResetNotified());
  EXPECT_TRUE(observer.ResetPurged());
  sync.Shutdown();
  EXPECT_FALSE(observer.SyncStateAllowsUkm());
  EXPECT_FALSE(observer.ResetNotified());
  EXPECT_FALSE(observer.ResetPurged());
}

}  // namespace ukm

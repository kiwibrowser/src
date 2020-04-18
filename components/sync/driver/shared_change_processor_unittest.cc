// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/shared_change_processor.h"

#include <cstddef>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "components/sync/base/model_type.h"
#include "components/sync/device_info/local_device_info_provider.h"
#include "components/sync/driver/data_type_manager.h"
#include "components/sync/driver/fake_sync_client.h"
#include "components/sync/driver/generic_change_processor.h"
#include "components/sync/driver/generic_change_processor_factory.h"
#include "components/sync/driver/sync_api_component_factory.h"
#include "components/sync/engine/sync_engine.h"
#include "components/sync/model/data_type_error_handler_mock.h"
#include "components/sync/model/fake_syncable_service.h"
#include "components/sync/syncable/test_user_share.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

namespace {

using ::testing::NiceMock;
using ::testing::StrictMock;

class TestSyncApiComponentFactory : public SyncApiComponentFactory {
 public:
  TestSyncApiComponentFactory() {}
  ~TestSyncApiComponentFactory() override {}

  // SyncApiComponentFactory implementation.
  void RegisterDataTypes(
      SyncService* sync_service,
      const RegisterDataTypesMethod& register_platform_types_method) override {}
  std::unique_ptr<DataTypeManager> CreateDataTypeManager(
      ModelTypeSet initial_types,
      const WeakHandle<DataTypeDebugInfoListener>& debug_info_listener,
      const DataTypeController::TypeMap* controllers,
      const DataTypeEncryptionHandler* encryption_handler,
      ModelTypeConfigurer* configurer,
      DataTypeManagerObserver* observer) override {
    return nullptr;
  }
  std::unique_ptr<SyncEngine> CreateSyncEngine(
      const std::string& name,
      invalidation::InvalidationService* invalidator,
      const base::WeakPtr<SyncPrefs>& sync_prefs,
      const base::FilePath& sync_folder) override {
    return nullptr;
  }
  std::unique_ptr<LocalDeviceInfoProvider> CreateLocalDeviceInfoProvider()
      override {
    return nullptr;
  }
  SyncApiComponentFactory::SyncComponents CreateBookmarkSyncComponents(
      SyncService* sync_service,
      std::unique_ptr<DataTypeErrorHandler> error_handler) override {
    return SyncApiComponentFactory::SyncComponents(nullptr, nullptr);
  }
};

class SyncSharedChangeProcessorTest : public testing::Test,
                                      public FakeSyncClient {
 public:
  SyncSharedChangeProcessorTest()
      : FakeSyncClient(&factory_),
        model_thread_("dbthread"),
        did_connect_(false) {}

  ~SyncSharedChangeProcessorTest() override {
    EXPECT_FALSE(db_syncable_service_);
  }

  // FakeSyncClient override.
  base::WeakPtr<SyncableService> GetSyncableServiceForType(
      ModelType type) override {
    return db_syncable_service_->AsWeakPtr();
  }

 protected:
  void SetUp() override {
    test_user_share_.SetUp();
    shared_change_processor_ = new SharedChangeProcessor(AUTOFILL);
    ASSERT_TRUE(model_thread_.Start());
    ASSERT_TRUE(model_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&SyncSharedChangeProcessorTest::SetUpDBSyncableService,
                   base::Unretained(this))));
  }

  void TearDown() override {
    EXPECT_TRUE(model_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&SyncSharedChangeProcessorTest::TearDownDBSyncableService,
                   base::Unretained(this))));
    // This must happen before the DB thread is stopped since
    // |shared_change_processor_| may post tasks to delete its members
    // on the correct thread.
    //
    // TODO(akalin): Write deterministic tests for the destruction of
    // |shared_change_processor_| on the UI and DB threads.
    shared_change_processor_ = nullptr;
    model_thread_.Stop();

    // Note: Stop() joins the threads, and that barrier prevents this read
    // from being moved (e.g by compiler optimization) in such a way that it
    // would race with the write in ConnectOnDBThread (because by this time,
    // everything that could have run on |model_thread_| has done so).
    ASSERT_TRUE(did_connect_);
    test_user_share_.TearDown();
  }

  // Connect |shared_change_processor_| on the DB thread.
  void Connect() {
    EXPECT_TRUE(model_thread_.task_runner()->PostTask(
        FROM_HERE,
        base::Bind(&SyncSharedChangeProcessorTest::ConnectOnDBThread,
                   base::Unretained(this), shared_change_processor_)));
  }

 private:
  // Used by SetUp().
  void SetUpDBSyncableService() {
    DCHECK(model_thread_.task_runner()->BelongsToCurrentThread());
    DCHECK(!db_syncable_service_);
    db_syncable_service_ = std::make_unique<FakeSyncableService>();
  }

  // Used by TearDown().
  void TearDownDBSyncableService() {
    DCHECK(model_thread_.task_runner()->BelongsToCurrentThread());
    DCHECK(db_syncable_service_);
    db_syncable_service_.reset();
  }

  // Used by Connect().  The SharedChangeProcessor is passed in
  // because we modify |shared_change_processor_| on the main thread
  // (in TearDown()).
  void ConnectOnDBThread(
      const scoped_refptr<SharedChangeProcessor>& shared_change_processor) {
    DCHECK(model_thread_.task_runner()->BelongsToCurrentThread());
    EXPECT_TRUE(shared_change_processor->Connect(
        this, &processor_factory_, test_user_share_.user_share(),
        std::make_unique<DataTypeErrorHandlerMock>(),
        base::WeakPtr<SyncMergeResult>()));
    did_connect_ = true;
  }

  base::MessageLoop frontend_loop_;
  base::Thread model_thread_;
  TestUserShare test_user_share_;
  TestSyncApiComponentFactory factory_;

  scoped_refptr<SharedChangeProcessor> shared_change_processor_;

  GenericChangeProcessorFactory processor_factory_;
  bool did_connect_;

  // Used only on DB thread.
  std::unique_ptr<FakeSyncableService> db_syncable_service_;
};

// Simply connect the shared change processor.  It should succeed, and
// nothing further should happen.
TEST_F(SyncSharedChangeProcessorTest, Basic) {
  Connect();
}

}  // namespace

}  // namespace syncer

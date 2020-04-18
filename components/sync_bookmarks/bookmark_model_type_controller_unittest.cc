// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/bookmark_model_type_controller.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/test_bookmark_client.h"
#include "components/history/core/browser/history_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/data_type_controller.h"
#include "components/sync/driver/data_type_controller_mock.h"
#include "components/sync/driver/fake_sync_client.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/engine/model_type_configurer.h"
#include "components/sync/model/sync_error.h"
#include "components/sync/model/sync_merge_result.h"
#include "components/sync/syncable/directory.h"
#include "components/sync/syncable/test_user_share.h"
#include "components/sync/test/engine/test_syncable_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using syncer::DataTypeController;
using syncer::ModelType;
using syncer::SyncService;
using syncer::UserShare;
using testing::_;

namespace sync_bookmarks {

namespace {

// Fake specializations for BookmarModelTypeController's external dependencies.

class TestHistoryService : public history::HistoryService {
 public:
  bool BackendLoaded() override { return true; }
};

class TestSyncClient : public syncer::FakeSyncClient {
 public:
  TestSyncClient(bookmarks::BookmarkModel* bookmark_model,
                 history::HistoryService* history_service,
                 SyncService* sync_service)
      : bookmark_model_(bookmark_model),
        history_service_(history_service),
        sync_service_(sync_service) {}

  bookmarks::BookmarkModel* GetBookmarkModel() override {
    return bookmark_model_;
  }

  history::HistoryService* GetHistoryService() override {
    return history_service_;
  }

  SyncService* GetSyncService() override { return sync_service_; }

 private:
  bookmarks::BookmarkModel* bookmark_model_;
  history::HistoryService* history_service_;
  SyncService* sync_service_;
};

class TestSyncService : public syncer::FakeSyncService {
 public:
  explicit TestSyncService(UserShare* user_share) : user_share_(user_share) {}

  UserShare* GetUserShare() const override { return user_share_; }

 private:
  UserShare* user_share_;
};

class TestModelTypeConfigurer : public syncer::ModelTypeConfigurer {
 public:
  TestModelTypeConfigurer() {}
  ~TestModelTypeConfigurer() override {}

  void ConfigureDataTypes(ConfigureParams params) override {}

  void RegisterDirectoryDataType(ModelType type,
                                 syncer::ModelSafeGroup group) override {}

  void UnregisterDirectoryDataType(ModelType type) override {}

  void ActivateDirectoryDataType(
      ModelType type,
      syncer::ModelSafeGroup group,
      syncer::ChangeProcessor* change_processor) override {}

  void DeactivateDirectoryDataType(ModelType type) override {}

  void ActivateNonBlockingDataType(
      ModelType type,
      std::unique_ptr<syncer::ActivationContext> activation_context) override {
    activation_context_ = std::move(activation_context);
  }

  void DeactivateNonBlockingDataType(ModelType type) override {}

  syncer::ActivationContext* activation_context() {
    return activation_context_.get();
  }

 private:
  // ActivationContext captured in ActivateNonBlockingDataType call.
  std::unique_ptr<syncer::ActivationContext> activation_context_;
};

}  // namespace

class BookmarkModelTypeControllerTest : public testing::Test {
 public:
  void SetUp() override {
    bookmark_model_ = bookmarks::TestBookmarkClient::CreateModel();
    history_service_ = std::make_unique<TestHistoryService>();
    test_user_share_.SetUp();
    sync_service_ =
        std::make_unique<TestSyncService>(test_user_share_.user_share());
    sync_client_ = std::make_unique<TestSyncClient>(
        bookmark_model_.get(), history_service_.get(), sync_service_.get());
    controller_ =
        std::make_unique<BookmarkModelTypeController>(sync_client_.get());
  }

  void TearDown() override { test_user_share_.TearDown(); }

 protected:
  BookmarkModelTypeController* controller() { return controller_.get(); }

  syncer::UserShare* user_share() { return test_user_share_.user_share(); }

  syncer::ModelLoadCallbackMock& model_load_callback() {
    return model_load_callback_;
  }

  syncer::StartCallbackMock& start_callback() { return start_callback_; }

  syncer::ActivationContext* activation_context() {
    return model_type_configurer_.activation_context();
  }

  void LoadModels() {
    controller()->LoadModels(
        base::Bind(&syncer::ModelLoadCallbackMock::Run,
                   base::Unretained(&model_load_callback_)));
  }

  static void CaptureBoolean(bool* value_dest, bool value) {
    *value_dest = value;
  }

  void CallRegisterWithBackend(bool* initial_sync_done) {
    controller()->RegisterWithBackend(
        base::Bind(&BookmarkModelTypeControllerTest::CaptureBoolean,
                   initial_sync_done),
        &model_type_configurer_);
  }

  void StartAssociating() {
    controller()->StartAssociating(base::Bind(
        &syncer::StartCallbackMock::Run, base::Unretained(&start_callback_)));
  }

 private:
  base::MessageLoop message_loop_;

  std::unique_ptr<bookmarks::BookmarkModel> bookmark_model_;
  std::unique_ptr<history::HistoryService> history_service_;
  syncer::TestUserShare test_user_share_;
  std::unique_ptr<SyncService> sync_service_;
  std::unique_ptr<TestSyncClient> sync_client_;
  TestModelTypeConfigurer model_type_configurer_;

  syncer::ModelLoadCallbackMock model_load_callback_;
  syncer::StartCallbackMock start_callback_;

  std::unique_ptr<BookmarkModelTypeController> controller_;
};

// Tests model type and initial state of bookmarks controller.
TEST_F(BookmarkModelTypeControllerTest, InitialState) {
  EXPECT_EQ(syncer::BOOKMARKS, controller()->type());
  EXPECT_EQ(DataTypeController::NOT_RUNNING, controller()->state());
  EXPECT_TRUE(controller()->ShouldLoadModelBeforeConfigure());
}

// Tests that call to LoadModels triggers ModelLoadCallback and advances DTC
// state.
TEST_F(BookmarkModelTypeControllerTest, LoadModels) {
  EXPECT_CALL(model_load_callback(), Run(_, _));
  LoadModels();
  EXPECT_EQ(DataTypeController::MODEL_LOADED, controller()->state());
}

// Tests that registering with backend from clean state reports that initial
// sync is not done and progress marker is empty.
TEST_F(BookmarkModelTypeControllerTest, RegisterWithBackend_CleanState) {
  LoadModels();
  bool initial_sync_done = false;
  CallRegisterWithBackend(&initial_sync_done);
  EXPECT_FALSE(initial_sync_done);
  EXPECT_FALSE(activation_context()->model_type_state.initial_sync_done());
  EXPECT_TRUE(
      activation_context()->model_type_state.progress_marker().token().empty());
  EXPECT_NE(nullptr, activation_context()->type_processor);
}

// Tests that registering with backend from valid state returns non-empty
// progress marker.
TEST_F(BookmarkModelTypeControllerTest, RegisterWithBackend) {
  syncer::TestUserShare::CreateRoot(syncer::BOOKMARKS, user_share());
  sync_pb::DataTypeProgressMarker progress_marker =
      syncer::syncable::BuildProgress(syncer::BOOKMARKS);
  user_share()->directory->SetDownloadProgress(syncer::BOOKMARKS,
                                               progress_marker);
  LoadModels();
  bool initial_sync_done = false;
  CallRegisterWithBackend(&initial_sync_done);
  EXPECT_TRUE(initial_sync_done);
  EXPECT_TRUE(activation_context()->model_type_state.initial_sync_done());
  EXPECT_EQ(progress_marker.SerializeAsString(),
            activation_context()
                ->model_type_state.progress_marker()
                .SerializeAsString());
  EXPECT_NE(nullptr, activation_context()->type_processor);
}

// Tests that call to StartAssociating triggers StartCallback and adjusts DTC
// state.
TEST_F(BookmarkModelTypeControllerTest, StartAssociating) {
  LoadModels();
  EXPECT_CALL(start_callback(), Run(_, _, _));
  StartAssociating();
  EXPECT_EQ(DataTypeController::RUNNING, controller()->state());
}

}  // namespace sync_bookmarks

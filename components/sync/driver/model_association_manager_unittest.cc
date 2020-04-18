// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/model_association_manager.h"

#include <memory>

#include "base/callback.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/sync/driver/fake_data_type_controller.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace syncer {

class MockModelAssociationManagerDelegate
    : public ModelAssociationManagerDelegate {
 public:
  MockModelAssociationManagerDelegate() {}
  ~MockModelAssociationManagerDelegate() override {}
  MOCK_METHOD0(OnAllDataTypesReadyForConfigure, void());
  MOCK_METHOD2(OnSingleDataTypeAssociationDone,
               void(ModelType type,
                    const DataTypeAssociationStats& association_stats));
  MOCK_METHOD1(OnSingleDataTypeWillStart, void(ModelType type));
  MOCK_METHOD2(OnSingleDataTypeWillStop,
               void(ModelType, const SyncError& error));
  MOCK_METHOD1(OnModelAssociationDone,
               void(const DataTypeManager::ConfigureResult& result));
};

FakeDataTypeController* GetController(
    const DataTypeController::TypeMap& controllers,
    ModelType model_type) {
  DataTypeController::TypeMap::const_iterator it = controllers.find(model_type);
  if (it == controllers.end()) {
    return nullptr;
  }
  return static_cast<FakeDataTypeController*>(it->second.get());
}

ACTION_P(VerifyResult, expected_result) {
  EXPECT_EQ(arg0.status, expected_result.status);
  EXPECT_EQ(expected_result.requested_types, arg0.requested_types);
}

class SyncModelAssociationManagerTest : public testing::Test {
 public:
  SyncModelAssociationManagerTest() {}

 protected:
  base::MessageLoopForUI ui_loop_;
  MockModelAssociationManagerDelegate delegate_;
  DataTypeController::TypeMap controllers_;
};

// Start a type and make sure ModelAssociationManager callst the |Start|
// method and calls the callback when it is done.
TEST_F(SyncModelAssociationManagerTest, SimpleModelStart) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types(BOOKMARKS, APPS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure());
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::NOT_RUNNING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::NOT_RUNNING);

  // Initialize() kicks off model loading.
  model_association_manager.Initialize(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::MODEL_LOADED);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_LOADED);

  model_association_manager.StartAssociationAsync(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::ASSOCIATING);
  GetController(controllers_, BOOKMARKS)->FinishStart(DataTypeController::OK);
  GetController(controllers_, APPS)->FinishStart(DataTypeController::OK);
}

// Start a type and call stop before it finishes associating.
TEST_F(SyncModelAssociationManagerTest, StopModelBeforeFinish) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);

  ModelTypeSet types;
  types.Put(BOOKMARKS);

  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::ABORTED,
                                                   types);
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));

  model_association_manager.Initialize(types);
  model_association_manager.StartAssociationAsync(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  model_association_manager.Stop();
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::NOT_RUNNING);
}

// Start a type, let it finish and then call stop.
TEST_F(SyncModelAssociationManagerTest, StopAfterFinish) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));

  model_association_manager.Initialize(types);
  model_association_manager.StartAssociationAsync(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  GetController(controllers_, BOOKMARKS)->FinishStart(DataTypeController::OK);

  model_association_manager.Stop();
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::NOT_RUNNING);
}

// Make a type fail model association and verify correctness.
TEST_F(SyncModelAssociationManagerTest, TypeFailModelAssociation) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));

  model_association_manager.Initialize(types);
  model_association_manager.StartAssociationAsync(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  GetController(controllers_, BOOKMARKS)
      ->FinishStart(DataTypeController::ASSOCIATION_FAILED);
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::NOT_RUNNING);
}

// Ensure configuring stops when a type returns a unrecoverable error.
TEST_F(SyncModelAssociationManagerTest, TypeReturnUnrecoverableError) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  DataTypeManager::ConfigureResult expected_result(
      DataTypeManager::UNRECOVERABLE_ERROR, types);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));

  model_association_manager.Initialize(types);

  model_association_manager.StartAssociationAsync(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  GetController(controllers_, BOOKMARKS)
      ->FinishStart(DataTypeController::UNRECOVERABLE_ERROR);
}

TEST_F(SyncModelAssociationManagerTest, SlowTypeAsFailedType) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  GetController(controllers_, BOOKMARKS)->SetDelayModelLoad();
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  types.Put(APPS);

  DataTypeManager::ConfigureResult expected_result_partially_done(
      DataTypeManager::OK, types);

  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result_partially_done));

  model_association_manager.Initialize(types);
  model_association_manager.StartAssociationAsync(types);
  GetController(controllers_, APPS)->FinishStart(DataTypeController::OK);

  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));
  model_association_manager.GetTimerForTesting()->user_task().Run();

  EXPECT_EQ(DataTypeController::NOT_RUNNING,
            GetController(controllers_, BOOKMARKS)->state());
}

TEST_F(SyncModelAssociationManagerTest, StartMultipleTimes) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  types.Put(APPS);

  DataTypeManager::ConfigureResult result_1st(DataTypeManager::OK,
                                              ModelTypeSet(BOOKMARKS));
  DataTypeManager::ConfigureResult result_2nd(DataTypeManager::OK,
                                              ModelTypeSet(APPS));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .Times(2)
      .WillOnce(VerifyResult(result_1st))
      .WillOnce(VerifyResult(result_2nd));

  model_association_manager.Initialize(types);

  // Start BOOKMARKS first.
  model_association_manager.StartAssociationAsync(ModelTypeSet(BOOKMARKS));
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_LOADED);

  // Finish BOOKMARKS association.
  GetController(controllers_, BOOKMARKS)->FinishStart(DataTypeController::OK);
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::RUNNING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_LOADED);

  // Start APPS next.
  model_association_manager.StartAssociationAsync(ModelTypeSet(APPS));
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::ASSOCIATING);
  GetController(controllers_, APPS)->FinishStart(DataTypeController::OK);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::RUNNING);
}

// Test that model that failed to load between initialization and association
// is reported and stopped properly.
TEST_F(SyncModelAssociationManagerTest, ModelLoadFailBeforeAssociationStart) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  GetController(controllers_, BOOKMARKS)
      ->SetModelLoadError(
          SyncError(FROM_HERE, SyncError::DATATYPE_ERROR, "", BOOKMARKS));
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));

  model_association_manager.Initialize(types);
  EXPECT_EQ(DataTypeController::NOT_RUNNING,
            GetController(controllers_, BOOKMARKS)->state());
  model_association_manager.StartAssociationAsync(types);
  EXPECT_EQ(DataTypeController::NOT_RUNNING,
            GetController(controllers_, BOOKMARKS)->state());
}

// Test that a runtime error is handled by stopping the type.
TEST_F(SyncModelAssociationManagerTest, StopAfterConfiguration) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result));

  model_association_manager.Initialize(types);
  model_association_manager.StartAssociationAsync(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);
  GetController(controllers_, BOOKMARKS)->FinishStart(DataTypeController::OK);
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::RUNNING);

  testing::Mock::VerifyAndClearExpectations(&delegate_);
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));
  SyncError error(FROM_HERE, SyncError::DATATYPE_ERROR, "error", BOOKMARKS);
  GetController(controllers_, BOOKMARKS)
      ->CreateErrorHandler()
      ->OnUnrecoverableError(error);
  base::RunLoop().RunUntilIdle();
}

TEST_F(SyncModelAssociationManagerTest, AbortDuringAssociation) {
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types;
  types.Put(BOOKMARKS);
  types.Put(APPS);

  ModelTypeSet expected_types_unfinished;
  expected_types_unfinished.Put(BOOKMARKS);
  DataTypeManager::ConfigureResult expected_result_partially_done(
      DataTypeManager::OK, types);

  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
  EXPECT_CALL(delegate_, OnModelAssociationDone(_))
      .WillOnce(VerifyResult(expected_result_partially_done));

  model_association_manager.Initialize(types);
  model_association_manager.StartAssociationAsync(types);
  GetController(controllers_, APPS)->FinishStart(DataTypeController::OK);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::RUNNING);
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::ASSOCIATING);

  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(BOOKMARKS, _));
  model_association_manager.GetTimerForTesting()->user_task().Run();

  EXPECT_EQ(DataTypeController::NOT_RUNNING,
            GetController(controllers_, BOOKMARKS)->state());
}

// Test that OnAllDataTypesReadyForConfigure is called when all datatypes that
// require LoadModels before configuration are loaded.
TEST_F(SyncModelAssociationManagerTest, OnAllDataTypesReadyForConfigure) {
  // Create two controllers with delayed model load.
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  GetController(controllers_, BOOKMARKS)->SetDelayModelLoad();
  GetController(controllers_, APPS)->SetDelayModelLoad();

  // APPS controller requires LoadModels complete before configure.
  GetController(controllers_, APPS)->SetShouldLoadModelBeforeConfigure(true);

  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types(BOOKMARKS, APPS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  // OnAllDataTypesReadyForConfigure shouldn't be called, APPS data type is not
  // loaded yet.
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure()).Times(0);

  model_association_manager.Initialize(types);

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::MODEL_STARTING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_STARTING);

  testing::Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure());
  // Finish loading APPS. This should trigger OnAllDataTypesReadyForConfigure
  // even though BOOKMARKS is not loaded yet.
  GetController(controllers_, APPS)->SimulateModelLoadFinishing();
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::MODEL_STARTING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_LOADED);

  // Call ModelAssociationManager::Initialize with reduced set of datatypes.
  // All datatypes in reduced set are already loaded.
  // OnAllDataTypesReadyForConfigure() should be called.
  testing::Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure());
  ModelTypeSet reduced_types(APPS);
  model_association_manager.Initialize(reduced_types);
}

// Test that OnAllDataTypesReadyForConfigure() is called correctly after
// LoadModels fails for one of datatypes.
TEST_F(SyncModelAssociationManagerTest,
       OnAllDataTypesReadyForConfigure_FailedLoadModels) {
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  GetController(controllers_, APPS)->SetDelayModelLoad();

  // APPS controller requires LoadModels complete before configure.
  GetController(controllers_, APPS)->SetShouldLoadModelBeforeConfigure(true);

  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types(APPS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  // OnAllDataTypesReadyForConfigure shouldn't be called, APPS data type is not
  // loaded yet.
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure()).Times(0);

  model_association_manager.Initialize(types);

  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_STARTING);

  testing::Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure());
  // Simulate model load error for APPS and finish loading it. This should
  // trigger OnAllDataTypesReadyForConfigure.
  GetController(controllers_, APPS)
      ->SetModelLoadError(
          SyncError(FROM_HERE, SyncError::DATATYPE_ERROR, "", APPS));
  GetController(controllers_, APPS)->SimulateModelLoadFinishing();
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::NOT_RUNNING);
}

// Test that if one of the types fails while another is still being loaded then
// OnAllDataTypesReadyForConfgiure is still called correctly.
TEST_F(SyncModelAssociationManagerTest,
       OnAllDataTypesReadyForConfigure_TypeFailedAfterLoadModels) {
  // Create two controllers with delayed model load. Both should block
  // configuration.
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);
  GetController(controllers_, BOOKMARKS)->SetDelayModelLoad();
  GetController(controllers_, APPS)->SetDelayModelLoad();

  GetController(controllers_, BOOKMARKS)
      ->SetShouldLoadModelBeforeConfigure(true);
  GetController(controllers_, APPS)->SetShouldLoadModelBeforeConfigure(true);

  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types(BOOKMARKS, APPS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);

  // Apps will finish loading but bookmarks won't.
  // OnAllDataTypesReadyForConfigure shouldn't be called.
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
  EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure()).Times(0);

  model_association_manager.Initialize(types);

  GetController(controllers_, APPS)->SimulateModelLoadFinishing();

  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::MODEL_STARTING);
  EXPECT_EQ(GetController(controllers_, APPS)->state(),
            DataTypeController::MODEL_LOADED);

  testing::Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure()).Times(0);

  EXPECT_CALL(delegate_, OnSingleDataTypeWillStop(APPS, _));
  // Apps datatype reports failure.
  SyncError error(FROM_HERE, SyncError::DATATYPE_ERROR, "error", APPS);
  GetController(controllers_, APPS)
      ->CreateErrorHandler()
      ->OnUnrecoverableError(error);
  base::RunLoop().RunUntilIdle();

  testing::Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure());
  // Finish loading BOOKMARKS. This should trigger
  // OnAllDataTypesReadyForConfigure().
  GetController(controllers_, BOOKMARKS)->SimulateModelLoadFinishing();
  EXPECT_EQ(GetController(controllers_, BOOKMARKS)->state(),
            DataTypeController::MODEL_LOADED);
}

// Tests that OnAllDataTypesReadyForConfigure is only called after
// OnSingleDataTypeWillStart is called for all enabled types.
TEST_F(SyncModelAssociationManagerTest, TypeRegistrationCallSequence) {
  // Create two controllers and allow them to complete LoadModels synchronously.
  controllers_[BOOKMARKS] = std::make_unique<FakeDataTypeController>(BOOKMARKS);
  controllers_[APPS] = std::make_unique<FakeDataTypeController>(APPS);

  ModelAssociationManager model_association_manager(&controllers_, &delegate_);
  ModelTypeSet types(BOOKMARKS, APPS);
  DataTypeManager::ConfigureResult expected_result(DataTypeManager::OK, types);
  // OnAllDataTypesReadyForConfigure should only be called after calls to
  // OnSingleDataTypeWillStart for both enabled types.
  {
    ::testing::InSequence call_sequence;
    EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(BOOKMARKS));
    EXPECT_CALL(delegate_, OnSingleDataTypeWillStart(APPS));
    EXPECT_CALL(delegate_, OnAllDataTypesReadyForConfigure());
  }

  model_association_manager.Initialize(types);

  EXPECT_EQ(DataTypeController::MODEL_LOADED,
            GetController(controllers_, BOOKMARKS)->state());
  EXPECT_EQ(DataTypeController::MODEL_LOADED,
            GetController(controllers_, APPS)->state());
}

}  // namespace syncer

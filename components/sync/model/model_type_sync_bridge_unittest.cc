// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/model/model_type_sync_bridge.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "components/sync/model/conflict_resolution.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/mock_model_type_change_processor.h"
#include "components/sync/model/stub_model_type_sync_bridge.h"
#include "components/sync/protocol/model_type_state.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

using testing::Return;
using testing::_;

class ModelTypeSyncBridgeTest : public ::testing::Test {
 public:
  ModelTypeSyncBridgeTest()
      : bridge_(mock_processor_.CreateForwardingProcessor()) {}
  ~ModelTypeSyncBridgeTest() override {}

  StubModelTypeSyncBridge* bridge() { return &bridge_; }
  MockModelTypeChangeProcessor* processor() { return &mock_processor_; }

 private:
  testing::NiceMock<MockModelTypeChangeProcessor> mock_processor_;
  StubModelTypeSyncBridge bridge_;
};

// ResolveConflicts should return USE_REMOTE unless the remote data is deleted.
TEST_F(ModelTypeSyncBridgeTest, DefaultConflictResolution) {
  EntityData local_data;
  EntityData remote_data;

  // There is no deleted/deleted case because that's not a conflict.

  local_data.specifics.mutable_preference()->set_value("value");
  EXPECT_FALSE(local_data.is_deleted());
  EXPECT_TRUE(remote_data.is_deleted());
  EXPECT_EQ(ConflictResolution::USE_LOCAL,
            bridge()->ResolveConflict(local_data, remote_data).type());

  remote_data.specifics.mutable_preference()->set_value("value");
  EXPECT_FALSE(local_data.is_deleted());
  EXPECT_FALSE(remote_data.is_deleted());
  EXPECT_EQ(ConflictResolution::USE_REMOTE,
            bridge()->ResolveConflict(local_data, remote_data).type());

  local_data.specifics.clear_preference();
  EXPECT_TRUE(local_data.is_deleted());
  EXPECT_FALSE(remote_data.is_deleted());
  EXPECT_EQ(ConflictResolution::USE_REMOTE,
            bridge()->ResolveConflict(local_data, remote_data).type());
}

}  // namespace
}  // namespace syncer

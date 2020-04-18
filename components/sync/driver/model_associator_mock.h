// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_MODEL_ASSOCIATOR_MOCK_H__
#define COMPONENTS_SYNC_DRIVER_MODEL_ASSOCIATOR_MOCK_H__

#include "base/location.h"
#include "components/sync/driver/model_associator.h"
#include "components/sync/model/sync_error.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace syncer {

ACTION_P(SetSyncError, type) {
  arg0->Reset(FROM_HERE, "test", type);
}

class ModelAssociatorMock : public AssociatorInterface {
 public:
  ModelAssociatorMock();
  ~ModelAssociatorMock() override;

  MOCK_METHOD2(AssociateModels, SyncError(SyncMergeResult*, SyncMergeResult*));
  MOCK_METHOD0(DisassociateModels, SyncError());
  MOCK_METHOD1(SyncModelHasUserCreatedNodes, bool(bool* has_nodes));
  MOCK_METHOD0(AbortAssociation, void());
  MOCK_METHOD0(CryptoReadyIfNecessary, bool());
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_MODEL_ASSOCIATOR_MOCK_H__

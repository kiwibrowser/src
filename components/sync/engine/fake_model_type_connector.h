// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_ENGINE_FAKE_MODEL_TYPE_CONNECTOR_H_
#define COMPONENTS_SYNC_ENGINE_FAKE_MODEL_TYPE_CONNECTOR_H_

#include <memory>

#include "components/sync/engine/model_type_connector.h"

namespace syncer {

// A no-op implementation of ModelTypeConnector for testing.
class FakeModelTypeConnector : public ModelTypeConnector {
 public:
  FakeModelTypeConnector();
  ~FakeModelTypeConnector() override;

  void ConnectNonBlockingType(
      ModelType type,
      std::unique_ptr<ActivationContext> activation_context) override;
  void DisconnectNonBlockingType(ModelType type) override;
  void RegisterDirectoryType(ModelType type, ModelSafeGroup group) override;
  void UnregisterDirectoryType(ModelType type) override;
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_ENGINE_FAKE_MODEL_TYPE_CONNECTOR_H_

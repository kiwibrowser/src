// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/engine/fake_model_type_connector.h"

#include "components/sync/engine/activation_context.h"

namespace syncer {

FakeModelTypeConnector::FakeModelTypeConnector() {}

FakeModelTypeConnector::~FakeModelTypeConnector() {}

void FakeModelTypeConnector::ConnectNonBlockingType(
    ModelType type,
    std::unique_ptr<ActivationContext> activation_context) {}

void FakeModelTypeConnector::DisconnectNonBlockingType(ModelType type) {}

void FakeModelTypeConnector::RegisterDirectoryType(ModelType type,
                                                   ModelSafeGroup group) {}

void FakeModelTypeConnector::UnregisterDirectoryType(ModelType type) {}

}  // namespace syncer

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/core/base_session_service_test_helper.h"

#include "components/sessions/core/base_session_service.h"
#include "components/sessions/core/session_backend.h"

namespace sessions {

BaseSessionServiceTestHelper::BaseSessionServiceTestHelper(
    BaseSessionService* base_session_service)
    : base_session_service_(base_session_service) {
  CHECK(base_session_service);
}

BaseSessionServiceTestHelper::~BaseSessionServiceTestHelper() {
}

void BaseSessionServiceTestHelper::RunTaskOnBackendThread(
    const base::Location& from_here,
    const base::Closure& task) {
  base_session_service_->RunTaskOnBackendThread(from_here, task);
}

bool BaseSessionServiceTestHelper::ProcessedAnyCommands() {
  return base_session_service_->backend_->inited() ||
         !base_session_service_->pending_commands().empty();
}

bool BaseSessionServiceTestHelper::ReadLastSessionCommands(
    std::vector<std::unique_ptr<SessionCommand>>* commands) {
  return base_session_service_->backend_->ReadLastSessionCommandsImpl(commands);
}

}  // namespace sessions

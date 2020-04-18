// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SESSIONS_CORE_BASE_SESSION_SERVICE_TEST_HELPER_H_
#define COMPONENTS_SESSIONS_CORE_BASE_SESSION_SERVICE_TEST_HELPER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/task/cancelable_task_tracker.h"

namespace sessions {
class SessionCommand;
class BaseSessionService;

class BaseSessionServiceTestHelper {
 public:
  explicit BaseSessionServiceTestHelper(
      BaseSessionService* base_session_service_);
  ~BaseSessionServiceTestHelper();

  // This posts the task to the SequencedWorkerPool, or run immediately
  // if the SequencedWorkerPool has been shutdown.
  void RunTaskOnBackendThread(const base::Location& from_here,
                              const base::Closure& task);

  // Returns true if any commands got processed yet - saved or queued.
  bool ProcessedAnyCommands();

  // Read the last session commands directly from file.
  bool ReadLastSessionCommands(
      std::vector<std::unique_ptr<SessionCommand>>* commands);

 private:
  BaseSessionService* base_session_service_;

  DISALLOW_COPY_AND_ASSIGN(BaseSessionServiceTestHelper);
};

}  // namespace sessions

#endif  // COMPONENTS_SESSIONS_CORE_BASE_SESSION_SERVICE_TEST_HELPER_H_

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_STUB_LOG_MANAGER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_STUB_LOG_MANAGER_H_

#include <string>

#include "base/macros.h"
#include "components/password_manager/core/browser/log_manager.h"

namespace password_manager {

// Use this in tests only, to provide a no-op implementation of LogManager.
class StubLogManager : public LogManager {
 public:
  StubLogManager() = default;
  ~StubLogManager() override = default;

 private:
  // LogManager
  void OnLogRouterAvailabilityChanged(bool router_can_be_used) override;
  void SetSuspended(bool suspended) override;
  void LogSavePasswordProgress(const std::string& text) const override;
  bool IsLoggingActive() const override;

  DISALLOW_COPY_AND_ASSIGN(StubLogManager);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_STUB_LOG_MANAGER_H_

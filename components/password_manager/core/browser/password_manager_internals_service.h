// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_INTERNALS_SERVICE_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_INTERNALS_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/password_manager/core/browser/log_router.h"

namespace password_manager {

// Collects the logs for the password manager internals page and distributes
// them to all open tabs with the internals page.
class PasswordManagerInternalsService : public KeyedService, public LogRouter {
 public:
  // There are only two ways in which the service depends on the BrowserContext:
  // 1) There is one service per each non-incognito BrowserContext.
  // 2) No service will be created for an incognito BrowserContext.
  // Both properties are guarantied by the BrowserContextKeyedFactory framework,
  // so the service itself does not need the context on creation.
  PasswordManagerInternalsService();
  ~PasswordManagerInternalsService() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PasswordManagerInternalsService);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_INTERNALS_SERVICE_H_

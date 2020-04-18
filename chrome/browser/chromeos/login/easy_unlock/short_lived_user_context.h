// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_SHORT_LIVED_USER_CONTEXT_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_SHORT_LIVED_USER_CONTEXT_H_

#include <memory>

#include "apps/app_lifetime_monitor.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace base {
class TaskRunner;
}

namespace chromeos {

class UserContext;

// Stores the UserContext of an authentication operation on the sign-in/lock
// screen, which is used to generate the keys for Easy Sign-in.
// The lifetime of the user context is bound the the setup app window. As a
// fail-safe, the user context will also be deleted after a set period of time
// in case the app is left open indefintely.
class ShortLivedUserContext : public apps::AppLifetimeMonitor::Observer {
 public:
  ShortLivedUserContext(const UserContext& user_context,
                        apps::AppLifetimeMonitor* app_lifetime_monitor,
                        base::TaskRunner* task_runner);
  ~ShortLivedUserContext() override;

  // The UserContext returned here can be NULL if its time-to-live has expired.
  UserContext* user_context() { return user_context_.get(); }

 private:
  void Reset();

  // apps::AppLifetimeMonitor::Observer:
  void OnAppDeactivated(content::BrowserContext* context,
                        const std::string& app_id) override;

  std::unique_ptr<UserContext> user_context_;

  apps::AppLifetimeMonitor* app_lifetime_monitor_;

  base::WeakPtrFactory<ShortLivedUserContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShortLivedUserContext);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_EASY_UNLOCK_SHORT_LIVED_USER_CONTEXT_H_

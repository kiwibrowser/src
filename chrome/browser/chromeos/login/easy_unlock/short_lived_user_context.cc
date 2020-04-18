// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/easy_unlock/short_lived_user_context.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/task_runner.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chromeos/login/auth/user_context.h"

namespace chromeos {

namespace {

// The number of minutes that the user context will be stored.
const int64_t kUserContextTimeToLiveMinutes = 10;

}  // namespace

ShortLivedUserContext::ShortLivedUserContext(
    const UserContext& user_context,
    apps::AppLifetimeMonitor* app_lifetime_monitor,
    base::TaskRunner* task_runner)
    : user_context_(new UserContext(user_context)),
      app_lifetime_monitor_(app_lifetime_monitor),
      weak_ptr_factory_(this) {
  app_lifetime_monitor_->AddObserver(this);

  task_runner->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&ShortLivedUserContext::Reset,
                     weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromMinutes(kUserContextTimeToLiveMinutes));
}

ShortLivedUserContext::~ShortLivedUserContext() {
  Reset();
}

void ShortLivedUserContext::Reset() {
  if (user_context_.get()) {
    user_context_->ClearSecrets();
    user_context_.reset();
    app_lifetime_monitor_->RemoveObserver(this);
  }
}

void ShortLivedUserContext::OnAppDeactivated(content::BrowserContext* context,
                                             const std::string& app_id) {
  if (app_id == extension_misc::kEasyUnlockAppId)
    Reset();
}

}  // namespace chromeos

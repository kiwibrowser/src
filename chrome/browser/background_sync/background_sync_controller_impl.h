// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_CONTROLLER_IMPL_H_
#define CHROME_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_CONTROLLER_IMPL_H_

#include "content/public/browser/background_sync_controller.h"

#include <stdint.h>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_thread.h"

namespace content {
struct BackgroundSyncParameters;
}

namespace rappor {
class RapporServiceImpl;
}

class Profile;

class BackgroundSyncControllerImpl : public content::BackgroundSyncController,
                                     public KeyedService {
 public:
  static const char kFieldTrialName[];
  static const char kDisabledParameterName[];
  static const char kMaxAttemptsParameterName[];
  static const char kInitialRetryParameterName[];
  static const char kRetryDelayFactorParameterName[];
  static const char kMinSyncRecoveryTimeName[];
  static const char kMaxSyncEventDurationName[];

  explicit BackgroundSyncControllerImpl(Profile* profile);
  ~BackgroundSyncControllerImpl() override;

  // content::BackgroundSyncController overrides.
  void GetParameterOverrides(
      content::BackgroundSyncParameters* parameters) const override;
  void NotifyBackgroundSyncRegistered(const GURL& origin) override;
  void RunInBackground(bool enabled, int64_t min_ms) override;

 protected:
  // Virtual for testing.
  virtual rappor::RapporServiceImpl* GetRapporServiceImpl();

 private:
  Profile* profile_;  // This object is owned by profile_.

  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncControllerImpl);
};

#endif  // CHROME_BROWSER_BACKGROUND_SYNC_BACKGROUND_SYNC_CONTROLLER_IMPL_H_

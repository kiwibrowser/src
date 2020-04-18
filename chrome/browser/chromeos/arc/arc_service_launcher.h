// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ARC_SERVICE_LAUNCHER_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ARC_SERVICE_LAUNCHER_H_

#include <memory>

#include "base/macros.h"

class Profile;

namespace arc {

class ArcPlayStoreEnabledPreferenceHandler;
class ArcServiceManager;
class ArcSessionManager;

// Detects ARC availability and launches ARC bridge service.
class ArcServiceLauncher {
 public:
  ArcServiceLauncher();
  ~ArcServiceLauncher();

  // Returns a global instance.
  static ArcServiceLauncher* Get();

  // Called just before most of BrowserContextKeyedService instance creation.
  // Set the given |profile| to ArcSessionManager, if the profile is allowed
  // to use ARC.
  void MaybeSetProfile(Profile* profile);

  // Called when the main profile is initialized after user logs in.
  void OnPrimaryUserProfilePrepared(Profile* profile);

  // Called after the main MessageLoop stops, and before the Profile is
  // destroyed.
  void Shutdown();

  // Resets internal state for testing. Specifically this needs to be
  // called if other profile needs to be used in the tests. In that case,
  // following this call, MaybeSetProfile() and
  // OnPrimaryUserProfilePrepared() should be called.
  void ResetForTesting();

 private:
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<ArcSessionManager> arc_session_manager_;
  std::unique_ptr<ArcPlayStoreEnabledPreferenceHandler>
      arc_play_store_enabled_preference_handler_;

  DISALLOW_COPY_AND_ASSIGN(ArcServiceLauncher);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ARC_SERVICE_LAUNCHER_H_

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EOL_NOTIFICATION_H_
#define CHROME_BROWSER_CHROMEOS_EOL_NOTIFICATION_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "chrome/browser/profiles/profile.h"
#include "third_party/cros_system_api/dbus/update_engine/dbus-constants.h"

namespace chromeos {

// EolNotification is created when user logs in. It is
// used to check current EndOfLife Status of the device,
// and show notification accordingly.
class EolNotification final {
 public:
  // Returns true if the eol notification needs to be displayed.
  static bool ShouldShowEolNotification();

  explicit EolNotification(Profile* profile);
  ~EolNotification();

  // Check Eol status from update engine.
  void CheckEolStatus();

 private:
  // Callback invoked when |GetEolStatus()| has finished.
  void OnEolStatus(update_engine::EndOfLifeStatus status);

  // Create or updates the notfication.
  void Update();

  // Returns messages that applys to this eol status.
  base::string16 GetEolMessage();

  // Profile which is associated with the EndOfLife notification.
  Profile* const profile_;

  // Device EndOfLife status.
  update_engine::EndOfLifeStatus status_;

  // Factory of callbacks.
  base::WeakPtrFactory<EolNotification> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(EolNotification);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_EOL_NOTIFICATION_H_

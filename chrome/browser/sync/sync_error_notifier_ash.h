// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SYNC_ERROR_NOTIFIER_ASH_H_
#define CHROME_BROWSER_SYNC_SYNC_ERROR_NOTIFIER_ASH_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/driver/sync_error_controller.h"

class Profile;

// Shows sync-related errors as notifications in Ash.
class SyncErrorNotifier : public syncer::SyncErrorController::Observer,
                          public KeyedService {
 public:
  SyncErrorNotifier(syncer::SyncErrorController* controller, Profile* profile);
  ~SyncErrorNotifier() override;

  // KeyedService:
  void Shutdown() override;

  // syncer::SyncErrorController::Observer:
  void OnErrorChanged() override;

 private:
  // The error controller to query for error details.
  syncer::SyncErrorController* error_controller_;

  // The Profile this service belongs to.
  Profile* profile_;

  // Notification was added to NotificationUIManager. This flag is used to
  // prevent displaying passphrase notification to user if they already saw (and
  // potentially dismissed) previous one.
  bool notification_displayed_;

  // Used to keep track of the message center notification.
  std::string notification_id_;

  DISALLOW_COPY_AND_ASSIGN(SyncErrorNotifier);
};

#endif  // CHROME_BROWSER_SYNC_SYNC_ERROR_NOTIFIER_ASH_H_

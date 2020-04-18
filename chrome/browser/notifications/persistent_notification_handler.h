// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_PERSISTENT_NOTIFICATION_HANDLER_H_
#define CHROME_BROWSER_NOTIFICATIONS_PERSISTENT_NOTIFICATION_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/notifications/notification_handler.h"

// NotificationHandler implementation for persistent, service worker backed,
// notifications.
class PersistentNotificationHandler : public NotificationHandler {
 public:
  PersistentNotificationHandler();
  ~PersistentNotificationHandler() override;

  // NotificationHandler implementation.
  void OnClose(Profile* profile,
               const GURL& origin,
               const std::string& notification_id,
               bool by_user,
               base::OnceClosure completed_closure) override;
  void OnClick(Profile* profile,
               const GURL& origin,
               const std::string& notification_id,
               const base::Optional<int>& action_index,
               const base::Optional<base::string16>& reply,
               base::OnceClosure completed_closure) override;
  void DisableNotifications(Profile* profile, const GURL& origin) override;
  void OpenSettings(Profile* profile, const GURL& origin) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PersistentNotificationHandler);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_PERSISTENT_NOTIFICATION_HANDLER_H_

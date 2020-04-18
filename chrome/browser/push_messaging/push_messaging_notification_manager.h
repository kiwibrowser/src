// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_NOTIFICATION_MANAGER_H_
#define CHROME_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_NOTIFICATION_MANAGER_H_

#include <stdint.h>
#include <vector>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

class GURL;
class Profile;

namespace content {
struct NotificationDatabaseData;
struct PlatformNotificationData;
class WebContents;
}

// Developers may be required to display a Web Notification in response to an
// incoming push message in order to clarify to the user that something has
// happened in the background. When they forget to do so, a default notification
// has to be displayed on their behalf.
//
// This class implements the heuristics for determining whether the default
// notification is necessary, as well as the functionality of displaying the
// default notification when it is.
//
// See the following document and bug for more context:
// https://docs.google.com/document/d/13VxFdLJbMwxHrvnpDm8RXnU41W2ZlcP0mdWWe9zXQT8/edit
// https://crbug.com/437277
class PushMessagingNotificationManager {
 public:
  explicit PushMessagingNotificationManager(Profile* profile);
  ~PushMessagingNotificationManager();

  // Enforces the requirements implied for push subscriptions which must display
  // a Web Notification in response to an incoming message.
  void EnforceUserVisibleOnlyRequirements(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const base::Closure& message_handled_closure);

 private:
  FRIEND_TEST_ALL_PREFIXES(PushMessagingNotificationManagerTest, IsTabVisible);
  FRIEND_TEST_ALL_PREFIXES(PushMessagingNotificationManagerTest,
                           IsTabVisibleViewSource);

  static void DidGetNotificationsFromDatabaseIOProxy(
      const base::WeakPtr<PushMessagingNotificationManager>& ui_weak_ptr,
      const GURL& origin,
      int64_t service_worker_registration_id,
      const base::Closure& message_handled_closure,
      bool success,
      const std::vector<content::NotificationDatabaseData>& data);

  void DidGetNotificationsFromDatabase(
      const GURL& origin,
      int64_t service_worker_registration_id,
      const base::Closure& message_handled_closure,
      bool success,
      const std::vector<content::NotificationDatabaseData>& data);

  // Checks whether |profile| is the one owning this instance,
  // |active_web_contents| exists and its main frame is visible, and the URL
  // currently visible to the user is for |origin|.
  bool IsTabVisible(Profile* profile,
                    content::WebContents* active_web_contents,
                    const GURL& origin);

  void ProcessSilentPush(const GURL& origin,
                         int64_t service_worker_registration_id,
                         const base::Closure& message_handled_closure,
                         bool silent_push_allowed);

  static void DidWriteNotificationDataIOProxy(
      const base::WeakPtr<PushMessagingNotificationManager>& ui_weak_ptr,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const base::Closure& message_handled_closure,
      bool success,
      const std::string& notification_id);

  void DidWriteNotificationData(
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const base::Closure& message_handled_closure,
      bool success,
      const std::string& notification_id);

  // Weak. This manager is owned by a keyed service on this profile.
  Profile* profile_;

  base::WeakPtrFactory<PushMessagingNotificationManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingNotificationManager);
};

#endif  // CHROME_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_NOTIFICATION_MANAGER_H_

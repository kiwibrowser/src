// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_ARC_MIGRATION_GUIDE_NOTIFICATION_H_
#define CHROME_BROWSER_CHROMEOS_ARC_ARC_MIGRATION_GUIDE_NOTIFICATION_H_

class Profile;

namespace arc {

// Shows a notification for guiding the user for file system migration.
// This is used when an ARC app is launched while ARC is temporarily disabled
// due to the file system incompatibility.
void ShowArcMigrationGuideNotification(Profile* profile);

// Shows a one-time notification when this is the first sign-in after the
// migration has happened successfully. It records the fact to the pref that no
// more notification is necessary, either when it showed the notification or
// when the profile is newly created on the compatible filesystem.
void ShowArcMigrationSuccessNotificationIfNeeded(Profile* profile);

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_ARC_MIGRATION_GUIDE_NOTIFICATION_H_

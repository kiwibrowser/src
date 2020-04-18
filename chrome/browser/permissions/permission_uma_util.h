// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PERMISSIONS_PERMISSION_UMA_UTIL_H_
#define CHROME_BROWSER_PERMISSIONS_PERMISSION_UMA_UTIL_H_

#include <vector>

#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/permissions/permission_request.h"
#include "chrome/browser/permissions/permission_result.h"
#include "chrome/browser/permissions/permission_util.h"

namespace content {
class WebContents;
}

enum class PermissionRequestGestureType;
class GURL;
class PermissionRequest;
class Profile;

// Any new values should be inserted immediately prior to NUM.
enum class PermissionSourceUI {
  PROMPT = 0,
  OIB = 1,
  SITE_SETTINGS = 2,
  PAGE_ACTION = 3,

  // Always keep this at the end.
  NUM,
};

// Any new values should be inserted immediately prior to NUM.
enum class PermissionEmbargoStatus {
  NOT_EMBARGOED = 0,
  // Removed: PERMISSIONS_BLACKLISTING = 1,
  REPEATED_DISMISSALS = 2,
  REPEATED_IGNORES = 3,

  // Keep this at the end.
  NUM,
};

// Provides a convenient way of logging UMA for permission related operations.
class PermissionUmaUtil {
 public:
  static const char kPermissionsPromptShown[];
  static const char kPermissionsPromptShownGesture[];
  static const char kPermissionsPromptShownNoGesture[];
  static const char kPermissionsPromptAccepted[];
  static const char kPermissionsPromptAcceptedGesture[];
  static const char kPermissionsPromptAcceptedNoGesture[];
  static const char kPermissionsPromptDenied[];
  static const char kPermissionsPromptDeniedGesture[];
  static const char kPermissionsPromptDeniedNoGesture[];

  static void PermissionRequested(ContentSettingsType permission,
                                  const GURL& requesting_origin);
  static void PermissionRevoked(ContentSettingsType permission,
                                PermissionSourceUI source_ui,
                                const GURL& revoked_origin,
                                Profile* profile);

  static void RecordEmbargoPromptSuppression(
      PermissionEmbargoStatus embargo_status);

  static void RecordEmbargoPromptSuppressionFromSource(
      PermissionStatusSource source);

  static void RecordEmbargoStatus(PermissionEmbargoStatus embargo_status);

  // UMA specifically for when permission prompts are shown. This should be
  // roughly equivalent to the metrics above, however it is
  // useful to have separate UMA to a few reasons:
  // - to account for, and get data on coalesced permission bubbles
  // - there are other types of permissions prompts (e.g. download limiting)
  //   which don't go through PermissionContext
  // - the above metrics don't always add up (e.g. sum of
  //   granted+denied+dismissed+ignored is not equal to requested), so it is
  //   unclear from those metrics alone how many prompts are seen by users.
  static void PermissionPromptShown(
      const std::vector<PermissionRequest*>& requests);

  static void PermissionPromptResolved(
      const std::vector<PermissionRequest*>& requests,
      const content::WebContents* web_contents,
      PermissionAction permission_action);

  static void RecordWithBatteryBucket(const std::string& histogram);

 private:
  friend class PermissionUmaUtilTest;

  // web_contents may be null when for recording non-prompt actions.
  static void RecordPermissionAction(ContentSettingsType permission,
                                     PermissionAction action,
                                     PermissionSourceUI source_ui,
                                     PermissionRequestGestureType gesture_type,
                                     const GURL& requesting_origin,
                                     const content::WebContents* web_contents,
                                     Profile* profile);

  // Records |count| total prior actions for a prompt of type |permission|
  // for a single origin using |prefix| for the metric.
  static void RecordPermissionPromptPriorCount(
      ContentSettingsType permission,
      const std::string& prefix,
      int count);

  static void RecordPromptDecided(
      const std::vector<PermissionRequest*>& requests,
      bool accepted);

  DISALLOW_IMPLICIT_CONSTRUCTORS(PermissionUmaUtil);
};

#endif  // CHROME_BROWSER_PERMISSIONS_PERMISSION_UMA_UTIL_H_

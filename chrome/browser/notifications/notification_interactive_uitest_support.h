// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_INTERACTIVE_UITEST_SUPPORT_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_INTERACTIVE_UITEST_SUPPORT_H_

#include "base/command_line.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/content_settings/core/common/content_settings.h"

class MessageCenterChangeObserver {
 public:
  MessageCenterChangeObserver();
  ~MessageCenterChangeObserver();

  bool Wait();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenterChangeObserver);
};

class NotificationsTest : public InProcessBrowserTest {
 public:
  NotificationsTest() {}

 protected:
  // InProcessBrowserTest overrides.
  void SetUpDefaultCommandLine(base::CommandLine* command_line) override;

  int GetNotificationCount();
  int GetNotificationPopupCount();

  void CloseBrowserWindow(Browser* browser);
  void CrashTab(Browser* browser, int index);

  void DenyOrigin(const GURL& origin);
  void AllowOrigin(const GURL& origin);
  void AllowAllOrigins();
  void SetDefaultContentSetting(ContentSetting setting);
  void DropOriginPreference(const GURL& origin);

  std::string CreateNotification(Browser* browser,
                                 bool wait_for_new_balloon,
                                 const char* icon,
                                 const char* title,
                                 const char* body,
                                 const char* replace_id,
                                 const char* onclick = "");
  std::string CreateSimpleNotification(Browser* browser,
                                       bool wait_for_new_balloon);
  bool RequestAndAcceptPermission(Browser* browser);
  bool RequestAndDenyPermission(Browser* browser);
  bool RequestAndDismissPermission(Browser* browser);
  bool RequestPermissionAndWait(Browser* browser);
  std::string QueryPermissionStatus(Browser* browser);
  bool CancelNotification(const char* notification_id, Browser* browser);
  void GetPrefsByContentSetting(ContentSetting setting,
                                ContentSettingsForOneType* settings);
  bool CheckOriginInSetting(const ContentSettingsForOneType& settings,
                            const GURL& origin);

  GURL GetTestPageURLForFile(const std::string& file) const;
  GURL GetTestPageURL() const;
  content::WebContents* GetActiveWebContents(Browser* browser);

 protected:
  void EnablePermissionsEmbargo(
      base::test::ScopedFeatureList* scoped_feature_list);

 private:
  std::string RequestAndRespondToPermission(
      Browser* browser,
      PermissionRequestManager::AutoResponseType bubble_response);

  base::test::ScopedFeatureList feature_list_;
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_INTERACTIVE_UITEST_SUPPORT_H_

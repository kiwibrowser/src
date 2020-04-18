// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_NOTIFICATION_DOWNLOAD_NOTIFICATION_MANAGER_H_
#define CHROME_BROWSER_DOWNLOAD_NOTIFICATION_DOWNLOAD_NOTIFICATION_MANAGER_H_

#include <memory>
#include <set>

#include "chrome/browser/download/download_ui_controller.h"
#include "chrome/browser/download/notification/download_item_notification.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/common/download_item.h"

class DownloadNotificationManagerForProfile;

class DownloadNotificationManager : public DownloadUIController::Delegate {
 public:
  explicit DownloadNotificationManager(Profile* profile);
  ~DownloadNotificationManager() override;

  void OnAllDownloadsRemoving(Profile* profile);
  // DownloadUIController::Delegate:
  void OnNewDownloadReady(download::DownloadItem* item) override;

  DownloadNotificationManagerForProfile* GetForProfile(Profile* profile) const;

 private:
  friend class test::DownloadItemNotificationTest;

  Profile* main_profile_ = nullptr;
  std::map<Profile*, std::unique_ptr<DownloadNotificationManagerForProfile>>
      manager_for_profile_;
};

class DownloadNotificationManagerForProfile
    : public download::DownloadItem::Observer {
 public:
  DownloadNotificationManagerForProfile(
      Profile* profile, DownloadNotificationManager* parent_manager);
  ~DownloadNotificationManagerForProfile() override;

  // DownloadItem::Observer overrides:
  void OnDownloadUpdated(download::DownloadItem* download) override;
  void OnDownloadOpened(download::DownloadItem* download) override;
  void OnDownloadRemoved(download::DownloadItem* download) override;
  void OnDownloadDestroyed(download::DownloadItem* download) override;

  void OnNewDownloadReady(download::DownloadItem* item);

  DownloadItemNotification* GetNotificationItemByGuid(const std::string& guid);

 private:
  friend class test::DownloadItemNotificationTest;

  Profile* profile_ = nullptr;
  DownloadNotificationManager* parent_manager_;  // weak
  std::set<download::DownloadItem*> downloading_items_;
  std::map<download::DownloadItem*, std::unique_ptr<DownloadItemNotification>>
      items_;
};

#endif  // CHROME_BROWSER_DOWNLOAD_NOTIFICATION_DOWNLOAD_NOTIFICATION_MANAGER_H_

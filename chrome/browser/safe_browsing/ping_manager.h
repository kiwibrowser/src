// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_PING_MANAGER_H_
#define CHROME_BROWSER_SAFE_BROWSING_PING_MANAGER_H_

#include "components/safe_browsing/base_ping_manager.h"

class Profile;
class SkBitmap;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace safe_browsing {

class NotificationImageReporter;
class SafeBrowsingDatabaseManager;

class SafeBrowsingPingManager : public BasePingManager {
 public:
  ~SafeBrowsingPingManager() override;

  // Create an instance of the safe browsing ping manager.
  static std::unique_ptr<SafeBrowsingPingManager> Create(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const SafeBrowsingProtocolConfig& config);

  // Report notification content image to SafeBrowsing CSD server if necessary.
  void ReportNotificationImage(
      Profile* profile,
      const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager,
      const GURL& origin,
      const SkBitmap& image);

 private:
  friend class NotificationImageReporterTest;
  FRIEND_TEST_ALL_PREFIXES(SafeBrowsingPingManagerCertReportingTest,
                           UMAOnFailure);

  // Constructs a SafeBrowsingPingManager that issues network requests
  // using |url_loader_factory|.
  SafeBrowsingPingManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const SafeBrowsingProtocolConfig& config);

  // Sends reports of notification content images.
  std::unique_ptr<NotificationImageReporter> notification_image_reporter_;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingPingManager);
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_PING_MANAGER_H_

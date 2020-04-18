// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/ping_manager.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/safe_browsing/notification_image_reporter.h"
#include "components/net_log/chrome_net_log.h"
#include "content/public/browser/browser_thread.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/skia/include/core/SkBitmap.h"

using content::BrowserThread;

namespace safe_browsing {

// SafeBrowsingPingManager implementation ----------------------------------

// static
std::unique_ptr<SafeBrowsingPingManager> SafeBrowsingPingManager::Create(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const SafeBrowsingProtocolConfig& config) {
  return base::WrapUnique(
      new SafeBrowsingPingManager(url_loader_factory, config));
}

SafeBrowsingPingManager::SafeBrowsingPingManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const SafeBrowsingProtocolConfig& config)
    : BasePingManager(url_loader_factory, config) {
  if (url_loader_factory) {
    notification_image_reporter_ =
        std::make_unique<NotificationImageReporter>(url_loader_factory);
  }
}

SafeBrowsingPingManager::~SafeBrowsingPingManager() {}

void SafeBrowsingPingManager::ReportNotificationImage(
    Profile* profile,
    const scoped_refptr<SafeBrowsingDatabaseManager>& database_manager,
    const GURL& origin,
    const SkBitmap& image) {
  notification_image_reporter_->ReportNotificationImage(
      profile, database_manager, origin, image);
}

}  // namespace safe_browsing

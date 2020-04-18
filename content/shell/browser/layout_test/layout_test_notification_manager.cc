// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_notification_manager.h"

#include "content/public/browser/permission_type.h"
#include "content/shell/browser/layout_test/layout_test_browser_context.h"
#include "content/shell/browser/layout_test/layout_test_content_browser_client.h"
#include "content/shell/browser/layout_test/layout_test_permission_manager.h"

namespace content {

LayoutTestNotificationManager::LayoutTestNotificationManager() {}
LayoutTestNotificationManager::~LayoutTestNotificationManager() {}


blink::mojom::PermissionStatus
LayoutTestNotificationManager::CheckPermission(const GURL& origin) {
  return LayoutTestContentBrowserClient::Get()
      ->GetLayoutTestBrowserContext()
      ->GetLayoutTestPermissionManager()
      ->GetPermissionStatus(PermissionType::NOTIFICATIONS,
                            origin,
                            origin);
}

}  // namespace content

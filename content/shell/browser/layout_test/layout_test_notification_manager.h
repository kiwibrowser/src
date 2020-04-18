// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_NOTIFICATION_MANAGER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_NOTIFICATION_MANAGER_H_

#include <string>

#include "base/macros.h"
#include "content/test/mock_platform_notification_service.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"
#include "url/gurl.h"

namespace base {
class NullableString16;
}

namespace content {

// Responsible for tracking active notifications and allowed origins for the
// Web Notification API when running layout tests.
class LayoutTestNotificationManager : public MockPlatformNotificationService {
 public:
  LayoutTestNotificationManager();
  ~LayoutTestNotificationManager() override;

 private:
  blink::mojom::PermissionStatus CheckPermission(const GURL& origin) override;
  DISALLOW_COPY_AND_ASSIGN(LayoutTestNotificationManager);
};

}  // content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_NOTIFICATION_MANAGER_H_

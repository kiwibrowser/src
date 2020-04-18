// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_MOCK_NOTIFICATION_IMAGE_RETAINER_H_
#define CHROME_BROWSER_NOTIFICATIONS_MOCK_NOTIFICATION_IMAGE_RETAINER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/notifications/notification_image_retainer.h"

class GURL;

namespace gfx {
class Image;
}

// A mock NotificationImageRetainer class for use with unit tests. Returns
// predictable paths to callers wanting to register temporary files.
class MockNotificationImageRetainer : public NotificationImageRetainer {
 public:
  MockNotificationImageRetainer() : NotificationImageRetainer(nullptr) {}
  ~MockNotificationImageRetainer() override = default;

  // NotificationImageRetainer implementation:
  base::FilePath RegisterTemporaryImage(const gfx::Image& image,
                                        const std::string& profile_id,
                                        const GURL& origin) override;

 private:
  int counter_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MockNotificationImageRetainer);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_MOCK_NOTIFICATION_IMAGE_RETAINER_H_

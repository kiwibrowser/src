// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_FIRST_RUN_DESKTOP_CLEANER_
#define ASH_FIRST_RUN_DESKTOP_CLEANER_

#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {
class ContainerHider;
class NotificationBlocker;

// Class used to "clean" ash desktop, i.e. hide all windows and notifications.
class ASH_EXPORT DesktopCleaner {
 public:
  DesktopCleaner();
  ~DesktopCleaner();

 private:
  friend class FirstRunHelperTest;

  // Returns the list of containers that DesctopCleaner hides.
  static std::vector<int> GetContainersToHideForTest();

  std::vector<std::unique_ptr<ContainerHider>> container_hiders_;
  std::unique_ptr<NotificationBlocker> notification_blocker_;

  DISALLOW_COPY_AND_ASSIGN(DesktopCleaner);
};

}  // namespace ash

#endif  // ASH_FIRST_RUN_DESKTOP_CLEANER_

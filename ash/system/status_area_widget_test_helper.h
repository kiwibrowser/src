// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_STATUS_AREA_WIDGET_TEST_HELPER_H_
#define ASH_SYSTEM_STATUS_AREA_WIDGET_TEST_HELPER_H_

#include "base/macros.h"

namespace ash {

enum class LoginStatus;
class StatusAreaWidget;

class StatusAreaWidgetTestHelper {
 public:
  static LoginStatus GetUserLoginStatus();

  // Returns the StatusAreaWidget that appears on the primary display.
  static StatusAreaWidget* GetStatusAreaWidget();

  // Returns the StatusAreaWidget that appears on the secondary display.
  static StatusAreaWidget* GetSecondaryStatusAreaWidget();

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StatusAreaWidgetTestHelper);
};

}  // namespace ash

#endif  // ASH_SYSTEM_STATUS_AREA_WIDGET_TEST_HELPER_H_

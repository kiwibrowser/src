// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SIDEBAR_SIDEBAR_PARAMS_H_
#define ASH_SIDEBAR_SIDEBAR_PARAMS_H_

namespace ash {

enum class SidebarInitMode {
  // Normal mode: show message center and notifications
  NORMAL,
  // Settings: show settings panel
  MESSAGE_CENTER_SETTINGS
};

}  // namespace ash

#endif  // ASH_SIDEBAR_SIDEBAR_PARAMS_H_

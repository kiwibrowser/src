// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ASH_VIEW_IDS_H_
#define ASH_ASH_VIEW_IDS_H_

namespace ash {

// TODO(jamescook): Move to //ash/public/cpp.
enum ViewID {
  VIEW_ID_NONE = 0,

  // Ash IDs start above the range used in Chrome (c/b/ui/view_ids.h).
  VIEW_ID_ASH_START = 10000,

  VIEW_ID_ACCESSIBILITY_TRAY_ITEM,
  VIEW_ID_BLUETOOTH_DEFAULT_VIEW,
  // System tray network submenu item for extension-controlled networks.
  VIEW_ID_EXTENSION_CONTROLLED_WIFI,
  VIEW_ID_MEDIA_TRAY_VIEW,
  // Sticky header rows in a scroll view.
  VIEW_ID_STICKY_HEADER,
  // System tray menu item for "device is managed by example.com".
  VIEW_ID_TRAY_ENTERPRISE,
  // System tray up-arrow icon that shows an update is available.
  VIEW_ID_TRAY_UPDATE_ICON,
  // System tray menu item label for updates (e.g. "Restart to update").
  VIEW_ID_TRAY_UPDATE_MENU_LABEL,
  VIEW_ID_USER_VIEW_MEDIA_INDICATOR,
  // Keep alphabetized.
};

}  // namespace ash

#endif  // ASH_ASH_VIEW_IDS_H_

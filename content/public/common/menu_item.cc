// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/menu_item.h"

namespace content {

MenuItem::MenuItem()
    : type(OPTION),
      action(0),
      rtl(false),
      has_directional_override(false),
      enabled(false),
      checked(false) {
}

MenuItem::MenuItem(const MenuItem& item)
    : label(item.label),
      icon(item.icon),
      tool_tip(item.tool_tip),
      type(item.type),
      action(item.action),
      rtl(item.rtl),
      has_directional_override(item.has_directional_override),
      enabled(item.enabled),
      checked(item.checked),
      submenu(item.submenu) {
}

MenuItem::~MenuItem() {
}

}  // namespace content

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_MENU_ITEM_H_
#define CONTENT_PUBLIC_COMMON_MENU_ITEM_H_

#include <vector>

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/web/web_menu_item_info.h"

namespace content {

// Container for information about entries in an HTML select popup menu and
// custom entries of the context menu.
struct CONTENT_EXPORT MenuItem {
  enum Type {
    OPTION = blink::WebMenuItemInfo::kOption,
    CHECKABLE_OPTION = blink::WebMenuItemInfo::kCheckableOption,
    GROUP = blink::WebMenuItemInfo::kGroup,
    SEPARATOR = blink::WebMenuItemInfo::kSeparator,
    SUBMENU,  // This is currently only used by Pepper, not by WebKit.
    TYPE_LAST = SUBMENU
  };

  MenuItem();
  MenuItem(const MenuItem& item);
  ~MenuItem();

  base::string16 label;
  base::string16 icon;
  base::string16 tool_tip;
  Type type;
  unsigned action;
  bool rtl;
  bool has_directional_override;
  bool enabled;
  bool checked;
  std::vector<MenuItem> submenu;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_MENU_ITEM_H_

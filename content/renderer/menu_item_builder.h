// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MENU_ITEM_BUILDER_H_
#define CONTENT_RENDERER_MENU_ITEM_BUILDER_H_

namespace blink {
struct WebMenuItemInfo;
}

namespace content {
struct MenuItem;

class MenuItemBuilder {
 public:
  static MenuItem Build(const blink::WebMenuItemInfo& item);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MENU_ITEM_BUILDER_H_


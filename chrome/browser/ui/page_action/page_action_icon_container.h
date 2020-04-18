// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_ICON_CONTAINER_H_
#define CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_ICON_CONTAINER_H_

enum class PageActionIconType {
  // TODO(https://crbug.com/788051): Migrate page action icon update methods out
  // of LocationBar to this interface.
};

class PageActionIconContainer {
 public:
  // Signals a page action icon to update its visual state if it is present in
  // the browser window.
  virtual void UpdatePageActionIcon(PageActionIconType type) = 0;
};

#endif  // CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_ICON_CONTAINER_H_

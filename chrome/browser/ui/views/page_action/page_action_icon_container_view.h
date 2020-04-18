// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_CONTAINER_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_CONTAINER_VIEW_H_

#include "chrome/browser/ui/page_action/page_action_icon_container.h"
#include "ui/views/view.h"

class PageActionIconContainerView : public views::View,
                                    public PageActionIconContainer {
 public:
  PageActionIconContainerView() {}

 private:
  // PageActionIconContainer:
  void UpdatePageActionIcon(PageActionIconType type) override;
};

#endif  // CHROME_BROWSER_UI_VIEWS_PAGE_ACTION_PAGE_ACTION_ICON_CONTAINER_VIEW_H_

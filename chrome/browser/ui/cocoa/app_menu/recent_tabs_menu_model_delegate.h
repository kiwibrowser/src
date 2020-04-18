// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APP_MENU_RECENT_TABS_MENU_MODEL_DELEGATE_H_
#define CHROME_BROWSER_UI_COCOA_APP_MENU_RECENT_TABS_MENU_MODEL_DELEGATE_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "ui/base/models/menu_model_delegate.h"

namespace ui {
class MenuModel;
}

// Updates the recent tabs menu when the model changes.
class RecentTabsMenuModelDelegate : public ui::MenuModelDelegate {
 public:
  // |model| must live longer than this object.
  RecentTabsMenuModelDelegate(ui::MenuModel* model, NSMenu* menu);
  ~RecentTabsMenuModelDelegate() override;

  // ui::MenuModelDelegate:
  void OnIconChanged(int index) override;

 private:
  ui::MenuModel* model_;  // weak
  base::scoped_nsobject<NSMenu> menu_;

  DISALLOW_COPY_AND_ASSIGN(RecentTabsMenuModelDelegate);
};

#endif  // CHROME_BROWSER_UI_COCOA_APP_MENU_RECENT_TABS_MENU_MODEL_DELEGATE_H_

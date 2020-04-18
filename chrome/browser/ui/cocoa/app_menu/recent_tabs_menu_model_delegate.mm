// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/app_menu/recent_tabs_menu_model_delegate.h"
#include "ui/base/models/menu_model.h"
#include "ui/gfx/image/image.h"

RecentTabsMenuModelDelegate::RecentTabsMenuModelDelegate(
    ui::MenuModel* model,
    NSMenu* menu)
    : model_(model),
      menu_([menu retain]) {
  model_->SetMenuModelDelegate(this);
}

RecentTabsMenuModelDelegate::~RecentTabsMenuModelDelegate() {
  model_->SetMenuModelDelegate(NULL);
}

void RecentTabsMenuModelDelegate::OnIconChanged(int index) {
  gfx::Image icon;
  if (!model_->GetIconAt(index, &icon))
    return;

  NSMenuItem* item = [menu_ itemAtIndex:index];
  [item setImage:icon.ToNSImage()];
}

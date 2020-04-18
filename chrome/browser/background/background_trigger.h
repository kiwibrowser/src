// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_BACKGROUND_TRIGGER_H_
#define CHROME_BROWSER_BACKGROUND_BACKGROUND_TRIGGER_H_

#include "base/strings/string16.h"
#include "ui/gfx/image/image_skia.h"

// Enables background mode if registered with the BackgroundModeManager.
class BackgroundTrigger {
 public:
  virtual ~BackgroundTrigger() {}

  // The name to be displayed in the status icon menu and the installation
  // notification.
  virtual base::string16 GetName() = 0;

  // The icon to be displayed in the status icon menu.
  virtual gfx::ImageSkia* GetIcon() = 0;

  // Handles a user's click on the status icon menu item.
  virtual void OnMenuClick() = 0;
};

#endif  // CHROME_BROWSER_BACKGROUND_BACKGROUND_TRIGGER_H_

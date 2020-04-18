// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PERMISSION_BUBBLE_CHOOSER_BUBBLE_UI_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_PERMISSION_BUBBLE_CHOOSER_BUBBLE_UI_COCOA_H_

#import <Cocoa/Cocoa.h>

#include <memory>

#include "base/macros.h"
#include "components/bubble/bubble_ui.h"

class Browser;
@class ChooserBubbleUiController;
class ChooserController;

// ChooserBubbleUiCocoa implements a chooser-based permission model.
// It uses |NSTableView| to show a list of options for user to grant
// permission. It can be used by the WebUSB or WebBluetooth APIs.
// It is owned by the BubbleController, which is owned by the BubbleManager.
class ChooserBubbleUiCocoa : public BubbleUi {
 public:
  ChooserBubbleUiCocoa(Browser* browser,
                       std::unique_ptr<ChooserController> chooser_controller);
  ~ChooserBubbleUiCocoa() override;

  // BubbleUi:
  void Show(BubbleReference bubble_reference) override;
  void Close() override;
  void UpdateAnchorPosition() override;

  // Called when |chooser_bubble_ui_controller_| is closing.
  void OnBubbleClosing();

 private:
  Browser* browser_;  // Weak.
  // Cocoa-side chooser bubble UI controller. Weak, as it will close itself.
  ChooserBubbleUiController* chooser_bubble_ui_controller_;

  DISALLOW_COPY_AND_ASSIGN(ChooserBubbleUiCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_PERMISSION_BUBBLE_CHOOSER_BUBBLE_UI_COCOA_H_

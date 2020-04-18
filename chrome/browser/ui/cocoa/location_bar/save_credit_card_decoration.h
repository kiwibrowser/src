// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SAVE_CREDIT_CARD_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SAVE_CREDIT_CARD_DECORATION_H_

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "chrome/browser/ui/cocoa/location_bar/image_decoration.h"

class CommandUpdater;

// Save credit card icon on the right side of the field. This appears when
// the save credit card bubble is available on the current page. This icon is
// never lit.
class SaveCreditCardDecoration : public ImageDecoration {
 public:
  explicit SaveCreditCardDecoration(CommandUpdater* command_updater);
  ~SaveCreditCardDecoration() override;

  // Set up the SaveCreditCardDecoration's icon to match the darkness of the
  // location bar. This happens once the location bar has been added to the
  // window so that we know the theme.
  void SetIcon(bool locationBarIsDark);

  // LocationBarDecoration implementation:
  AcceptsPress AcceptsMousePress() override;
  bool OnMousePressed(NSRect frame, NSPoint location) override;
  NSString* GetToolTip() override;
  NSPoint GetBubblePointInFrame(NSRect frame) override;

 private:
  // For showing the save credit card bubble.
  CommandUpdater* command_updater_;  // Weak, owned by Browser.

  DISALLOW_COPY_AND_ASSIGN(SaveCreditCardDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SAVE_CREDIT_CARD_DECORATION_H_

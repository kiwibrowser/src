// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_STAR_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_STAR_DECORATION_H_

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "chrome/browser/ui/cocoa/location_bar/image_decoration.h"

class CommandUpdater;

// Star icon on the right side of the field.

class StarDecoration : public ImageDecoration {
 public:
  explicit StarDecoration(CommandUpdater* command_updater);
  ~StarDecoration() override;

  // Sets the image and tooltip based on |starred|.
  void SetStarred(bool starred, bool locationBarIsDark);

  // Returns an anchor for GetBubblePointInFrame which points between the star
  // icon's legs.
  static NSPoint GetStarBubblePointInFrame(NSRect draw_frame);

  // Returns true if the star is lit.
  bool starred() const { return starred_; }

  // Implement |LocationBarDecoration|.
  AcceptsPress AcceptsMousePress() override;
  bool OnMousePressed(NSRect frame, NSPoint location) override;
  NSString* GetToolTip() override;
  NSPoint GetBubblePointInFrame(NSRect frame) override;

 protected:
  // Overridden from LocationBarDecoration:
  SkColor GetMaterialIconColor(bool location_bar_is_dark) const override;
  const gfx::VectorIcon* GetMaterialVectorIcon() const override;

 private:
  // For bringing up bookmark bar.
  CommandUpdater* command_updater_;  // Weak, owned by Browser.

  // The string to show for a tooltip.
  base::scoped_nsobject<NSString> tooltip_;

  // Whether the star icon is lit.
  bool starred_;

  DISALLOW_COPY_AND_ASSIGN(StarDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_STAR_DECORATION_H_

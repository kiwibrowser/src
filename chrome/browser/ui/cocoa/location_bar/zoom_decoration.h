// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_ZOOM_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_ZOOM_DECORATION_H_

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#import "chrome/browser/ui/cocoa/browser/zoom_bubble_controller.h"
#include "chrome/browser/ui/cocoa/location_bar/image_decoration.h"

class LocationBarViewMac;
@class ZoomBubbleController;
class ZoomDecorationTest;

namespace zoom {
class ZoomController;
}

// Zoom icon at the end of the omnibox (close to page actions) when at a
// non-standard zoom level.
class ZoomDecoration : public ImageDecoration,
                       public ZoomBubbleControllerDelegate {
 public:
  explicit ZoomDecoration(LocationBarViewMac* owner);
  ~ZoomDecoration() override;

  // Called when this decoration should show or hide itself in its most current
  // state. Returns whether any updates were made.
  bool UpdateIfNecessary(zoom::ZoomController* zoom_controller,
                         bool default_zoom_changed,
                         bool location_bar_is_dark);

  // Shows the zoom bubble for this decoration. If |auto_close| is YES, then
  // the bubble will automatically close after a fixed period of time.
  // If a bubble is already showing, the |auto_close| timer is reset.
  void ShowBubble(BOOL auto_close);

  // Closes the zoom bubble.
  void CloseBubble();

 protected:
  // Hides all UI associated with the zoom decoration.
  // Virtual and protected for testing.
  virtual void HideUI();

  // Update UI associated with the zoom decoration.
  // Virtual and protected for testing.
  virtual void UpdateUI(zoom::ZoomController* zoom_controller,
                        NSString* tooltip_string,
                        bool location_bar_is_dark);

  // Overridden from LocationBarDecoration:
  const gfx::VectorIcon* GetMaterialVectorIcon() const override;

 private:
  friend ZoomDecorationTest;

  bool IsAtDefaultZoom() const;

  // Returns true when |bubble_| or the views zoom bubble exists.
  bool IsBubbleShown() const;

  // Virtual for testing.
  virtual bool ShouldShowDecoration() const;

  // LocationBarDecoration implementation.
  AcceptsPress AcceptsMousePress() override;
  bool OnMousePressed(NSRect frame, NSPoint location) override;
  NSString* GetToolTip() override;
  NSPoint GetBubblePointInFrame(NSRect frame) override;

  // ZoomBubbleControllerDelegate implementation.
  content::WebContents* GetWebContents() override;
  void OnClose() override;

  // The control that owns this. Weak.
  LocationBarViewMac* owner_;

  // The bubble that this decoration shows. Weak, owns self.
  ZoomBubbleController* bubble_;

  // The string to show for a tooltip.
  base::scoped_nsobject<NSString> tooltip_;

  const gfx::VectorIcon* vector_icon_;

  DISALLOW_COPY_AND_ASSIGN(ZoomDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_ZOOM_DECORATION_H_

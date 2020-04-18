// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SECURITY_STATE_BUBBLE_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SECURITY_STATE_BUBBLE_DECORATION_H_

#import <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "chrome/browser/ui/cocoa/location_bar/bubble_decoration.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"

// Draws the verbose state bubble, which contains the security icon and a label
// describing its security state. If an EV cert is available, the icon will be
// a lock and the label will contain the certificate's name. The
// |location_icon| is used to fulfill drag-related calls.

class LocationBarViewMac;
class LocationIconDecoration;

namespace {
class PageInfoBubbleDecorationTest;
}

namespace {
class LocationBarViewMacTest;
}

class PageInfoBubbleDecoration : public BubbleDecoration,
                                 public gfx::AnimationDelegate {
 public:
  explicit PageInfoBubbleDecoration(LocationBarViewMac* owner);
  ~PageInfoBubbleDecoration() override;

  // |GetWidthForSpace()| will set |full_label| as the label, if it
  // fits, else it will set an elided version.
  void SetFullLabel(NSString* full_label);

  // Set the color of the label.
  void SetLabelColor(SkColor color);

  // Methods that animate in and out the chip. Virtual for testing.
  virtual void AnimateIn(bool image_fade = true);
  virtual void AnimateOut();

  // Shows the chip without animation. Virtual for testing.
  virtual void ShowWithoutAnimation();

  // Returns true if the chip has fully animated in.
  bool HasAnimatedIn() const;

  // Returns true if the chip has fully animated out. Virtual for testing.
  virtual bool HasAnimatedOut() const;

  // Returns true if the chip is in the process of animating out. Virtual for
  // testing.
  virtual bool AnimatingOut() const;

  // Resets the animation. Virtual for testing.
  virtual void ResetAnimation();

  // LocationBarDecoration:
  CGFloat GetWidthForSpace(CGFloat width) override;
  void DrawInFrame(NSRect frame, NSView* control_view) override;
  bool IsDraggable() override;
  NSPasteboard* GetDragPasteboard() override;
  NSImage* GetDragImage() override;
  NSRect GetDragImageFrame(NSRect frame) override;
  bool OnMousePressed(NSRect frame, NSPoint location) override;
  AcceptsPress AcceptsMousePress() override;
  NSPoint GetBubblePointInFrame(NSRect frame) override;
  NSString* GetToolTip() override;
  NSString* GetAccessibilityLabel() override;
  NSRect GetRealFocusRingBounds(NSRect apparent_frame) const override;

  // BubbleDecoration:
  NSColor* GetBackgroundBorderColor() override;

  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;

 protected:
  NSColor* GetDarkModeTextColor() override;

 private:
  friend class ::LocationBarViewMacTest;
  friend class ::PageInfoBubbleDecorationTest;

  // Returns the animation progress. If not in MD, the animation progress
  // should always be 1.0.
  CGFloat GetAnimationProgress() const;

  // Helper method that calculates and returns the width of the label and icon
  // within |width|.
  CGFloat GetWidthForText(CGFloat width);

  LocationIconDecoration* location_icon_;  // weak, owned by location bar.

  // The real label. BubbleDecoration's label may be elided.
  base::scoped_nsobject<NSString> full_label_;

  // The color of the label's text. The default color is kGoogleGreen700.
  SkColor label_color_;

  // True if the image should fade when the verbose animates in.
  bool image_fade_;

  // The animation of the decoration.
  gfx::SlideAnimation animation_;

  LocationBarViewMac* owner_;  // weak

  // Distance in points to inset the right edge of the focus ring by. This is
  // used by |GetRealFocusRingBounds| to prevent the focus ring from including
  // the divider bar. This is recomputed every time this object is drawn.
  int focus_ring_right_inset_ = 0;

  // Distance in points to inset the left edge of the focus ring by.
  int focus_ring_left_inset_ = 0;

  // Used to disable find bar animations when testing.
  bool disable_animations_during_testing_;

  // The frame of the drag-and-drop image.
  NSRect drag_frame_;

  DISALLOW_COPY_AND_ASSIGN(PageInfoBubbleDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_SECURITY_STATE_BUBBLE_DECORATION_H_

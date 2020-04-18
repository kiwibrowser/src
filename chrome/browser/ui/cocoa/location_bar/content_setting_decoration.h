// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_CONTENT_SETTING_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_CONTENT_SETTING_DECORATION_H_

#include <memory>

#include "base/macros.h"
#import "chrome/browser/ui/cocoa/location_bar/image_decoration.h"
#include "components/content_settings/core/common/content_settings_types.h"

// ContentSettingDecoration is used to display the content settings
// images on the current page. For example, the decoration animates into and out
// of view when a page attempts to show a popup and the popup blocker is on.

@class ContentSettingAnimationState;
@class ContentSettingBubbleController;
class ContentSettingDecorationTest;
class ContentSettingImageModel;
class LocationBarViewMac;
class Profile;

namespace content {
class WebContents;
}

class ContentSettingDecoration : public ImageDecoration {
 public:
  ContentSettingDecoration(std::unique_ptr<ContentSettingImageModel> model,
                           LocationBarViewMac* owner,
                           Profile* profile);
  ~ContentSettingDecoration() override;

  // Updates the image and visibility state based on the supplied WebContents.
  // Returns true if the decoration's visible state changed.
  bool UpdateFromWebContents(content::WebContents* web_contents);

  // Returns if the content setting bubble is showing for this decoration.
  bool IsShowingBubble() const;

  // Overridden from |LocationBarDecoration|
  AcceptsPress AcceptsMousePress() override;
  bool OnMousePressed(NSRect frame, NSPoint location) override;
  NSString* GetToolTip() override;
  CGFloat GetWidthForSpace(CGFloat width) override;
  void DrawInFrame(NSRect frame, NSView* control_view) override;
  NSPoint GetBubblePointInFrame(NSRect frame) override;

  // Called from internal animator. Only public because ObjC objects can't
  // be friends.
  virtual void AnimationTimerFired();

 protected:
  CGFloat DividerPadding() const override;

 private:
  friend class ContentSettingDecorationTest;

  void SetToolTip(NSString* tooltip);

  // Returns an attributed string with the animated text.
  base::scoped_nsobject<NSAttributedString> CreateAnimatedText();

  // Measure the width of the animated text.
  CGFloat MeasureTextWidth();

  std::unique_ptr<ContentSettingImageModel> content_setting_image_model_;

  LocationBarViewMac* owner_;  // weak
  Profile* profile_;  // weak

  base::scoped_nsobject<NSString> tooltip_;

  // Used when the decoration has animated text.
  base::scoped_nsobject<ContentSettingAnimationState> animation_;
  CGFloat text_width_;
  base::scoped_nsobject<NSAttributedString> animated_text_;

  // The window of the content setting bubble.
  base::scoped_nsobject<NSWindow> bubbleWindow_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_CONTENT_SETTING_DECORATION_H_

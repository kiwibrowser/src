// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_KEYWORD_HINT_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_KEYWORD_HINT_DECORATION_H_

#include <string>

#import "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/string16.h"

// Draws the keyword hint, "Press [tab] to search <site>".

class KeywordHintDecoration : public LocationBarDecoration {
 public:
  KeywordHintDecoration();
  ~KeywordHintDecoration() override;

  // Calculates the message to display and where to place the [tab]
  // image.
  void SetKeyword(const base::string16& keyword, bool is_extension_keyword);

  // Implement |LocationBarDecoration|.
  void DrawInFrame(NSRect frame, NSView* control_view) override;
  CGFloat GetWidthForSpace(CGFloat width) override;

  NSString* GetAccessibilityLabel() override;
  bool IsAccessibilityIgnored() override;

 private:
  // Fetch and cache the [tab] image.
  NSImage* GetHintImage();

  // Attributes for drawing the hint string, such as font and color.
  base::scoped_nsobject<NSDictionary> attributes_;

  // Cache for the [tab] image.
  base::scoped_nsobject<NSImage> hint_image_;

  // The text to display to the left and right of the hint image.
  base::scoped_nsobject<NSString> hint_prefix_;
  base::scoped_nsobject<NSString> hint_suffix_;

  // The accessibility text used to describe this decoration.
  base::string16 accessibility_text_;

  DISALLOW_COPY_AND_ASSIGN(KeywordHintDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_KEYWORD_HINT_DECORATION_H_

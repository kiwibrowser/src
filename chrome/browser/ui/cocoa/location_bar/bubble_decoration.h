// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_BUBBLE_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_BUBBLE_DECORATION_H_

#import <Cocoa/Cocoa.h>

#include "base/gtest_prod_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"

// Draws an outlined rounded rect, with an optional image to the left
// and an optional text label to the right.

class BubbleDecoration : public LocationBarDecoration {
 public:
  BubbleDecoration();
  ~BubbleDecoration() override;

  // Setup the drawing parameters.
  NSImage* GetImage();
  virtual NSColor* GetBackgroundBorderColor() = 0;
  void SetImage(NSImage* image);
  void SetLabel(NSString* label);
  void SetTextColor(NSColor* text_color);
  void SetFont(NSFont* font);
  void SetRetinaBaselineOffset(CGFloat offset);

  // Implement |LocationBarDecoration|.
  CGFloat GetWidthForSpace(CGFloat width) override;
  NSRect GetBackgroundFrame(NSRect frame) override;
  void DrawInFrame(NSRect frame, NSView* control_view) override;
  NSRect GetTrackingFrame(NSRect frame) override;
  NSFont* GetFont() const override;

 protected:
  // Helper returning bubble width for the given |image| and |label|
  // assuming |font_| (for sizing text).  Arguments can be nil.
  CGFloat GetWidthForImageAndLabel(NSImage* image, NSString* label);

  // Helper to return where the image is drawn, for subclasses to drag
  // from.  |frame| is the decoration's frame in the containing cell.
  NSRect GetImageRectInFrame(NSRect frame);

  // Returns the text color when the theme is dark.
  virtual NSColor* GetDarkModeTextColor();

  // Returns false if the |label_| is nil or empty.
  bool HasLabel() const;

  // Image drawn in the left side of the bubble.
  base::scoped_nsobject<NSImage> image_;

  // Label to draw to right of image.  Can be |nil|.
  base::scoped_nsobject<NSString> label_;

  // Contains attribute for drawing |label_|.
  base::scoped_nsobject<NSMutableDictionary> attributes_;

 private:
  friend class SelectedKeywordDecorationTest;
  FRIEND_TEST_ALL_PREFIXES(SelectedKeywordDecorationTest,
                           UsesPartialKeywordIfNarrow);

  // Contains any Retina-only baseline adjustment for |label_|.
  CGFloat retina_baseline_offset_;

  DISALLOW_COPY_AND_ASSIGN(BubbleDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_BUBBLE_DECORATION_H_

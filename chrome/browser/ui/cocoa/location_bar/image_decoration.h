// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_LOCATION_BAR_IMAGE_DECORATION_H_
#define CHROME_BROWSER_UI_COCOA_LOCATION_BAR_IMAGE_DECORATION_H_

#import "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "chrome/browser/ui/cocoa/location_bar/location_bar_decoration.h"

// |LocationBarDecoration| which sizes and draws itself according to
// an |NSImage|.

class ImageDecoration : public LocationBarDecoration {
 public:
  ImageDecoration();
  ~ImageDecoration() override;

  NSImage* GetImage();
  void SetImage(NSImage* image);

  // Returns the part of |frame| the image is drawn in.
  NSRect GetDrawRectInFrame(NSRect frame);

  // Implement |LocationBarDecoration|.
  CGFloat GetWidthForSpace(CGFloat width) override;
  void DrawInFrame(NSRect frame, NSView* control_view) override;

 private:
  base::scoped_nsobject<NSImage> image_;

  DISALLOW_COPY_AND_ASSIGN(ImageDecoration);
};

#endif  // CHROME_BROWSER_UI_COCOA_LOCATION_BAR_IMAGE_DECORATION_H_

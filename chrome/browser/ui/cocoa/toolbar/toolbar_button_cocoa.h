// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_BUTTON_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_BUTTON_COCOA_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#import "ui/gfx/color_utils.h"

namespace gfx {
struct VectorIcon;
}

enum class ToolbarButtonImageBackgroundStyle {
  DEFAULT,
  HOVER,
  HOVER_THEMED,
  PRESSED,
  PRESSED_THEMED,
};

// NSButton subclass which handles middle mouse clicking.
@interface ToolbarButton : NSButton<ThemedWindowDrawing>  {
 @protected
  // The toolbar button's image, as assigned by setImage:.
  base::scoped_nsobject<NSImage> image_;

  // YES when middle mouse clicks should be handled.
  BOOL handleMiddleClick_;
}

// Return the size of toolbar buttons.
+ (NSSize)toolbarButtonSize;
// Whether or not to handle the mouse middle click events.
@property(assign, nonatomic) BOOL handleMiddleClick;
// Whether this button should mirror in RTL. Defaults to YES.
@property(assign, nonatomic, readonly) BOOL shouldMirrorInRTL;
// Override point for subclasses to return their vector icon.
- (const gfx::VectorIcon*)vectorIcon;
// Override point for subclasses to return their vector icon color.
- (SkColor)vectorIconColor:(BOOL)themeIsDark;
// When in Material Design mode, sets the images for each of the ToolbarButton's
// states from the specified image.
- (void)setImage:(NSImage*)anImage;
// Resets the images for each of the ToolbarButton's states from its vector icon
// id or its main image. Should only be called when in Material Design mode.
- (void)resetButtonStateImages;
@end

@interface ToolbarButton (ExposedForTesting)
- (BOOL)shouldHandleEvent:(NSEvent*)theEvent;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TOOLBAR_TOOLBAR_BUTTON_COCOA_H_

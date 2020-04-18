// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_COLOR_CHOOSER_MAC_H_
#define CHROME_BROWSER_UI_COCOA_COLOR_CHOOSER_MAC_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "content/public/browser/color_chooser.h"
#include "content/public/browser/web_contents.h"

class ColorChooserMac;

// A Listener class to act as a event target for NSColorPanel and send
// the results to the C++ class, ColorChooserMac.
@interface ColorPanelCocoa : NSObject<NSWindowDelegate> {
 @protected
  // We don't call DidChooseColor if the change wasn't caused by the user
  // interacting with the panel.
  BOOL nonUserChange_;
 @private
  ColorChooserMac* chooser_;  // weak, owns this
}

- (id)initWithChooser:(ColorChooserMac*)chooser;

// Called from NSColorPanel.
- (void)didChooseColor:(NSColorPanel*)panel;

// Sets color to the NSColorPanel as a non user change.
- (void)setColor:(NSColor*)color;

@end

class ColorChooserMac : public content::ColorChooser {
 public:
  // Returns a ColorChooserMac instance owned by the ColorChooserMac class -
  // call End() when done to free it. Each call to Open() returns a new
  // instance after freeing the previous one (i.e. it does not reuse the
  // previous instance even if it still exists).
  static ColorChooserMac* Open(content::WebContents* web_contents,
                               SkColor initial_color);

  // Called from ColorPanelCocoa.
  void DidChooseColorInColorPanel(SkColor color);
  void DidCloseColorPabel();

  // Set the color programmatically.
  void SetSelectedColor(SkColor color) override;

  // Call when done with the ColorChooserMac.
  void End() override;

 private:
  ColorChooserMac(content::WebContents* tab, SkColor initial_color);

  ~ColorChooserMac() override;

  static ColorChooserMac* current_color_chooser_;

  // The web contents invoking the color chooser.  No ownership because it will
  // outlive this class.
  content::WebContents* web_contents_;
  base::scoped_nsobject<ColorPanelCocoa> panel_;

  DISALLOW_COPY_AND_ASSIGN(ColorChooserMac);
};

#endif  // CHROME_BROWSER_UI_COCOA_COLOR_CHOOSER_MAC_H_

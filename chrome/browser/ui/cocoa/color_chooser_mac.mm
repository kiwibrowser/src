// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/color_chooser_mac.h"

#include "base/logging.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "skia/ext/skia_utils_mac.h"

ColorChooserMac* ColorChooserMac::current_color_chooser_ = NULL;

// static
ColorChooserMac* ColorChooserMac::Open(content::WebContents* web_contents,
                                       SkColor initial_color) {
  if (current_color_chooser_)
    current_color_chooser_->End();
  DCHECK(!current_color_chooser_);
  current_color_chooser_ =
      new ColorChooserMac(web_contents, initial_color);
  return current_color_chooser_;
}

ColorChooserMac::ColorChooserMac(content::WebContents* web_contents,
                                 SkColor initial_color)
    : web_contents_(web_contents) {
  panel_.reset([[ColorPanelCocoa alloc] initWithChooser:this]);
  [panel_ setColor:skia::SkColorToDeviceNSColor(initial_color)];
  [[NSColorPanel sharedColorPanel] makeKeyAndOrderFront:nil];
}

ColorChooserMac::~ColorChooserMac() {
  // Always call End() before destroying.
  DCHECK(!panel_);
}

void ColorChooserMac::DidChooseColorInColorPanel(SkColor color) {
  if (web_contents_)
    web_contents_->DidChooseColorInColorChooser(color);
}

void ColorChooserMac::DidCloseColorPabel() {
  End();
}

void ColorChooserMac::End() {
  panel_.reset();
  DCHECK(current_color_chooser_ == this);
  current_color_chooser_ = NULL;
  if (web_contents_)
      web_contents_->DidEndColorChooser();
}

void ColorChooserMac::SetSelectedColor(SkColor color) {
  [panel_ setColor:skia::SkColorToDeviceNSColor(color)];
}

@interface NSColorPanel (Private)
// Private method returning the NSColorPanel's target.
- (id)__target;
@end

@implementation ColorPanelCocoa

- (id)initWithChooser:(ColorChooserMac*)chooser {
  if ((self = [super init])) {
    chooser_ = chooser;
    NSColorPanel* panel = [NSColorPanel sharedColorPanel];
    [panel setShowsAlpha:NO];
    [panel setDelegate:self];
    [panel setTarget:self];
    [panel setAction:@selector(didChooseColor:)];
  }
  return self;
}

- (void)dealloc {
  NSColorPanel* panel = [NSColorPanel sharedColorPanel];

  // On macOS 10.13 the NSColorPanel delegate can apparently get reset to nil
  // with the target left unchanged. Use the private __target method to see if
  // the ColorPanelCocoa is still the target.
  BOOL respondsToPrivateTargetMethod =
      [panel respondsToSelector:@selector(__target)];

  if ([panel delegate] == self ||
      (respondsToPrivateTargetMethod && [panel __target] == self)) {
    [panel setDelegate:nil];
    [panel setTarget:nil];
    [panel setAction:nullptr];
  }

  [super dealloc];
}

- (void)windowWillClose:(NSNotification*)notification {
  chooser_->DidCloseColorPabel();
  nonUserChange_ = NO;
}

- (void)didChooseColor:(NSColorPanel*)panel {
  if (nonUserChange_) {
    nonUserChange_ = NO;
    return;
  }
  nonUserChange_ = NO;
  NSColor* color = [panel color];
  if ([[color colorSpaceName] isEqualToString:NSNamedColorSpace]) {
    color = [color colorUsingColorSpace:[NSColorSpace genericRGBColorSpace]];
    // Some colors in "Developer" palette in "Color Palettes" tab can't be
    // converted to RGB. We just ignore such colors.
    // TODO(tkent): We should notice the rejection to users.
    if (!color)
      return;
  }
  if ([color colorSpace] == [NSColorSpace genericRGBColorSpace]) {
    // genericRGB -> deviceRGB conversion isn't ignorable.  We'd like to use RGB
    // values shown in NSColorPanel UI.
    CGFloat red, green, blue, alpha;
    [color getRed:&red green:&green blue:&blue alpha:&alpha];
    SkColor skColor = SkColorSetARGB(SkScalarRoundToInt(255.0 * alpha),
                                     SkScalarRoundToInt(255.0 * red),
                                     SkScalarRoundToInt(255.0 * green),
                                     SkScalarRoundToInt(255.0 * blue));
    chooser_->DidChooseColorInColorPanel(skColor);
  } else {
    chooser_->DidChooseColorInColorPanel(skia::NSDeviceColorToSkColor(
        [[panel color] colorUsingColorSpaceName:NSDeviceRGBColorSpace]));
  }
}

- (void)setColor:(NSColor*)color {
  nonUserChange_ = YES;
  [[NSColorPanel sharedColorPanel] setColor:color];
}

namespace chrome {

content::ColorChooser* ShowColorChooser(content::WebContents* web_contents,
                                        SkColor initial_color) {
  return ColorChooserMac::Open(web_contents, initial_color);
}

}  // namepace chrome

@end

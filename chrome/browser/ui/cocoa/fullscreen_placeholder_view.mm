// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/fullscreen_placeholder_view.h"

namespace {

NSImage* BlurImageWithRadius(CGImageRef image, NSNumber* radius) {
  CIContext* context = [CIContext contextWithCGContext:nil options:nil];
  CIImage* inputImage = [[[CIImage alloc] initWithCGImage:image] autorelease];

  CIFilter* clampFilter = [CIFilter filterWithName:@"CIAffineClamp"];
  [clampFilter setValue:inputImage forKey:kCIInputImageKey];
  [clampFilter setValue:[NSValue valueWithBytes:&CGAffineTransformIdentity
                                       objCType:@encode(CGAffineTransform)]
                 forKey:@"inputTransform"];
  CIImage* extendedImage = [clampFilter valueForKey:kCIOutputImageKey];

  CIFilter* blurFilter = [CIFilter filterWithName:@"CIGaussianBlur"];
  [blurFilter setValue:extendedImage forKey:kCIInputImageKey];
  [blurFilter setValue:radius forKey:@"inputRadius"];
  CIImage* outputImage = [blurFilter valueForKey:kCIOutputImageKey];

  CGImageRef cgImage =
      (CGImageRef)[(id)[context createCGImage:outputImage
                                     fromRect:[inputImage extent]] autorelease];
  NSImage* resultImage =
      [[[NSImage alloc] initWithCGImage:cgImage size:CGSizeZero] autorelease];
  return resultImage;
}
}

@implementation FullscreenPlaceholderView {
  NSTextField* textView_;
}

- (id)initWithFrame:(NSRect)frameRect image:(CGImageRef)screenshot {
  if (self = [super initWithFrame:frameRect]) {
    NSView* screenshotView =
        [[[NSView alloc] initWithFrame:self.bounds] autorelease];
    screenshotView.layer = [[[CALayer alloc] init] autorelease];
    screenshotView.wantsLayer = YES;
    screenshotView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self addSubview:screenshotView];
    NSImage* screenshotImage = BlurImageWithRadius(screenshot, @15.0);
    screenshotView.layer.contentsGravity = kCAGravityResizeAspectFill;
    screenshotView.layer.contents = [screenshotImage
        layerContentsForContentsScale:[screenshotView.layer contentsScale]];

    textView_ = [[[NSTextField alloc] initWithFrame:frameRect] autorelease];
    [textView_ setStringValue:@" Click to exit fullscreen "];
    [textView_ setTextColor:[[NSColor whiteColor] colorWithAlphaComponent:0.6]];
    [textView_.cell setWraps:NO];
    [textView_.cell setScrollable:YES];
    [textView_ setBezeled:NO];
    [textView_ setEditable:NO];
    [textView_ setDrawsBackground:NO];
    [textView_ setWantsLayer:YES];
    [self resizeLabel];

    NSColor* color = [NSColor colorWithCalibratedWhite:0.3 alpha:0.5];
    [textView_.layer setBackgroundColor:color.CGColor];
    [textView_.layer setCornerRadius:12];
    [self addSubview:textView_];
  }
  return self;
}

- (void)mouseDown:(NSEvent*)event {
  // This function handles click events on FullscreenPlaceholderView and will be
  // used to exit fullscreen
}

- (void)resizeSubviewsWithOldSize:(CGSize)oldSize {
  [super resizeSubviewsWithOldSize:oldSize];
  [self resizeLabel];
}

- (void)resizeLabel {
  [textView_ setFont:[NSFont systemFontOfSize:0.023 * (NSWidth(self.frame))]];
  [textView_ sizeToFit];
  [textView_
      setFrameOrigin:NSMakePoint(
                         NSMidX(self.bounds) - NSMidX(textView_.bounds),
                         NSMidY(self.bounds) - NSMidY(textView_.bounds))];
}

@end

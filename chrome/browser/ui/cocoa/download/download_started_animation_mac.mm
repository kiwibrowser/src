// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains the Mac implementation the download animation, displayed
// at the start of a download. The animation produces an arrow pointing
// downwards and animates towards the bottom of the window where the new
// download appears in the download shelf.

#include "chrome/browser/download/download_started_animation.h"

#import <QuartzCore/QuartzCore.h>

#include "base/logging.h"
#include "chrome/app/vector_icons/vector_icons.h"
#import "chrome/browser/ui/cocoa/animatable_image.h"
#import "chrome/browser/ui/cocoa/md_util.h"
#import "chrome/browser/ui/cocoa/nsview_additions.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/theme_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/color_palette.h"
#import "ui/gfx/image/image_skia_util_mac.h"
#include "ui/gfx/paint_vector_icon.h"

namespace {
constexpr CGFloat kLeftMargin = 10;
constexpr CGFloat kMDDownloadStartedImageSize = 72;
}  // namespace

class DownloadAnimationWebObserver;

// A class for managing the Core Animation download animation.
// Should be instantiated using +startAnimationWithWebContents:.
@interface DownloadStartedAnimationMac : NSObject {
 @private
  CGFloat imageWidth_;
  AnimatableImage* animation_;
};

+ (void)startAnimationWithWebContents:(content::WebContents*)webContents;

@end

@implementation DownloadStartedAnimationMac

- (id)initWithWebContents:(content::WebContents*)webContents {
  if ((self = [super init])) {
    // Load the image of the download arrow.
    ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
    NSImage* image =
        base::FeatureList::IsEnabled(features::kMacMaterialDesignDownloadShelf)
            ? NSImageFromImageSkia(gfx::CreateVectorIcon(
                  kFileDownloadShelfIcon, kMDDownloadStartedImageSize,
                  gfx::kGoogleBlue500))
            : bundle.GetNativeImageNamed(IDR_DOWNLOAD_ANIMATION_BEGIN)
                  .ToNSImage();

    // Figure out the positioning in the current tab. Try to position the layer
    // against the left edge, and three times the download image's height from
    // the bottom of the tab, assuming there is enough room. If there isn't
    // enough, don't show the animation and let the shelf speak for itself.
    gfx::Rect bounds = webContents->GetContainerBounds();
    imageWidth_ = [image size].width;
    CGFloat imageHeight = [image size].height;

    // Sanity check the size in case there's no room to display the animation.
    if (bounds.height() < imageHeight) {
      [self release];
      return nil;
    }

    NSView* tabContentsView = webContents->GetNativeView();
    NSWindow* parentWindow = [tabContentsView window];
    if (!parentWindow) {
      // The tab is no longer frontmost.
      [self release];
      return nil;
    }

    NSRect frame = [tabContentsView frame];
    CGFloat animationHeight = MIN(bounds.height(), 4 * imageHeight);
    frame.size = NSMakeSize(imageWidth_, animationHeight);
    if (base::FeatureList::IsEnabled(
            features::kMacMaterialDesignDownloadShelf)) {
      frame.origin.x += kLeftMargin;
    }
    frame =
        [tabContentsView convertRect:[tabContentsView cr_localizedRect:frame]
                              toView:nil];
    frame = [parentWindow convertRectToScreen:frame];

    // Create the animation object to assist in animating and fading.
    animation_ = [[AnimatableImage alloc] initWithImage:image
                                         animationFrame:frame];
    [parentWindow addChildWindow:animation_ ordered:NSWindowAbove];

    animationHeight = MIN(bounds.height(), 3 * imageHeight);
    [animation_ setStartFrame:CGRectMake(0, animationHeight,
                                         imageWidth_, imageHeight)];
    [animation_ setEndFrame:CGRectMake(0, imageHeight,
                                       imageWidth_, imageHeight)];
    [animation_ setStartOpacity:1.0];
    [animation_ setEndOpacity:0.4];
    [animation_ setDuration:0.6];
    if (base::FeatureList::IsEnabled(
            features::kMacMaterialDesignDownloadShelf)) {
      [animation_ setTimingFunction:CAMediaTimingFunction
                                        .cr_materialEaseInOutTimingFunction];
    }

    // Set up to get notified about resize events on the parent window.
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(parentWindowDidResize:)
                   name:NSWindowDidResizeNotification
                 object:parentWindow];
    // When the animation window closes, it needs to be removed from the
    // parent window.
    [center addObserver:self
               selector:@selector(windowWillClose:)
                   name:NSWindowWillCloseNotification
                 object:animation_];
    // If the parent window closes, shut everything down too.
    [center addObserver:self
               selector:@selector(windowWillClose:)
                   name:NSWindowWillCloseNotification
                 object:parentWindow];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

// Called when the parent window is resized.
- (void)parentWindowDidResize:(NSNotification*)notification {
  NSWindow* parentWindow = [animation_ parentWindow];
  DCHECK([[notification object] isEqual:parentWindow]);
  NSRect parentFrame = [parentWindow frame];
  NSRect frame = parentFrame;
  frame.size.width = MIN(imageWidth_, NSWidth(parentFrame));
  [animation_ setFrame:frame display:YES];
}

// When the animation closes, release self.
- (void)windowWillClose:(NSNotification*)notification {
  [[animation_ parentWindow] removeChildWindow:animation_];
  [self release];
}

+ (void)startAnimationWithWebContents:(content::WebContents*)contents {
  // Will be deleted when the animation window closes.
  DownloadStartedAnimationMac* controller =
      [[self alloc] initWithWebContents:contents];
  // The initializer can return nil.
  if (!controller)
    return;

  // The |controller| releases itself when done.
  [controller->animation_ startAnimation];
}

@end

void DownloadStartedAnimation::ShowCocoa(content::WebContents* web_contents) {
  DCHECK(web_contents);

  // There's code above for an MD version of the animation, but the current
  // plan is to not show it at all.
  if (base::FeatureList::IsEnabled(features::kMacMaterialDesignDownloadShelf))
    return;

  // Will be deleted when the animation is complete.
  [DownloadStartedAnimationMac startAnimationWithWebContents:web_contents];
}

#if !BUILDFLAG(MAC_VIEWS_BROWSER)
void DownloadStartedAnimation::Show(content::WebContents* web_contents) {
  ShowCocoa(web_contents);
}
#endif

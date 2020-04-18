// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/infobars/infobar_container_controller.h"

#include "base/logging.h"
#include "chrome/browser/infobars/infobar_service.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_container_cocoa.h"
#import "chrome/browser/ui/cocoa/infobars/infobar_controller.h"
#import "chrome/browser/ui/cocoa/view_id_util.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_container.h"

@interface InfoBarContainerController ()
// Removes |controller| from the list of controllers in this container and
// removes its view from the view hierarchy.  This method is safe to call while
// |controller| is still on the call stack.
- (void)removeController:(InfoBarController*)controller;
@end


@implementation InfoBarContainerController

- (id)initWithResizeDelegate:(id<ViewResizer>)resizeDelegate {
  DCHECK(resizeDelegate);
  if ((self = [super initWithNibName:nil bundle:nil])) {
    // This view and its subviews use autoresizing masks, The starting frame
    // needs to be reasonably large, although its exactly values don't matter.
    // It cannot be NSZeroRect.
    base::scoped_nsobject<NSView> view(
        [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 800, 100)]);
    [view setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
    view_id_util::SetID(view, VIEW_ID_INFO_BAR_CONTAINER);
    [self setView:view];

    resizeDelegate_ = resizeDelegate;
    containerCocoa_.reset(new InfoBarContainerCocoa(self));
    infobarControllers_.reset([[NSMutableArray alloc] init]);
  }
  return self;
}

- (void)dealloc {
  // Delete the container so that any remaining infobars are removed.
  containerCocoa_.reset();
  DCHECK_EQ([infobarControllers_ count], 0U);
  view_id_util::UnsetID([self view]);
  [super dealloc];
}

- (void)changeWebContents:(content::WebContents*)contents {
  currentWebContents_ = contents;
  InfoBarService* infobar_service =
      contents ? InfoBarService::FromWebContents(contents) : NULL;
  containerCocoa_->ChangeInfoBarManager(infobar_service);
}

- (void)tabDetachedWithContents:(content::WebContents*)contents {
  if (currentWebContents_ == contents)
    [self changeWebContents:NULL];
}

- (void)addInfoBar:(InfoBarCocoa*)infobar
          position:(NSUInteger)position {
  [infobarControllers_ insertObject:infobar->controller() atIndex:position];

  NSView* relativeView = nil;
  if (position > 0)
    relativeView = [[infobarControllers_ objectAtIndex:position - 1] view];
  [[self view] addSubview:[infobar->controller() view]
               positioned:NSWindowAbove
               relativeTo:relativeView];
}

- (void)removeInfoBar:(InfoBarCocoa*)infobar {
  [infobar->controller() infobarWillHide];
  [self removeController:infobar->controller()];
}

- (void)positionInfoBarsAndRedraw:(BOOL)isAnimating {
  if (isAnimating_ != isAnimating) {
    isAnimating_ = isAnimating;
    if ([resizeDelegate_ respondsToSelector:@selector(setAnimationInProgress:)])
      [resizeDelegate_ setAnimationInProgress:isAnimating_];
  }

  NSRect containerBounds = [[self view] bounds];
  int minY = 0;

  // Stack the infobars at the bottom of the view, starting with the
  // last infobar and working our way to the front of the array.  This
  // way we ensure that the first infobar added shows up on top, with
  // the others below.
  for (InfoBarController* controller in
           [infobarControllers_ reverseObjectEnumerator]) {
    NSRect frame;
    frame.origin.x = NSMinX(containerBounds);
    frame.origin.y = minY;
    frame.size.width = NSWidth(containerBounds);
    frame.size.height = [controller infobar]->computed_height();
    [[controller view] setFrame:frame];

    minY += NSHeight(frame);
  }

  [resizeDelegate_ resizeView:[self view] newHeight:minY];
}

- (void)removeController:(InfoBarController*)controller {
  if (![infobarControllers_ containsObject:controller])
    return;

  // This code can be executed while InfoBarController is still on the stack, so
  // we retain and autorelease the controller to prevent it from being
  // dealloc'ed too early.
  [[controller retain] autorelease];
  [[controller view] removeFromSuperview];
  [infobarControllers_ removeObject:controller];
}

- (CGFloat)heightOfInfoBars {
  CGFloat totalHeight = 0;
  for (InfoBarController* controller in infobarControllers_.get())
    totalHeight += [controller infobar]->computed_height();
  return totalHeight;
}

@end

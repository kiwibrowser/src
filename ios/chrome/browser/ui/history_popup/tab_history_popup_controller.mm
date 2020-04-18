// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history_popup/tab_history_popup_controller.h"

#import <QuartzCore/QuartzCore.h>

#include "base/logging.h"
#include "base/mac/bundle_locations.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/history_popup/tab_history_view_controller.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_view.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/common/material_timing.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ios/web/public/navigation_item.h"
#import "ui/gfx/ios/NSString+CrStringDrawing.h"
#include "ui/gfx/ios/uikit_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kTabHistoryMinWidth = 250.0;
const CGFloat kTabHistoryMaxWidthLandscapePhone = 350.0;
// x coordinate for the textLabel in a default table cell with an image.
const CGFloat kCellTextXCoordinate = 60.0;
// The corner radius for the popover container view.
const CGFloat kPopoverCornerRadius = 2.0;
// Inset for the shadows of the contained views.
NS_INLINE UIEdgeInsets TabHistoryPopupMenuInsets() {
  return UIEdgeInsetsMakeDirected(9, 11, 12, 11);
}

// Layout for popup: shift twelve pixels towards the leading edge.
LayoutOffset kHistoryPopupLeadingOffset = -12;
CGFloat kHistoryPopupYOffset = 3;
// Occupation of the Tools Container as a percentage of the parent height.
static const CGFloat kHeightPercentage = 0.85;
}  // anonymous namespace

@interface TabHistoryPopupController ()

// The UITableViewController displaying Tab history items.
@property(nonatomic, strong)
    TabHistoryViewController* tabHistoryTableViewController;
// The container view that displays |tabHistoryTableViewController|.
@property(nonatomic, strong) UIView* tabHistoryTableViewContainer;

// Determines the width for the popup depending on the device, orientation, and
// number of NavigationItems to display.
+ (CGFloat)popupWidthForItems:(const web::NavigationItemList)items;

@end

@implementation TabHistoryPopupController

@synthesize tabHistoryTableViewController = _tabHistoryTableViewController;
@synthesize tabHistoryTableViewContainer = _tabHistoryTableViewContainer;

- (id)initWithOrigin:(CGPoint)origin
          parentView:(UIView*)parent
               items:(const web::NavigationItemList&)items
          dispatcher:(id<TabHistoryPopupCommands>)dispatcher {
  DCHECK(parent);
  if ((self = [super initWithParentView:parent])) {
    // Create the table view controller.
    _tabHistoryTableViewController =
        [[TabHistoryViewController alloc] initWithItems:items
                                             dispatcher:dispatcher];

    // Set up the container view.
    _tabHistoryTableViewContainer = [[UIView alloc] initWithFrame:CGRectZero];
    _tabHistoryTableViewContainer.layer.cornerRadius = kPopoverCornerRadius;
    _tabHistoryTableViewContainer.layer.masksToBounds = YES;
    [_tabHistoryTableViewContainer
        addSubview:_tabHistoryTableViewController.view];

    // Calculate the optimal popup size.
    LayoutOffset originOffset =
        kHistoryPopupLeadingOffset - TabHistoryPopupMenuInsets().left;
    CGPoint newOrigin = CGPointLayoutOffset(origin, originOffset);
    newOrigin.y += kHistoryPopupYOffset;
    CGFloat availableHeight =
        (CGRectGetHeight([parent bounds]) - origin.y) * kHeightPercentage;
    CGFloat optimalHeight =
        [_tabHistoryTableViewController optimalHeight:availableHeight];
    CGFloat popupWidth = [[self class] popupWidthForItems:items];
    [self setOptimalSize:CGSizeMake(popupWidth, optimalHeight)
                atOrigin:newOrigin];

    // Fade in the popup.
    CGRect containerFrame = [[self popupContainer] frame];
    CGPoint destination = CGPointMake(CGRectGetLeadingEdge(containerFrame),
                                      CGRectGetMinY(containerFrame));
    [self fadeInPopupFromSource:origin toDestination:destination];
  }
  return self;
}

- (void)dealloc {
  [_tabHistoryTableViewContainer removeFromSuperview];
}

#pragma mark - PopupMenuController

- (void)fadeInPopupFromSource:(CGPoint)source
                toDestination:(CGPoint)destination {
  // Add the container view to the popup view and resize.
  if (!_tabHistoryTableViewContainer.superview)
    [self.popupContainer addSubview:_tabHistoryTableViewContainer];
  _tabHistoryTableViewContainer.frame = UIEdgeInsetsInsetRect(
      self.popupContainer.bounds, TabHistoryPopupMenuInsets());
  _tabHistoryTableViewController.view.frame =
      _tabHistoryTableViewContainer.bounds;

  // Set up the animation.
  [_tabHistoryTableViewContainer setAlpha:0];
  [UIView animateWithDuration:ios::material::kDuration1
                   animations:^{
                     [_tabHistoryTableViewContainer setAlpha:1];
                   }];
  [super fadeInPopupFromSource:source toDestination:destination completion:nil];
}

- (void)dismissAnimatedWithCompletion:(void (^)(void))completion {
  [_tabHistoryTableViewContainer setAlpha:0];
  [super dismissAnimatedWithCompletion:completion];
}

#pragma mark -

+ (CGFloat)popupWidthForItems:(const web::NavigationItemList)items {
  CGFloat maxWidth;

  // Determine the maximum width for the device and orientation.

  if (!IsIPadIdiom()) {
    UIInterfaceOrientation orientation =
        [[UIApplication sharedApplication] statusBarOrientation];
    // Phone in portrait has constant width.
    if (UIInterfaceOrientationIsPortrait(orientation))
      return kTabHistoryMinWidth;
    maxWidth = kTabHistoryMaxWidthLandscapePhone;
  } else {
    // On iPad use 85% of the available width.
    maxWidth = ui::AlignValueToUpperPixel(
        [UIApplication sharedApplication].keyWindow.frame.size.width * .85);
  }
  // Increase the width to fit the text to display but don't exceed maxWidth.
  CGFloat cellWidth = kTabHistoryMinWidth;
  UIFont* font = [[MDCTypography fontLoader] regularFontOfSize:16];
  for (web::NavigationItem* item : items) {
    // Can this be replaced with GetTitleForDisplay()?
    NSString* cellText = item->GetTitle().empty()
                             ? base::SysUTF8ToNSString(item->GetURL().spec())
                             : base::SysUTF16ToNSString(item->GetTitle());
    CGFloat contentWidth = [cellText cr_pixelAlignedSizeWithFont:font].width +
                           kCellTextXCoordinate;

    // If contentWidth is larger than maxWidth, return maxWidth instead of
    // checking the rest of the items.
    if (contentWidth > maxWidth)
      return maxWidth;
    if (contentWidth > cellWidth)
      cellWidth = contentWidth;
  }
  return cellWidth;
}

@end

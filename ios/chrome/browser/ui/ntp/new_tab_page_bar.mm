// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_bar.h"

#import <QuartzCore/QuartzCore.h>
#include <cmath>

#include "base/logging.h"

#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_button.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_item.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ui/gfx/ios/NSString+CrStringDrawing.h"
#include "ui/gfx/scoped_ui_graphics_push_context_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kBarHeight = 48.0f;

const CGFloat kRegularLayoutButtonWidth = 168;

}  // anonymous namespace

@interface NewTabPageBar () {
  UIImageView* shadow_;
}

@property(nonatomic, readwrite, strong) NSArray* buttons;

// View containing the content and respecting the safe area.
@property(nonatomic, strong) UIView* contentView;

- (void)setup;
- (void)calculateButtonWidth;
- (void)setupButton:(UIButton*)button;
@end

@implementation NewTabPageBar {
  // Don't allow tabbar animations on startup, only after first tap.
  BOOL canAnimate_;
  // Overlay view, used to highlight the selected button.
  UIImageView* overlayView_;
  // Overlay view, used to highlight the selected button.
  UIView* overlayColorView_;
  // Width of a button.
  CGFloat buttonWidth_;
}

@synthesize items = items_;
@synthesize selectedIndex = selectedIndex_;
@synthesize buttons = buttons_;
@synthesize delegate = delegate_;
@synthesize overlayPercentage = overlayPercentage_;
@synthesize contentView = _contentView;
@synthesize safeAreaInsetFromNTPView = _safeAreaInsetFromNTPView;

- (id)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setup];
  }
  return self;
}

- (id)initWithCoder:(NSCoder*)aDecoder {
  self = [super initWithCoder:aDecoder];
  if (self) {
    [self setup];
  }
  return self;
}

- (void)setup {
  self.selectedIndex = NSNotFound;
  canAnimate_ = NO;
  self.backgroundColor = [UIColor whiteColor];

  _contentView = [[UIView alloc] initWithFrame:CGRectZero];
  [self addSubview:_contentView];

  // Make the drop shadow.
  UIImage* shadowImage = [UIImage imageNamed:@"ntp_bottom_bar_shadow"];
  shadow_ = [[UIImageView alloc] initWithImage:shadowImage];
  // Shadow is positioned directly above the new tab page bar.
  [shadow_
      setFrame:CGRectMake(0, -shadowImage.size.height, self.bounds.size.width,
                          shadowImage.size.height)];
  [shadow_ setAutoresizingMask:UIViewAutoresizingFlexibleWidth];
  [self addSubview:shadow_];

  self.contentMode = UIViewContentModeRedraw;
}

- (void)layoutSubviews {
  [super layoutSubviews];

  CGRect contentViewFrame = self.frame;
  contentViewFrame.size.height -= self.safeAreaInsetFromNTPView.bottom;
  contentViewFrame.origin.y = 0;
  self.contentView.frame = contentViewFrame;

  // |buttonWidth_| changes with the screen orientation when the NTP button bar
  // is enabled.
  [self calculateButtonWidth];

  CGFloat buttonPadding = floor((CGRectGetWidth(self.contentView.bounds) -
                                 buttonWidth_ * self.buttons.count) /
                                2);

  for (NSUInteger i = 0; i < self.buttons.count; ++i) {
    NewTabPageBarButton* button = [self.buttons objectAtIndex:i];
    LayoutRect layout =
        LayoutRectMake(buttonPadding + (i * buttonWidth_),
                       CGRectGetWidth(self.contentView.bounds), 0, buttonWidth_,
                       CGRectGetHeight(self.contentView.bounds));
    button.frame = LayoutRectGetRect(layout);
    [button setContentToDisplay:new_tab_page_bar_button::ContentType::IMAGE];
  }

  // Position overlay image over percentage of tab bar button(s).
  CGRect frame = [overlayView_ frame];
  frame.origin.x = floor(
      buttonWidth_ * self.buttons.count * overlayPercentage_ + buttonPadding);
  frame.size.width = buttonWidth_;
  DCHECK(!std::isnan(frame.origin.x));
  DCHECK(!std::isnan(frame.size.width));
  [overlayView_ setFrame:frame];
}

- (CGSize)sizeThatFits:(CGSize)size {
  return CGSizeMake(size.width,
                    kBarHeight + self.safeAreaInsetFromNTPView.bottom);
}

- (void)calculateButtonWidth {
  if (IsCompactWidth()) {
    if ([items_ count] > 0) {
      buttonWidth_ = self.contentView.bounds.size.width / [items_ count];
    } else {
      // In incognito on phones, there are no items shown.
      buttonWidth_ = 0;
    }
    return;
  }

  buttonWidth_ = kRegularLayoutButtonWidth;
}

// When setting a new set of items on the tab bar, the buttons need to be
// regenerated and the old buttons need to be removed.
- (void)setItems:(NSArray*)newItems {
  if (newItems == items_)
    return;

  items_ = newItems;
  // Remove all the existing buttons from the view.
  for (UIButton* button in self.buttons) {
    [button removeFromSuperview];
  }

  // Create a set of new buttons.
  [self calculateButtonWidth];
  if (newItems.count) {
    NSMutableArray* newButtons = [NSMutableArray array];
    for (NSUInteger i = 0; i < newItems.count; ++i) {
      NewTabPageBarItem* item = [newItems objectAtIndex:i];
      NewTabPageBarButton* button = [NewTabPageBarButton buttonWithItem:item];
      button.frame = CGRectIntegral(
          CGRectMake(i * buttonWidth_, 0, buttonWidth_, kBarHeight));
      [self setupButton:button];
      [self.contentView addSubview:button];
      [newButtons addObject:button];
    }
    self.buttons = newButtons;
  } else {
    self.buttons = nil;
  }
  [self setNeedsLayout];
}

- (void)setOverlayPercentage:(CGFloat)overlayPercentage {
  DCHECK(!std::isnan(overlayPercentage));
  overlayPercentage_ = overlayPercentage;
  [self setNeedsLayout];
}

- (void)setupButton:(UIButton*)button {
  // Treat the NTP buttons as normal buttons, where the standard is to fire an
  // action on touch up inside.
  [button addTarget:self
                action:@selector(buttonDidTap:)
      forControlEvents:UIControlEventTouchUpInside];
}

- (void)buttonDidTap:(UIButton*)button {
  canAnimate_ = YES;
  NSUInteger buttonIndex = [self.buttons indexOfObject:button];
  if (buttonIndex != NSNotFound) {
    self.selectedIndex = buttonIndex;
    [delegate_ newTabBarItemDidChange:[self.items objectAtIndex:buttonIndex]];
  }
}

- (void)setShadowAlpha:(CGFloat)alpha {
  CGFloat currentAlpha = [shadow_ alpha];
  CGFloat diff = std::abs(alpha - currentAlpha);
  if (diff >= 1) {
    [UIView animateWithDuration:0.3
                     animations:^{
                       [shadow_ setAlpha:alpha];
                     }];
  } else {
    [shadow_ setAlpha:alpha];
  }
}

@end

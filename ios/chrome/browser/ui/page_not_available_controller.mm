// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/page_not_available_controller.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/strings/grit/components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Top padding for |self.titleLabel|.
const CGFloat kTitleLabelTopPadding = 20.0;
// Height for |self.titleLabel|.
const CGFloat kTitleLabelHeight = 38.0;
// Top padding for |self.descriptionView|.
const CGFloat kDescriptionViewTopPadding = 66.0;
// Bottom padding for |self.descriptionView|.
const CGFloat kDescriptionViewBottomPadding = 20.0;
// Horizontal padding between subviews and |self.view|.
const CGFloat kHorizontalPadding = 20.0;
// Font size for |self.titleLabel|.
const CGFloat kTitleLabelFontSize = 18.0;
// Font size for |self.descriptionView|.
const CGFloat kDescriptionViewFontSize = 17.0;
}

@interface PageNotAvailableController () {
}

// The title label displayed centered at the top of the screen.
@property(nonatomic, strong) UILabel* titleLabel;

// TextView containing a detailed description of the problem.
@property(nonatomic, strong) UITextView* descriptionView;

@end

@implementation PageNotAvailableController

@synthesize titleLabel = _titleLabel;
@synthesize descriptionView = _descriptionView;
@synthesize descriptionText = _descriptionText;

- (instancetype)initWithUrl:(const GURL&)url {
  self = [super initWithNibName:nil url:url];
  if (self) {
    // Use the host as the page title, unless the URL has a custom scheme.
    if (self.url.SchemeIsHTTPOrHTTPS()) {
      self.title = base::SysUTF16ToNSString(
          url_formatter::IDNToUnicode(self.url.host()));
    } else {
      base::string16 formattedURL = url_formatter::FormatUrl(
          self.url, url_formatter::kFormatUrlOmitNothing,
          net::UnescapeRule::NORMAL, nullptr, nullptr, nullptr);
      if (base::i18n::IsRTL()) {
        base::i18n::WrapStringWithLTRFormatting(&formattedURL);
      }
      self.title = base::SysUTF16ToNSString(formattedURL);
    }

    // |self.view| setup.
    CGRect windowBounds = [UIApplication sharedApplication].keyWindow.bounds;
    UIView* view = [[UIView alloc] initWithFrame:windowBounds];
    [view setBackgroundColor:[UIColor whiteColor]];
    [view setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                               UIViewAutoresizingFlexibleHeight)];
    self.view = view;

    // |self.titleLabel| setup.
    CGRect titleLabelFrame = windowBounds;
    titleLabelFrame.origin.x += kHorizontalPadding;
    titleLabelFrame.size.width -= 2.0 * kHorizontalPadding;
    titleLabelFrame.origin.y += kTitleLabelTopPadding;
    titleLabelFrame.size.height = kTitleLabelHeight;
    _titleLabel = [[UILabel alloc] initWithFrame:titleLabelFrame];
    _titleLabel.text =
        l10n_util::GetNSString(IDS_ERRORPAGES_HEADING_NOT_AVAILABLE);
    _titleLabel.autoresizingMask = (UIViewAutoresizingFlexibleBottomMargin |
                                    UIViewAutoresizingFlexibleWidth);
    _titleLabel.font =
        [UIFont fontWithName:@"Helvetica-Bold" size:kTitleLabelFontSize];
    _titleLabel.textAlignment = NSTextAlignmentCenter;
    [self.view addSubview:_titleLabel];

    // |self.descriptionView| setup.
    CGRect descriptionViewFrame = windowBounds;
    descriptionViewFrame.origin.x += kHorizontalPadding;
    descriptionViewFrame.size.width -= 2 * kHorizontalPadding;
    descriptionViewFrame.origin.y = kDescriptionViewTopPadding;
    descriptionViewFrame.size.height = CGRectGetHeight(windowBounds) -
                                       descriptionViewFrame.origin.y -
                                       kDescriptionViewBottomPadding;
    _descriptionView = [[UITextView alloc] initWithFrame:descriptionViewFrame];
    _descriptionView.text = l10n_util::GetNSStringF(
        IDS_ERRORPAGES_SUMMARY_NOT_AVAILABLE_NO_EMPHASIS,
        base::UTF8ToUTF16(self.url.spec()));
    _descriptionView.autoresizingMask =
        UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
    _descriptionView.font = [UIFont systemFontOfSize:kDescriptionViewFontSize];
    _descriptionView.editable = NO;
    [self.view addSubview:_descriptionView];
  }
  return self;
}

- (instancetype)initWithNibName:(NSString*)nibName
                            url:(const GURL&)url NS_UNAVAILABLE {
  NOTREACHED();
  return nil;
}

- (void)setDescriptionText:(NSString*)descriptionText {
  _descriptionText = [descriptionText copy];
  _descriptionView.text = _descriptionText;
}

@end

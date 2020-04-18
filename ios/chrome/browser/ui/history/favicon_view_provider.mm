// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/favicon_view_provider.h"

#include "base/i18n/case_conversion.h"
#include "base/mac/bind_objc_block.h"
#import "base/mac/foundation_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon/core/fallback_url_util.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon_base/fallback_icon_style.h"
#include "components/favicon_base/favicon_types.h"
#import "ios/chrome/browser/ui/history/favicon_view.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "skia/ext/skia_utils_ios.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface FaviconViewProvider () {
  // Delegate for handling completion of favicon load.
  __weak id<FaviconViewProviderDelegate> _delegate;
  // Used to cancel tasks for the LargeIconService.
  base::CancelableTaskTracker _faviconTaskTracker;
}

// Size to render the favicon.
@property(nonatomic, assign) CGFloat faviconSize;
// Favicon image for the favicon view.
@property(nonatomic, strong) UIImage* favicon;
// Fallback text for the favicon view if there is no appropriately sized
// favicon availabile.
@property(nonatomic, copy) NSString* fallbackText;
// Fallback background color for the favicon view if there is no appropriately
// sized favicon available.
@property(nonatomic, strong) UIColor* fallbackBackgroundColor;
// Fallback text color for the favicon view if there is no appropriately
// sized favicon available.
@property(nonatomic, strong) UIColor* fallbackTextColor;

// Fetches favicon for |URL| from |faviconService|. Notifies delegate when
// favicon is retrieved.
- (void)fetchFaviconForURL:(const GURL&)URL
                      size:(CGFloat)size
                   minSize:(CGFloat)minSize
                   service:(favicon::LargeIconService*)faviconService;

@end

@implementation FaviconViewProvider

@synthesize faviconSize = _faviconSize;
@synthesize favicon = _favicon;
@synthesize fallbackText = _fallbackText;
@synthesize fallbackBackgroundColor = _fallbackBackgroundColor;
@synthesize fallbackTextColor = _fallbackTextColor;
@synthesize faviconView = _faviconView;

- (instancetype)initWithURL:(const GURL&)URL
                faviconSize:(CGFloat)faviconSize
             minFaviconSize:(CGFloat)minFaviconSize
           largeIconService:(favicon::LargeIconService*)largeIconService
                   delegate:(id<FaviconViewProviderDelegate>)delegate {
  self = [super init];
  if (self) {
    _faviconSize = faviconSize;
    _delegate = delegate;
    _fallbackBackgroundColor = [UIColor grayColor];
    _fallbackTextColor = [UIColor whiteColor];
    [self fetchFaviconForURL:URL
                        size:faviconSize
                     minSize:minFaviconSize
                     service:largeIconService];
  }
  return self;
}

- (instancetype)init {
  NOTREACHED();
  return nil;
}

- (void)fetchFaviconForURL:(const GURL&)URL
                      size:(CGFloat)size
                   minSize:(CGFloat)minSize
                   service:(favicon::LargeIconService*)largeIconService {
  if (!largeIconService)
    return;
  __weak FaviconViewProvider* weakSelf = self;
  GURL blockURL(URL);
  void (^faviconBlock)(const favicon_base::LargeIconResult&) = ^(
      const favicon_base::LargeIconResult& result) {
    FaviconViewProvider* strongSelf = weakSelf;
    if (!strongSelf)
      return;
    if (result.bitmap.is_valid()) {
      scoped_refptr<base::RefCountedMemory> data =
          result.bitmap.bitmap_data.get();
      [strongSelf
          setFavicon:[UIImage
                         imageWithData:[NSData dataWithBytes:data->front()
                                                      length:data->size()]]];
    } else if (result.fallback_icon_style) {
      [strongSelf setFallbackBackgroundColor:skia::UIColorFromSkColor(
                                                 result.fallback_icon_style
                                                     ->background_color)];
      [strongSelf
          setFallbackTextColor:skia::UIColorFromSkColor(
                                   result.fallback_icon_style->text_color)];
      [strongSelf setFallbackText:base::SysUTF16ToNSString(
                                      favicon::GetFallbackIconText(blockURL))];
    }
    [strongSelf->_delegate faviconViewProviderFaviconDidLoad:strongSelf];
  };

  // Always call LargeIconService in case the favicon was updated.
  CGFloat faviconSize = [UIScreen mainScreen].scale * size;
  CGFloat minFaviconSize = [UIScreen mainScreen].scale * minSize;
  largeIconService->GetLargeIconOrFallbackStyle(
      URL, minFaviconSize, faviconSize, base::BindBlockArc(faviconBlock),
      &_faviconTaskTracker);
}

- (FaviconView*)faviconView {
  if (!_faviconView) {
    _faviconView = [[FaviconView alloc] initWithFrame:CGRectZero];
  }
  _faviconView.size = _faviconSize;
  // Update favicon view with current properties.
  if (self.favicon) {
    _faviconView.faviconImage.image = self.favicon;
    _faviconView.faviconImage.backgroundColor = [UIColor whiteColor];
    _faviconView.faviconFallbackLabel.text = nil;
  } else {
    _faviconView.faviconImage.image = nil;
    _faviconView.faviconImage.backgroundColor = self.fallbackBackgroundColor;
    _faviconView.faviconFallbackLabel.text = self.fallbackText;
    _faviconView.faviconFallbackLabel.textColor = self.fallbackTextColor;

    CGFloat fontSize = floorf(_faviconSize / 2);
    _faviconView.faviconFallbackLabel.font =
        [[MDCTypography fontLoader] regularFontOfSize:fontSize];
  }
  return _faviconView;
}

@end

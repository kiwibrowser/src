// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_HISTORY_FAVICON_VIEW_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_HISTORY_FAVICON_VIEW_PROVIDER_H_

#import <UIKit/UIKit.h>

namespace favicon {
class LargeIconService;
}  // namespace favicon

@class FaviconView;
@class FaviconViewProvider;
class GURL;

// Delegate protocol for FaviconViewProvider.
@protocol FaviconViewProviderDelegate<NSObject>
// Called when favicon or fallback format has been fetched.
- (void)faviconViewProviderFaviconDidLoad:(FaviconViewProvider*)provider;
@end

// Object to fetch and configure the view for a favicon, or a fallback icon if
// there is no favicon image available with large enough resolution.
@interface FaviconViewProvider : NSObject
// A favicon or fallback format associated with |URL| will be fetched using
// |largeIconService|. The favicon will be rendered with height and width equal
// to |faviconSize|, and the image will be fetched if the source size is greater
// than or equal to |minFaviconSize|. The |delegate| is notified when the
// favicon has been loaded, and may be nil.
- (instancetype)initWithURL:(const GURL&)URL
                faviconSize:(CGFloat)faviconSize
             minFaviconSize:(CGFloat)minFaviconSize
           largeIconService:(favicon::LargeIconService*)largeIconService
                   delegate:(id<FaviconViewProviderDelegate>)delegate
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

// View that displays a favicon or fallback icon.
@property(strong, nonatomic, readonly) FaviconView* faviconView;

@end

#endif  // IOS_CHROME_BROWSER_UI_HISTORY_FAVICON_VIEW_PROVIDER_H_

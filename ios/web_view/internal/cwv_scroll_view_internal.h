// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_INTERNAL_CWV_SCROLL_VIEW_INTERNAL_H_
#define IOS_WEB_VIEW_INTERNAL_CWV_SCROLL_VIEW_INTERNAL_H_

#import <Foundation/Foundation.h>

#import "ios/web_view/public/cwv_scroll_view.h"

NS_ASSUME_NONNULL_BEGIN

@class CRWWebViewScrollViewProxy;

@interface CWVScrollView ()

// Operations to this scroll view are delegated to this proxy.
@property(nonatomic, weak, readwrite) CRWWebViewScrollViewProxy* proxy;

@end

NS_ASSUME_NONNULL_END

#endif  // IOS_WEB_VIEW_INTERNAL_CWV_SCROLL_VIEW_INTERNAL_H_

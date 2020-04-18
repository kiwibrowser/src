// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BrowserListObserver::BrowserListObserver() = default;

BrowserListObserver::~BrowserListObserver() = default;

void BrowserListObserver::OnBrowserCreated(BrowserList* browser_list,
                                           Browser* browser) {}

void BrowserListObserver::OnBrowserRemoved(BrowserList* browser_list,
                                           Browser* browser) {}

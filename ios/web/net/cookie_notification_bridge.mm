// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/cookie_notification_bridge.h"

#import <Foundation/Foundation.h>

#include "base/bind.h"
#include "base/location.h"
#import "ios/net/cookies/cookie_store_ios.h"
#include "ios/web/public/web_thread.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

CookieNotificationBridge::CookieNotificationBridge() {
  id<NSObject> observer = [[NSNotificationCenter defaultCenter]
      addObserverForName:NSHTTPCookieManagerCookiesChangedNotification
                  object:[NSHTTPCookieStorage sharedHTTPCookieStorage]
                   queue:nil
              usingBlock:^(NSNotification* notification) {
                OnNotificationReceived(notification);
              }];
  observer_ = observer;
}

CookieNotificationBridge::~CookieNotificationBridge() {
  [[NSNotificationCenter defaultCenter] removeObserver:observer_];
}

void CookieNotificationBridge::OnNotificationReceived(
    NSNotification* notification) {
  DCHECK([[notification name]
      isEqualToString:NSHTTPCookieManagerCookiesChangedNotification]);
  web::WebThread::PostTask(
      web::WebThread::IO, FROM_HERE,
      base::Bind(&net::CookieStoreIOS::NotifySystemCookiesChanged));
}

}  // namespace web

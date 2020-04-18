// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/cookies/cookie_store_ios_persistent.h"

#import <Foundation/Foundation.h>

#include <memory>

#include "base/threading/thread_checker.h"
#import "ios/net/cookies/ns_http_system_cookie_store.h"
#import "ios/net/cookies/system_cookie_util.h"
#include "net/cookies/cookie_monster.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

#pragma mark -
#pragma mark CookieStoreIOSPersistent

CookieStoreIOSPersistent::CookieStoreIOSPersistent(
    net::CookieMonster::PersistentCookieStore* persistent_store)
    : CookieStoreIOS(persistent_store,
                     std::make_unique<net::NSHTTPSystemCookieStore>()) {}

CookieStoreIOSPersistent::CookieStoreIOSPersistent(
    net::CookieMonster::PersistentCookieStore* persistent_store,
    std::unique_ptr<SystemCookieStore> system_store)
    : CookieStoreIOS(persistent_store, std::move(system_store)) {}

CookieStoreIOSPersistent::~CookieStoreIOSPersistent() {}

#pragma mark -
#pragma mark CookieStoreIOSPersistent methods

void CookieStoreIOSPersistent::SetCookieWithOptionsAsync(
    const GURL& url,
    const std::string& cookie_line,
    const net::CookieOptions& options,
    SetCookiesCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  cookie_monster()->SetCookieWithOptionsAsync(
      url, cookie_line, options, WrapSetCallback(std::move(callback)));
}

void CookieStoreIOSPersistent::SetCanonicalCookieAsync(
    std::unique_ptr<CanonicalCookie> cookie,
    bool secure_source,
    bool modify_http_only,
    SetCookiesCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  cookie_monster()->SetCanonicalCookieAsync(
      std::move(cookie), secure_source, modify_http_only,
      WrapSetCallback(std::move(callback)));
}

void CookieStoreIOSPersistent::GetCookieListWithOptionsAsync(
    const GURL& url,
    const net::CookieOptions& options,
    GetCookieListCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  cookie_monster()->GetCookieListWithOptionsAsync(url, options,
                                                  std::move(callback));
}

void CookieStoreIOSPersistent::GetAllCookiesAsync(
    GetCookieListCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  cookie_monster()->GetAllCookiesAsync(std::move(callback));
}

void CookieStoreIOSPersistent::DeleteCookieAsync(const GURL& url,
                                                 const std::string& cookie_name,
                                                 base::OnceClosure callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  cookie_monster()->DeleteCookieAsync(url, cookie_name,
                                      WrapClosure(std::move(callback)));
}

void CookieStoreIOSPersistent::DeleteCanonicalCookieAsync(
    const CanonicalCookie& cookie,
    DeleteCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  cookie_monster()->DeleteCanonicalCookieAsync(
      cookie, WrapDeleteCallback(std::move(callback)));
}

void CookieStoreIOSPersistent::DeleteAllCreatedInTimeRangeAsync(
    const net::CookieDeletionInfo::TimeRange& creation_range,
    DeleteCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (metrics_enabled())
    ResetCookieCountMetrics();

  cookie_monster()->DeleteAllCreatedInTimeRangeAsync(
      creation_range, WrapDeleteCallback(std::move(callback)));
}

void CookieStoreIOSPersistent::DeleteAllMatchingInfoAsync(
    CookieDeletionInfo delete_info,
    DeleteCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (metrics_enabled())
    ResetCookieCountMetrics();

  cookie_monster()->DeleteAllMatchingInfoAsync(
      std::move(delete_info), WrapDeleteCallback(std::move(callback)));
}

void CookieStoreIOSPersistent::DeleteSessionCookiesAsync(
    DeleteCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (metrics_enabled())
    ResetCookieCountMetrics();

  cookie_monster()->DeleteSessionCookiesAsync(
      WrapDeleteCallback(std::move(callback)));
}

#pragma mark -
#pragma mark Private methods

void CookieStoreIOSPersistent::WriteToCookieMonster(NSArray* system_cookies) {}

void CookieStoreIOSPersistent::OnSystemCookiesChanged() {}

}  // namespace net

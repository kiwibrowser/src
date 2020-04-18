// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_COOKIES_COOKIE_STORE_IOS_PERSISTENT_H_
#define IOS_NET_COOKIES_COOKIE_STORE_IOS_PERSISTENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/time/time.h"
#import "ios/net/cookies/cookie_store_ios.h"
#include "net/cookies/cookie_monster.h"
#include "net/cookies/cookie_store.h"
#include "url/gurl.h"

namespace net {

// The CookieStoreIOSPersistent is an implementation of CookieStore relying on
// on backing CookieStore.
// CookieStoreIOSPersistent is not thread safe.
//
// All the changes are written back to the backing CookieStore.
// For synchronized CookieStore, please see CookieStoreIOS.
//
// TODO(crbug.com/686147): evaluate if inheritance is
// needed here.
class CookieStoreIOSPersistent : public CookieStoreIOS {
 public:
  // Constructs a CookieStoreIOS with a default SystemCookieStore.
  explicit CookieStoreIOSPersistent(
      net::CookieMonster::PersistentCookieStore* persistent_store);

  // Constructs a CookieStoreIOS backed by |system_store|.
  CookieStoreIOSPersistent(
      net::CookieMonster::PersistentCookieStore* persistent_store,
      std::unique_ptr<SystemCookieStore> system_store);

  ~CookieStoreIOSPersistent() override;

  // Inherited CookieStore methods.
  void SetCookieWithOptionsAsync(const GURL& url,
                                 const std::string& cookie_line,
                                 const net::CookieOptions& options,
                                 SetCookiesCallback callback) override;
  void SetCanonicalCookieAsync(std::unique_ptr<CanonicalCookie> cookie,
                               bool secure_source,
                               bool modify_http_only,
                               SetCookiesCallback callback) override;
  void GetCookieListWithOptionsAsync(const GURL& url,
                                     const net::CookieOptions& options,
                                     GetCookieListCallback callback) override;
  void GetAllCookiesAsync(GetCookieListCallback callback) override;
  void DeleteCookieAsync(const GURL& url,
                         const std::string& cookie_name,
                         base::OnceClosure callback) override;
  void DeleteCanonicalCookieAsync(const CanonicalCookie& cookie,
                                  DeleteCallback callback) override;
  void DeleteAllCreatedInTimeRangeAsync(
      const net::CookieDeletionInfo::TimeRange& creation_range,
      DeleteCallback callback) override;
  void DeleteAllMatchingInfoAsync(CookieDeletionInfo delete_info,
                                  DeleteCallback callback) override;
  void DeleteSessionCookiesAsync(DeleteCallback callback) override;

 private:
  // No-op functions for this class.
  void WriteToCookieMonster(NSArray* system_cookies) override;
  void OnSystemCookiesChanged() override;

  DISALLOW_COPY_AND_ASSIGN(CookieStoreIOSPersistent);
};

}  // namespace net

#endif  // IOS_NET_COOKIES_COOKIE_STORE_IOS_PERSISTENT_H_

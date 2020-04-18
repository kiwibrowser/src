// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_COOKIE_HELPER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_COOKIE_HELPER_H_

#include <stddef.h>

#include <map>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/browsing_data/canonical_cookie_hash.h"
#include "net/cookies/cookie_monster.h"

class GURL;

namespace net {
class CanonicalCookie;
class URLRequestContextGetter;
}

// This class fetches cookie information on behalf of a caller
// on the UI thread.
// A client of this class need to call StartFetching from the UI thread to
// initiate the flow, and it'll be notified by the callback in its UI
// thread at some later point.
class BrowsingDataCookieHelper
    : public base::RefCountedThreadSafe<BrowsingDataCookieHelper> {
 public:
  using FetchCallback = base::Callback<void(const net::CookieList&)>;
  explicit BrowsingDataCookieHelper(
      net::URLRequestContextGetter* request_context_getter);

  // Starts the fetching process, which will notify its completion via
  // callback.
  // This must be called only in the UI thread.
  virtual void StartFetching(const FetchCallback& callback);

  // Requests a single cookie to be deleted in the IO thread. This must be
  // called in the UI thread.
  virtual void DeleteCookie(const net::CanonicalCookie& cookie);

 protected:
  friend class base::RefCountedThreadSafe<BrowsingDataCookieHelper>;
  virtual ~BrowsingDataCookieHelper();

  net::URLRequestContextGetter* request_context_getter() {
    return request_context_getter_.get();
  }

 private:
  // Fetch the cookies. This must be called in the IO thread.
  void FetchCookiesOnIOThread(const FetchCallback& callback);

  // Delete a single cookie. This must be called in IO thread.
  void DeleteCookieOnIOThread(const net::CanonicalCookie& cookie);

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataCookieHelper);
};

// This class is a thin wrapper around BrowsingDataCookieHelper that does not
// fetch its information from the persistent cookie store. It is a simple
// container for CanonicalCookies. Clients that use this container can add
// cookies that are sent to a server via the AddReadCookies method and cookies
// that are received from a server or set via JavaScript using the method
// AddChangedCookie.
// Cookies are distinguished by the tuple cookie name (called cookie-name in
// RFC 6265), cookie domain (called cookie-domain in RFC 6265), cookie path
// (called cookie-path in RFC 6265) and host-only-flag (see RFC 6265 section
// 5.3). Cookies with same tuple (cookie-name, cookie-domain, cookie-path,
// host-only-flag) as cookie that are already stored, will replace the stored
// cookies.
class CannedBrowsingDataCookieHelper : public BrowsingDataCookieHelper {
 public:
  typedef std::map<GURL, std::unique_ptr<canonical_cookie::CookieHashSet>>
      OriginCookieSetMap;

  explicit CannedBrowsingDataCookieHelper(
      net::URLRequestContextGetter* request_context);

  // Adds the cookies from |cookie_list|. Current cookies that have the same
  // cookie name, cookie domain, cookie path, host-only-flag tuple as passed
  // cookies are replaced by the passed cookies.
  void AddReadCookies(const GURL& frame_url,
                      const GURL& request_url,
                      const net::CookieList& cookie_list);

  // Adds a CanonicalCookie.
  // TODO(markusheintz): Remove the dublicated logic.
  void AddChangedCookie(const GURL& frame_url,
                        const GURL& request_url,
                        const net::CanonicalCookie& cookie);

  // Clears the list of canned cookies.
  void Reset();

  // True if no cookie are currently stored.
  bool empty() const;

  // BrowsingDataCookieHelper methods.
  void StartFetching(const FetchCallback& callback) override;
  void DeleteCookie(const net::CanonicalCookie& cookie) override;

  // Returns the number of stored cookies.
  size_t GetCookieCount() const;

  // Returns the map that contains the cookie lists for all frame urls.
  const OriginCookieSetMap& origin_cookie_set_map() {
    return origin_cookie_set_map_;
  }

 private:
  // Check if the cookie set contains a cookie with the same name,
  // domain, and path as the newly created cookie. Delete the old cookie
  // if does.
  bool DeleteMatchingCookie(const net::CanonicalCookie& add_cookie,
                            canonical_cookie::CookieHashSet* cookie_set);

  ~CannedBrowsingDataCookieHelper() override;

  // Returns the |CookieSet| for the given |origin|.
  canonical_cookie::CookieHashSet* GetCookiesFor(const GURL& origin);

  // Adds the |cookie| to the cookie set for the given |frame_url|.
  void AddCookie(const GURL& frame_url,
                 const net::CanonicalCookie& cookie);

  // Map that contains the cookie sets for all frame origins.
  OriginCookieSetMap origin_cookie_set_map_;

  DISALLOW_COPY_AND_ASSIGN(CannedBrowsingDataCookieHelper);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_COOKIE_HELPER_H_

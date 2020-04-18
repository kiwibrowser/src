// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Provides utility structures for inserting a CanonicalCookie into a hash set.
// Two cookies are considered equal if their names, domains, and paths are
// equivalent.

#ifndef CHROME_BROWSER_BROWSING_DATA_CANONICAL_COOKIE_HASH_H_
#define CHROME_BROWSER_BROWSING_DATA_CANONICAL_COOKIE_HASH_H_

#include <stddef.h>

#include "base/containers/hash_tables.h"
#include "net/cookies/canonical_cookie.h"

namespace canonical_cookie {

// Returns a fast hash of a cookie, based on its name, domain, and path.
size_t FastHash(const net::CanonicalCookie& cookie);

struct CanonicalCookieHasher {
  std::size_t operator()(const net::CanonicalCookie& cookie) const {
    return FastHash(cookie);
  }
};

struct CanonicalCookieComparer {
  bool operator()(const net::CanonicalCookie& cookie1,
                  const net::CanonicalCookie& cookie2) const {
    return cookie1.Name() == cookie2.Name() &&
           cookie1.Domain() == cookie2.Domain() &&
           cookie1.Path() == cookie2.Path();
  }
};

typedef base::hash_set<net::CanonicalCookie,
                       CanonicalCookieHasher,
                       CanonicalCookieComparer> CookieHashSet;

}  // namespace canonical_cookie

#endif  // CHROME_BROWSER_BROWSING_DATA_CANONICAL_COOKIE_HASH_H_

// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_TIMED_CACHE_H_
#define CHROME_BROWSER_NET_TIMED_CACHE_H_

#include "base/containers/mru_cache.h"
#include "base/macros.h"
#include "base/time/time.h"

class GURL;

namespace chrome_browser_net {

// Define a LRU cache that recalls all navigations within the last N seconds.
// When we learn about subresources to possibly preconnect to, it would be a
// waste to preconnect when the original navigation was too long ago.  Any
// connected, but unused TCP/IP connection, will generally be reset by the
// server if it is not used quickly (i.e., GET or POST is sent).
class TimedCache {
 public:
  explicit TimedCache(const base::TimeDelta& max_duration);
  ~TimedCache();

  // Evicts any entries that have been in the FIFO "too long," and then checks
  // to see if the given url is (still) in the FIFO cache.
  bool WasRecentlySeen(const GURL& url);

  // Adds the given url to the cache, where it will remain for max_duration_.
  void SetRecentlySeen(const GURL& url);

 private:
  // Our cache will be keyed on a URL (actually, just a scheme/host/port).
  // We will always track the time it was last added to the FIFO cache by
  // remembering a TimeTicks value.
  typedef base::MRUCache<GURL, base::TimeTicks> UrlMruTimedCache;
  UrlMruTimedCache mru_cache_;

  // The longest time an entry can persist in the cache, and still be found.
  const base::TimeDelta max_duration_;

  DISALLOW_COPY_AND_ASSIGN(TimedCache);
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_TIMED_CACHE_H_

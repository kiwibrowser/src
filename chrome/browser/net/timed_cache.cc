// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/timed_cache.h"

#include "url/gurl.h"

namespace chrome_browser_net {

TimedCache::TimedCache(const base::TimeDelta& max_duration)
    : mru_cache_(UrlMruTimedCache::NO_AUTO_EVICT),
      max_duration_(max_duration) {
}

// Make Clang compilation happy with explicit destructor.
TimedCache::~TimedCache() {}

bool TimedCache::WasRecentlySeen(const GURL& url) {
  DCHECK_EQ(url.GetWithEmptyPath(), url);
  // Evict any overly old entries.
  base::TimeTicks now = base::TimeTicks::Now();
  UrlMruTimedCache::reverse_iterator eldest = mru_cache_.rbegin();
  while (!mru_cache_.empty()) {
    DCHECK(eldest == mru_cache_.rbegin());
    if (now - eldest->second < max_duration_)
      break;
    eldest = mru_cache_.Erase(eldest);
  }
  return mru_cache_.end() != mru_cache_.Peek(url);
}

void TimedCache::SetRecentlySeen(const GURL& url) {
  DCHECK_EQ(url.GetWithEmptyPath(), url);
  mru_cache_.Put(url, base::TimeTicks::Now());
}

}  // namespace chrome_browser_net

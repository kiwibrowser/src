// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_BROWSING_DATA_CACHE_COUNTER_H_
#define IOS_CHROME_BROWSER_BROWSING_DATA_CACHE_COUNTER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/browsing_data/core/counters/browsing_data_counter.h"

namespace ios {
class ChromeBrowserState;
}

// CacheCounter is a BrowsingDataCounter used to compute the cache size.
class CacheCounter : public browsing_data::BrowsingDataCounter {
 public:
  explicit CacheCounter(ios::ChromeBrowserState* browser_state);
  ~CacheCounter() override;

  // browsing_data::BrowsingDataCounter implementation.
  const char* GetPrefName() const override;
  void Count() override;

 private:
  // Invoked when cache size has been computed.
  void OnCacheSizeCalculated(int cache_size);

  ios::ChromeBrowserState* browser_state_;

  base::WeakPtrFactory<CacheCounter> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheCounter);
};

#endif  // IOS_CHROME_BROWSER_BROWSING_DATA_CACHE_COUNTER_H_

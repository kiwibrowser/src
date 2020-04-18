// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NTP_SNIPPETS_REMOTE_PREFETCHED_PAGES_TRACKER_H_
#define COMPONENTS_NTP_SNIPPETS_REMOTE_PREFETCHED_PAGES_TRACKER_H_

#include <string>

#include "base/callback.h"
#include "url/gurl.h"

namespace ntp_snippets {

// Synchronously answers whether there is a prefetched offline page for a given
// URL.
class PrefetchedPagesTracker {
 public:
  virtual ~PrefetchedPagesTracker() = default;

  // Whether the tracker has finished initialization.
  virtual bool IsInitialized() const = 0;

  // Starts asynchronous initialization. Callback will be called when the
  // initialization is completed. If the tracker has been initialized already,
  // the callback is called immediately.
  virtual void Initialize(base::OnceCallback<void()> callback) = 0;

  virtual bool PrefetchedOfflinePageExists(const GURL& url) const = 0;
};

}  // namespace ntp_snippets

#endif  // COMPONENTS_NTP_SNIPPETS_REMOTE_PREFETCHED_PAGES_TRACKER_H_

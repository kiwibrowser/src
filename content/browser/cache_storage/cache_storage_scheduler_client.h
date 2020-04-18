// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_SCHEDULER_CLIENT_H_
#define CONTENT_BROWSER_CACHE_STORAGE_SCHEDULER_CLIENT_H_

namespace content {

enum class CacheStorageSchedulerClient {
  CLIENT_STORAGE = 0,
  CLIENT_CACHE = 1,
  CLIENT_BACKGROUND_SYNC = 2
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_SCHEDULER_CLIENT_H_

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/mock_appcache_service.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

namespace content {

static void DeferredCallCallback(
    const net::CompletionCallback& callback, int rv) {
  callback.Run(rv);
}

void MockAppCacheService::DeleteAppCachesForOrigin(
    const url::Origin& origin,
    const net::CompletionCallback& callback) {
  ++delete_called_count_;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&DeferredCallCallback, callback,
                                mock_delete_appcaches_for_origin_result_));
}

}  // namespace content

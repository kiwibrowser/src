// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/test/test_helper.h"

#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"

namespace media_router {

#if !defined(OS_ANDROID)
TestMediaSinkService::TestMediaSinkService()
    : TestMediaSinkService(base::DoNothing()) {}

TestMediaSinkService::TestMediaSinkService(
    const OnSinksDiscoveredCallback& callback)
    : MediaSinkServiceBase(callback),
      timer_(new base::MockTimer(true /*retain_user_task*/,
                                 false /*is_repeating*/)) {
  SetTimerForTest(base::WrapUnique(timer_));
}

TestMediaSinkService::~TestMediaSinkService() = default;
#endif  // !defined(OS_ANDROID)

}  // namespace media_router

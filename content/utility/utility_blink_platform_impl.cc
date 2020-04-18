// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/utility/utility_blink_platform_impl.h"

#include "third_party/blink/public/platform/scheduler/child/webthread_base.h"

namespace content {

UtilityBlinkPlatformImpl::UtilityBlinkPlatformImpl()
    : main_thread_(blink::scheduler::WebThreadBase::InitializeUtilityThread()) {
}

UtilityBlinkPlatformImpl::~UtilityBlinkPlatformImpl() {
}

blink::WebThread* UtilityBlinkPlatformImpl::CurrentThread() {
  if (main_thread_->IsCurrentThread())
    return main_thread_.get();
  return BlinkPlatformImpl::CurrentThread();
}

}  // namespace content

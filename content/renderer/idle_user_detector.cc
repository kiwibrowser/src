// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/idle_user_detector.h"

#include "base/logging.h"
#include "content/common/input_messages.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

IdleUserDetector::IdleUserDetector() = default;

IdleUserDetector::~IdleUserDetector() = default;

void IdleUserDetector::ActivityDetected() {
  if (GetContentClient()->renderer()->RunIdleHandlerWhenWidgetsHidden()) {
    RenderThreadImpl* render_thread = RenderThreadImpl::current();
    if (render_thread != nullptr) {
      render_thread->PostponeIdleNotification();
    }
  }
}

}  // namespace content

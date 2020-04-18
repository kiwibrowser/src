// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/renderer/render_thread.h"

#include "base/lazy_instance.h"
#include "base/threading/thread_local.h"

namespace content {

// Keep the global RenderThread in a TLS slot so it is impossible to access
// incorrectly from the wrong thread.
static base::LazyInstance<
    base::ThreadLocalPointer<RenderThread>>::DestructorAtExit lazy_tls =
    LAZY_INSTANCE_INITIALIZER;

RenderThread* RenderThread::Get() {
  return lazy_tls.Pointer()->Get();
}

RenderThread::RenderThread() {
  lazy_tls.Pointer()->Set(this);
}

RenderThread::~RenderThread() {
  lazy_tls.Pointer()->Set(nullptr);
}

}  // namespace content

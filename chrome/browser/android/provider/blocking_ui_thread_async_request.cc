// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/provider/blocking_ui_thread_async_request.h"

BlockingUIThreadAsyncRequest::BlockingUIThreadAsyncRequest()
    : request_completed_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                         base::WaitableEvent::InitialState::NOT_SIGNALED) {}

void BlockingUIThreadAsyncRequest::RequestCompleted() {
  // Currently all our use cases receive their request response in the UI
  // thread (the same thread that made the request). However this is not
  // a design constraint and can be changed if ever needed.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  request_completed_.Signal();
}

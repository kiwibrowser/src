// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_PROVIDER_BLOCKING_UI_THREAD_ASYNC_REQUEST_H_
#define CHROME_BROWSER_ANDROID_PROVIDER_BLOCKING_UI_THREAD_ASYNC_REQUEST_H_

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/synchronization/waitable_event.h"
#include "chrome/browser/android/provider/run_on_ui_thread_blocking.h"
#include "content/public/browser/browser_thread.h"

// Allows making async requests and blocking the current thread until the
// response arrives. The request is performed in the UI thread.
// Users of this class MUST call RequestCompleted when receiving the response.
// Cannot be called directly from the UI thread.
class BlockingUIThreadAsyncRequest {
 public:
  BlockingUIThreadAsyncRequest();

  // Runs an asynchronous request, blocking the invoking thread until a response
  // is received. Class users MUST call RequestCompleted when this happens.
  // Make sure that the response is also delivered to the UI thread.
  // The request argument can be defined using base::Bind.
  template <typename Signature>
  void RunAsyncRequestOnUIThreadBlocking(base::Callback<Signature> request) {
    DCHECK(!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

    // Make the request in the UI thread.
    request_completed_.Reset();
    RunOnUIThreadBlocking::Run(
        base::Bind(
            &BlockingUIThreadAsyncRequest::RunRequestOnUIThread<Signature>,
            request));

    // Wait until the request callback invokes Finished.
    request_completed_.Wait();
  }

  // Notifies about a finished request.
  void RequestCompleted();

 private:
  template <typename Signature>
  static void RunRequestOnUIThread(base::Callback<Signature> request) {
    request.Run();
  }

  base::WaitableEvent request_completed_;

  DISALLOW_COPY_AND_ASSIGN(BlockingUIThreadAsyncRequest);
};

#endif  // CHROME_BROWSER_ANDROID_PROVIDER_BLOCKING_UI_THREAD_ASYNC_REQUEST_H_

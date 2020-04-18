// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_H_
#define IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_H_

#include <memory>

#include "base/macros.h"
#include "ios/web/public/web_thread.h"

namespace base {
class MessageLoop;
}

namespace web {

class TestWebThreadImpl;

// DEPRECATED: use TestWebThreadBundle instead.
// A WebThread for unit tests; this lets unit tests outside of web create
// WebThread instances.
class TestWebThread {
 public:
  explicit TestWebThread(WebThread::ID identifier);
  TestWebThread(WebThread::ID identifier, base::MessageLoop* message_loop);

  ~TestWebThread();

  // Provides a subset of the capabilities of the Thread interface to enable
  // certain unit tests. To avoid a stronger dependency of the internals of
  // WebThread, do no provide the full Thread interface.

  // Starts the thread with a generic message loop.
  bool Start();

  // Starts the thread with an IOThread message loop.
  bool StartIOThread();

  // Stops the thread.
  void Stop();

  // Returns true if the thread is running.
  bool IsRunning();

 private:
  std::unique_ptr<TestWebThreadImpl> impl_;

  DISALLOW_COPY_AND_ASSIGN(TestWebThread);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_H_

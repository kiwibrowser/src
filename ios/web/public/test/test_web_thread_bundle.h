// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_BUNDLE_H_
#define IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_BUNDLE_H_

// TestWebThreadBundle is a convenience class for creating a set of
// TestWebThreads and a task scheduler in unit tests. For most tests, it is
// sufficient to just instantiate the TestWebThreadBundle as a member variable.
// It is a good idea to put the TestWebThreadBundle as the first member variable
// in test classes, so it is destroyed last, and the test threads always exist
// from the perspective of other classes.
//
// By default, all of the created TestWebThreads will be backed by a single
// shared MessageLoop. If a test truly needs separate threads, it can do so by
// passing the appropriate combination of option values during the
// TestWebThreadBundle construction.
//
// To synchronously run tasks posted to TestWebThreads that use the shared
// MessageLoop, call RunLoop::Run/RunUntilIdle() on the thread where the
// TestWebThreadBundle lives. The destructor of TestWebThreadBundle runs
// remaining TestWebThreads tasks and remaining BLOCK_SHUTDOWN task scheduler
// tasks.
//
// Some tests using the IO thread expect a MessageLoopForIO. Passing
// IO_MAINLOOP will use a MessageLoopForIO for the main MessageLoop.
// Most of the time, this avoids needing to use a REAL_IO_THREAD.

#include <memory>

#include "base/macros.h"

namespace base {
namespace test {
class ScopedTaskEnvironment;
}  // namespace test
}  // namespace base

namespace web {

class TestWebThread;

class TestWebThreadBundle {
 public:
  // Used to specify the type of MessageLoop that backs the UI thread, and
  // which of the named WebThreads should be backed by a real
  // threads. The UI thread is always the main thread in a unit test.
  enum Options {
    DEFAULT = 0,
    IO_MAINLOOP = 1 << 0,
    REAL_IO_THREAD = 1 << 1,
  };

  TestWebThreadBundle();
  explicit TestWebThreadBundle(int options);

  ~TestWebThreadBundle();

 private:
  void Init(int options);

  std::unique_ptr<base::test::ScopedTaskEnvironment> scoped_task_environment_;
  std::unique_ptr<TestWebThread> ui_thread_;
  std::unique_ptr<TestWebThread> io_thread_;

  DISALLOW_COPY_AND_ASSIGN(TestWebThreadBundle);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_TEST_WEB_THREAD_BUNDLE_H_

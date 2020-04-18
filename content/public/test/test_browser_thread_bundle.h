// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TestBrowserThreadBundle is a convenience class which allows usage of these
// APIs within its scope:
// - Same APIs as base::test::ScopedTaskEnvironment.
// - content::BrowserThread.
//
// Only tests that need the BrowserThread API should instantiate a
// TestBrowserThreadBundle. Use base::test::ScopedTaskEnvironment otherwise.
//
// By default, BrowserThread::UI/IO are backed by a single shared MessageLoop on
// the main thread. If a test truly needs BrowserThread::IO tasks to run on a
// separate thread, it can pass the REAL_IO_THREAD option to the constructor.
// TaskScheduler tasks always run on dedicated threads.
//
// To synchronously run tasks from the shared MessageLoop:
//
// ... until there are no undelayed tasks in the shared MessageLoop:
//    base::RunLoop::RunUntilIdle();
//
// ... until there are no undelayed tasks in the shared MessageLoop, in
// TaskScheduler (excluding tasks not posted from the shared MessageLoop's
// thread or TaskScheduler):
//    content::RunAllTasksUntilIdle();
//
// ... until a condition is met:
//    base::RunLoop run_loop;
//    // Runs until a task running in the shared MessageLoop calls
//    // run_loop.Quit() or runs run_loop.QuitClosure() (&run_loop or
//    // run_loop.QuitClosure() must be kept somewhere accessible by that task).
//    run_loop.Run();
//
// To wait until there are no pending undelayed tasks in TaskScheduler, without
// running tasks from the shared MessageLoop:
//    base::TaskScheduler::GetInstance()->FlushForTesting();
//
// The destructor of TestBrowserThreadBundle runs remaining UI/IO tasks and
// remaining task scheduler tasks.
//
// If a test needs to pump IO messages on the main thread, it should use the
// IO_MAINLOOP option. Most of the time, IO_MAINLOOP avoids needing to use a
// REAL_IO_THREAD.
//
// For some tests it is important to emulate real browser startup. During real
// browser startup, the main MessageLoop and the TaskScheduler are created
// before browser threads. Passing DONT_CREATE_BROWSER_THREADS to constructor
// will delay creating browser threads until the test explicitly calls
// CreateBrowserThreads().
//
// DONT_CREATE_BROWSER_THREADS should only be used when the options specify at
// least one real thread other than the main thread.
//
// TestBrowserThreadBundle may be instantiated in a scope where there is already
// a base::test::ScopedTaskEnvironment. In that case, it will use the
// MessageLoop and the TaskScheduler provided by this
// base::test::ScopedTaskEnvironment instead of creating its own. The ability to
// have a base::test::ScopedTaskEnvironment and a TestBrowserThreadBundle in the
// same scope is useful when a fixture that inherits from a fixture that
// provides a base::test::ScopedTaskEnvironment needs to add support for browser
// threads.
//
// Basic usage:
//
//   class MyTestFixture : public testing::Test {
//    public:
//     (...)
//
//    protected:
//     // Must be the first member (or at least before any member that cares
//     // about tasks) to be initialized first and destroyed last. protected
//     // instead of private visibility will allow controlling the task
//     // environment (e.g. clock) once such features are added (see
//     // base::test::ScopedTaskEnvironment for details), until then it at least
//     // doesn't hurt :).
//     content::TestBrowserThreadBundle test_browser_thread_bundle_;
//
//     // Other members go here (or further below in private section.)
//   };

#ifndef CONTENT_PUBLIC_TEST_TEST_BROWSER_THREAD_BUNDLE_H_
#define CONTENT_PUBLIC_TEST_TEST_BROWSER_THREAD_BUNDLE_H_

#include <memory>

#include "base/macros.h"
#include "build/build_config.h"

namespace base {
namespace test {
class ScopedTaskEnvironment;
}  // namespace test
#if defined(OS_WIN)
namespace win {
class ScopedCOMInitializer;
}  // namespace win
#endif
}  // namespace base

namespace content {

class TestBrowserThread;

// Note: to drive these threads (e.g. run all tasks until idle), see
// content/public/test/test_utils.h.
class TestBrowserThreadBundle {
 public:
  // Used to specify the type of MessageLoop that backs the UI thread, and
  // which of the named BrowserThreads should be backed by a real
  // threads. The UI thread is always the main thread in a unit test.
  enum Options {
    DEFAULT = 0,
    // The main thread will use a MessageLoopForIO (and support the
    // base::FileDescriptorWatcher API on POSIX).
    IO_MAINLOOP = 1 << 0,
    REAL_IO_THREAD = 1 << 1,
    DONT_CREATE_BROWSER_THREADS = 1 << 2,
  };

  TestBrowserThreadBundle();
  explicit TestBrowserThreadBundle(int options);

  // Creates browser threads; should only be called from other classes if the
  // DONT_CREATE_BROWSER_THREADS option was used when the bundle was created.
  void CreateBrowserThreads();

  // Runs all tasks posted to TaskScheduler and main thread until idle.
  // Note: At the moment, this will not process BrowserThread::IO if this
  // TestBrowserThreadBundle is using a REAL_IO_THREAD.
  // TODO(robliao): fix this by making TaskScheduler aware of all MessageLoops.
  //
  // Note that this is not the cleanest way to run until idle as it will return
  // early if it depends on an async condition that isn't guaranteed to have
  // occurred yet. The best way to run until a condition is met is with RunLoop:
  //
  //   void KickoffAsyncFoo(base::OnceClosure on_done);
  //
  //   base::RunLoop run_loop;
  //   KickoffAsyncFoo(run_loop.QuitClosure());
  //   run_loop.Run();
  //
  void RunUntilIdle();

  // Flush the IO thread. Replacement for RunLoop::RunUntilIdle() for tests that
  // have a REAL_IO_THREAD. As with RunUntilIdle() above, prefer using
  // RunLoop+QuitClosure() to await an async condition.
  void RunIOThreadUntilIdle();

  ~TestBrowserThreadBundle();

 private:
  void Init();

  std::unique_ptr<base::test::ScopedTaskEnvironment> scoped_task_environment_;
  std::unique_ptr<TestBrowserThread> ui_thread_;
  std::unique_ptr<TestBrowserThread> io_thread_;

  int options_;
  bool threads_created_;

#if defined(OS_WIN)
  std::unique_ptr<base::win::ScopedCOMInitializer> com_initializer_;
#endif

  DISALLOW_COPY_AND_ASSIGN(TestBrowserThreadBundle);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEST_BROWSER_THREAD_BUNDLE_H_

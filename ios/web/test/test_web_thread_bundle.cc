// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/test/test_web_thread_bundle.h"

#include <memory>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "ios/web/public/test/test_web_thread.h"
#include "ios/web/web_thread_impl.h"
#include "net/disk_cache/simple/simple_backend_impl.h"

namespace web {

TestWebThreadBundle::TestWebThreadBundle() {
  Init(TestWebThreadBundle::DEFAULT);
}

TestWebThreadBundle::TestWebThreadBundle(int options) {
  Init(options);
}

TestWebThreadBundle::~TestWebThreadBundle() {
  // To avoid memory leaks, ensure that any tasks posted to the cache pool via
  // PostTaskAndReply are able to reply back to the originating thread, by
  // flushing the cache pool while the browser threads still exist.
  base::RunLoop().RunUntilIdle();
  disk_cache::SimpleBackendImpl::FlushWorkerPoolForTesting();

  // To ensure a clean teardown, each thread's message loop must be flushed
  // just before the thread is destroyed. But destroying a fake thread does not
  // automatically flush the message loop, so do it manually.
  // See http://crbug.com/247525 for discussion.
  base::RunLoop().RunUntilIdle();
  io_thread_.reset();
  base::RunLoop().RunUntilIdle();
  // This is the point at which the cache pool is normally shut down. So flush
  // it again in case any shutdown tasks have been posted to the pool from the
  // threads above.
  disk_cache::SimpleBackendImpl::FlushWorkerPoolForTesting();
  base::RunLoop().RunUntilIdle();
  ui_thread_.reset();
  base::RunLoop().RunUntilIdle();

  scoped_task_environment_.reset();
}

void TestWebThreadBundle::Init(int options) {
  scoped_task_environment_ =
      std::make_unique<base::test::ScopedTaskEnvironment>(
          options & TestWebThreadBundle::IO_MAINLOOP
              ? base::test::ScopedTaskEnvironment::MainThreadType::IO
              : base::test::ScopedTaskEnvironment::MainThreadType::UI);

  ui_thread_.reset(
      new TestWebThread(WebThread::UI, base::MessageLoop::current()));

  if (options & TestWebThreadBundle::REAL_IO_THREAD) {
    io_thread_.reset(new TestWebThread(WebThread::IO));
    io_thread_->StartIOThread();
  } else {
    io_thread_.reset(
        new TestWebThread(WebThread::IO, base::MessageLoop::current()));
  }
}

}  // namespace web

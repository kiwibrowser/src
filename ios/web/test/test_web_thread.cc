// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/test/test_web_thread.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "ios/web/web_thread_impl.h"

namespace web {

class TestWebThreadImpl : public WebThreadImpl {
 public:
  TestWebThreadImpl(WebThread::ID identifier) : WebThreadImpl(identifier) {}

  TestWebThreadImpl(WebThread::ID identifier, base::MessageLoop* message_loop)
      : WebThreadImpl(identifier, message_loop) {}

  ~TestWebThreadImpl() override { Stop(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWebThreadImpl);
};

TestWebThread::TestWebThread(WebThread::ID identifier)
    : impl_(new TestWebThreadImpl(identifier)) {
}

TestWebThread::TestWebThread(WebThread::ID identifier,
                             base::MessageLoop* message_loop)
    : impl_(new TestWebThreadImpl(identifier, message_loop)) {
}

TestWebThread::~TestWebThread() {
  Stop();
}

bool TestWebThread::Start() {
  return impl_->Start();
}

bool TestWebThread::StartIOThread() {
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  return impl_->StartWithOptions(options);
}

void TestWebThread::Stop() {
  impl_->Stop();
}

bool TestWebThread::IsRunning() {
  return impl_->IsRunning();
}

}  // namespace web

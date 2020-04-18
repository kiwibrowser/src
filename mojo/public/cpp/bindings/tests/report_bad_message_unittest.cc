// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/tests/bindings_test_base.h"
#include "mojo/public/interfaces/bindings/tests/test_bad_messages.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

class TestBadMessagesImpl : public TestBadMessages {
 public:
  TestBadMessagesImpl() : binding_(this) {}
  ~TestBadMessagesImpl() override {}

  void BindImpl(TestBadMessagesRequest request) {
    binding_.Bind(std::move(request));
  }

  ReportBadMessageCallback& bad_message_callback() {
    return bad_message_callback_;
  }

 private:
  // TestBadMessages:
  void RejectEventually(const RejectEventuallyCallback& callback) override {
    bad_message_callback_ = GetBadMessageCallback();
    callback.Run();
  }

  void RequestResponse(const RequestResponseCallback& callback) override {
    callback.Run();
  }

  void RejectSync(const RejectSyncCallback& callback) override {
    callback.Run();
    ReportBadMessage("go away");
  }

  void RequestResponseSync(
      const RequestResponseSyncCallback& callback) override {
    callback.Run();
  }

  ReportBadMessageCallback bad_message_callback_;
  mojo::Binding<TestBadMessages> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestBadMessagesImpl);
};

class ReportBadMessageTest : public BindingsTestBase {
 public:
  ReportBadMessageTest() {}

  void SetUp() override {
    mojo::edk::SetDefaultProcessErrorCallback(
        base::Bind(&ReportBadMessageTest::OnProcessError,
                   base::Unretained(this)));

    impl_.BindImpl(MakeRequest(&proxy_));
  }

  void TearDown() override {
    mojo::edk::SetDefaultProcessErrorCallback(
        mojo::edk::ProcessErrorCallback());
  }

  TestBadMessages* proxy() { return proxy_.get(); }

  TestBadMessagesImpl* impl() { return &impl_; }

  void SetErrorHandler(const base::Closure& handler) {
    error_handler_ = handler;
  }

 private:
  void OnProcessError(const std::string& error) {
    if (!error_handler_.is_null())
      error_handler_.Run();
  }

  TestBadMessagesPtr proxy_;
  TestBadMessagesImpl impl_;
  base::Closure error_handler_;
};

TEST_P(ReportBadMessageTest, Request) {
  // Verify that basic immediate error reporting works.
  bool error = false;
  SetErrorHandler(base::Bind([] (bool* flag) { *flag = true; }, &error));
  EXPECT_TRUE(proxy()->RejectSync());
  EXPECT_TRUE(error);
}

TEST_P(ReportBadMessageTest, RequestAsync) {
  bool error = false;
  SetErrorHandler(base::Bind([] (bool* flag) { *flag = true; }, &error));

  // This should capture a bad message reporting callback in the impl.
  base::RunLoop loop;
  proxy()->RejectEventually(loop.QuitClosure());
  loop.Run();

  EXPECT_FALSE(error);

  // Now we can run the callback and it should trigger a bad message report.
  DCHECK(!impl()->bad_message_callback().is_null());
  std::move(impl()->bad_message_callback()).Run("bad!");
  EXPECT_TRUE(error);
}

TEST_P(ReportBadMessageTest, Response) {
  bool error = false;
  SetErrorHandler(base::Bind([] (bool* flag) { *flag = true; }, &error));

  base::RunLoop loop;
  proxy()->RequestResponse(
      base::Bind([] (const base::Closure& quit) {
        // Report a bad message inside the response callback. This should
        // trigger the error handler.
        ReportBadMessage("no way!");
        quit.Run();
      },
      loop.QuitClosure()));
  loop.Run();

  EXPECT_TRUE(error);
}

TEST_P(ReportBadMessageTest, ResponseAsync) {
  bool error = false;
  SetErrorHandler(base::Bind([] (bool* flag) { *flag = true; }, &error));

  ReportBadMessageCallback bad_message_callback;
  base::RunLoop loop;
  proxy()->RequestResponse(
      base::Bind([] (const base::Closure& quit,
                     ReportBadMessageCallback* callback) {
        // Capture the bad message callback inside the response callback.
        *callback = GetBadMessageCallback();
        quit.Run();
      },
      loop.QuitClosure(), &bad_message_callback));
  loop.Run();

  EXPECT_FALSE(error);

  // Invoking this callback should report a bad message and trigger the error
  // handler immediately.
  std::move(bad_message_callback)
      .Run("this message is bad and should feel bad");
  EXPECT_TRUE(error);
}

TEST_P(ReportBadMessageTest, ResponseSync) {
  bool error = false;
  SetErrorHandler(base::Bind([] (bool* flag) { *flag = true; }, &error));

  SyncMessageResponseContext context;
  proxy()->RequestResponseSync();

  EXPECT_FALSE(error);
  context.ReportBadMessage("i don't like this response");
  EXPECT_TRUE(error);
}

TEST_P(ReportBadMessageTest, ResponseSyncDeferred) {
  bool error = false;
  SetErrorHandler(base::Bind([] (bool* flag) { *flag = true; }, &error));

  ReportBadMessageCallback bad_message_callback;
  {
    SyncMessageResponseContext context;
    proxy()->RequestResponseSync();
    bad_message_callback = context.GetBadMessageCallback();
  }

  EXPECT_FALSE(error);
  std::move(bad_message_callback).Run("nope nope nope");
  EXPECT_TRUE(error);
}

INSTANTIATE_MOJO_BINDINGS_TEST_CASE_P(ReportBadMessageTest);

}  // namespace
}  // namespace test
}  // namespace mojo

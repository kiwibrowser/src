// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/tracing/child/child_trace_message_filter.h"

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "base/run_loop.h"
#include "components/tracing/common/tracing_messages.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_sender.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {

class FakeSender : public IPC::Sender {
 public:
  FakeSender() {}

  ~FakeSender() override {}

  bool Send(IPC::Message* msg) override {
    last_message_.reset(msg);
    return true;
  }

  std::unique_ptr<IPC::Message> last_message_;
};

class ChildTraceMessageFilterTest : public testing::Test {
 public:
  ChildTraceMessageFilterTest() {
    message_filter_ =
        new tracing::ChildTraceMessageFilter(message_loop_.task_runner().get());
    message_filter_->SetSenderForTesting(&fake_sender_);
  }

  void OnSetUMACallback(const std::string& histogram,
                        int low,
                        int high,
                        bool repeat) {
    fake_sender_.last_message_.reset();
    message_filter_->OnSetUMACallback(histogram, low, high, repeat);
  }

  base::MessageLoop message_loop_;
  FakeSender fake_sender_;
  scoped_refptr<tracing::ChildTraceMessageFilter> message_filter_;
};

TEST_F(ChildTraceMessageFilterTest, TestHistogramDoesNotTrigger) {
  LOCAL_HISTOGRAM_COUNTS("foo1", 10);

  OnSetUMACallback("foo1", 20000, 25000, true);

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(fake_sender_.last_message_);
}

TEST_F(ChildTraceMessageFilterTest, TestHistogramTriggers) {
  LOCAL_HISTOGRAM_COUNTS("foo2", 2);

  OnSetUMACallback("foo2", 1, 3, true);

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(fake_sender_.last_message_);
  EXPECT_EQ(fake_sender_.last_message_->type(),
            static_cast<uint32_t>(TracingHostMsg_TriggerBackgroundTrace::ID));
}

TEST_F(ChildTraceMessageFilterTest, TestHistogramAborts) {
  LOCAL_HISTOGRAM_COUNTS("foo3", 10);

  OnSetUMACallback("foo3", 1, 3, false);

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(fake_sender_.last_message_);
  EXPECT_EQ(fake_sender_.last_message_->type(),
            static_cast<uint32_t>(TracingHostMsg_AbortBackgroundTrace::ID));
}

}  // namespace tracing

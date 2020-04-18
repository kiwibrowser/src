// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "remoting/base/auto_thread_task_runner.h"
#include "remoting/host/chromoting_host_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

// A simple test that starts and stop the context. This tests the context
// operates properly and all threads and message loops are valid.
TEST(ChromotingHostContextTest, StartAndStop) {
  base::MessageLoopForUI message_loop;
  base::RunLoop run_loop;

  std::unique_ptr<ChromotingHostContext> context =
      ChromotingHostContext::Create(new AutoThreadTaskRunner(
          message_loop.task_runner(), run_loop.QuitClosure()));

  EXPECT_TRUE(context);
  if (!context)
    return;
  EXPECT_TRUE(context->audio_task_runner().get());
  EXPECT_TRUE(context->video_capture_task_runner().get());
  EXPECT_TRUE(context->video_encode_task_runner().get());
  EXPECT_TRUE(context->file_task_runner().get());
  EXPECT_TRUE(context->input_task_runner().get());
  EXPECT_TRUE(context->network_task_runner().get());
  EXPECT_TRUE(context->ui_task_runner().get());

  context.reset();
  run_loop.Run();
}

}  // namespace remoting

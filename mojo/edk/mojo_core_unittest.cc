// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/system/core.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(MojoCoreTest, SanityCheck) {
  // Exercises some APIs against the mojo_core library and expects them to work
  // as intended.

  MojoHandle a, b;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessagePipe(nullptr, &a, &b));

  MojoMessageHandle m;
  EXPECT_EQ(MOJO_RESULT_OK, MojoCreateMessage(nullptr, &m));
  EXPECT_EQ(MOJO_RESULT_OK,
            MojoSetMessageContext(m, 42, nullptr, nullptr, nullptr));
  EXPECT_EQ(MOJO_RESULT_OK, MojoWriteMessage(a, m, nullptr));
  m = MOJO_MESSAGE_HANDLE_INVALID;

  MojoHandleSignalsState state;
  EXPECT_EQ(MOJO_RESULT_OK, MojoQueryHandleSignalsState(b, &state));
  EXPECT_TRUE(state.satisfied_signals & MOJO_HANDLE_SIGNAL_READABLE);

  EXPECT_EQ(MOJO_RESULT_OK, MojoReadMessage(b, nullptr, &m));

  uintptr_t context = 0;
  EXPECT_EQ(MOJO_RESULT_OK, MojoGetMessageContext(m, nullptr, &context));
  EXPECT_EQ(42u, context);

  EXPECT_EQ(MOJO_RESULT_OK, MojoDestroyMessage(m));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(b));
  EXPECT_EQ(MOJO_RESULT_OK, MojoClose(a));
}

}  // namespace

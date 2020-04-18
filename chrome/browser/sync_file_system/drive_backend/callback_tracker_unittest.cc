// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync_file_system/drive_backend/callback_tracker.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace sync_file_system {
namespace drive_backend {

namespace {

void Receiver(bool* called) {
  DCHECK(called);
  EXPECT_FALSE(*called);
  *called = true;
}

}  // namespace

TEST(CallbackTrackerTest, AbortAll) {
  CallbackTracker tracker;

  bool aborted = false;
  bool invoked = false;
  base::Closure callback = tracker.Register(base::Bind(&Receiver, &aborted),
                                            base::Bind(&Receiver, &invoked));
  tracker.AbortAll();
  EXPECT_TRUE(aborted);
  EXPECT_FALSE(invoked);

  callback.Run();
  EXPECT_TRUE(aborted);
  EXPECT_FALSE(invoked);
}

TEST(CallbackTrackerTest, Invoke) {
  CallbackTracker tracker;

  bool aborted = false;
  bool invoked = false;
  base::Closure callback = tracker.Register(base::Bind(&Receiver, &aborted),
                                            base::Bind(&Receiver, &invoked));
  callback.Run();
  EXPECT_FALSE(aborted);
  EXPECT_TRUE(invoked);

  // Second call should not do anything.
  invoked = false;
  callback.Run();
  EXPECT_FALSE(invoked);

  tracker.AbortAll();
  EXPECT_FALSE(aborted);
}

}  // namespace drive_backend
}  // namespace sync_file_system

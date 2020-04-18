// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/driver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace binder {

TEST(BinderDriverTest, Initialize) {
  Driver driver;
  EXPECT_TRUE(driver.Initialize());
}

}  // namespace binder

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/source_file.h"

// The SourceFile object should normalize the input passed to the constructor.
// The normalizer unit test checks for all the weird edge cases for normalizing
// so here just check that it gets called.
TEST(SourceFile, Normalize) {
  SourceFile a("//foo/../bar.cc");
  EXPECT_EQ("//bar.cc", a.value());

  std::string b_str("//foo/././../bar.cc");
  SourceFile b(SourceFile::SwapIn(), &b_str);
  EXPECT_TRUE(b_str.empty());  // Should have been swapped in.
  EXPECT_EQ("//bar.cc", b.value());
}

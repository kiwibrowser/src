// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/mac/bind_objc_block.h"

#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

TEST(BindObjcBlockTestARC, TestScopedClosureRunnerExitScope) {
  int run_count = 0;
  int* ptr = &run_count;
  {
    base::ScopedClosureRunner runner(base::BindBlockArc(^{
      (*ptr)++;
    }));
    EXPECT_EQ(0, run_count);
  }
  EXPECT_EQ(1, run_count);
}

TEST(BindObjcBlockTestARC, TestScopedClosureRunnerRelease) {
  int run_count = 0;
  int* ptr = &run_count;
  base::OnceClosure c;
  {
    base::ScopedClosureRunner runner(base::BindBlockArc(^{
      (*ptr)++;
    }));
    c = runner.Release();
    EXPECT_EQ(0, run_count);
  }
  EXPECT_EQ(0, run_count);
  std::move(c).Run();
  EXPECT_EQ(1, run_count);
}

TEST(BindObjcBlockTestARC, TestReturnValue) {
  const int kReturnValue = 42;
  base::Callback<int(void)> c = base::BindBlockArc(^{
    return kReturnValue;
  });
  EXPECT_EQ(kReturnValue, c.Run());
}

TEST(BindObjcBlockTestARC, TestArgument) {
  const int kArgument = 42;
  base::Callback<int(int)> c = base::BindBlockArc(^(int a) {
    return a + 1;
  });
  EXPECT_EQ(kArgument + 1, c.Run(kArgument));
}

TEST(BindObjcBlockTestARC, TestTwoArguments) {
  std::string result;
  std::string* ptr = &result;
  base::Callback<void(const std::string&, const std::string&)> c =
      base::BindBlockArc(^(const std::string& a, const std::string& b) {
        *ptr = a + b;
      });
  c.Run("forty", "two");
  EXPECT_EQ(result, "fortytwo");
}

TEST(BindObjcBlockTestARC, TestThreeArguments) {
  std::string result;
  std::string* ptr = &result;
  base::Callback<void(const std::string&, const std::string&,
                      const std::string&)>
      c = base::BindBlockArc(
          ^(const std::string& a, const std::string& b, const std::string& c) {
            *ptr = a + b + c;
          });
  c.Run("six", "times", "nine");
  EXPECT_EQ(result, "sixtimesnine");
}

TEST(BindObjcBlockTestARC, TestSixArguments) {
  std::string result1;
  std::string* ptr = &result1;
  int result2;
  int* ptr2 = &result2;
  base::Callback<void(int, int, const std::string&, const std::string&, int,
                      const std::string&)>
      c = base::BindBlockArc(^(int a, int b, const std::string& c,
                               const std::string& d, int e,
                               const std::string& f) {
        *ptr = c + d + f;
        *ptr2 = a + b + e;
      });
  c.Run(1, 2, "infinite", "improbability", 3, "drive");
  EXPECT_EQ(result1, "infiniteimprobabilitydrive");
  EXPECT_EQ(result2, 6);
}

#if defined(OS_IOS)

TEST(BindObjcBlockTestARC, TestBlockReleased) {
  __weak NSObject* weak_nsobject;
  @autoreleasepool {
    NSObject* nsobject = [[NSObject alloc] init];
    weak_nsobject = nsobject;

    auto callback = base::BindBlockArc(^{
      [nsobject description];
    });
  }
  EXPECT_NSEQ(nil, weak_nsobject);
}

#endif

}  // namespace

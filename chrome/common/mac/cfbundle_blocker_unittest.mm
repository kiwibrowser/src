// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/mac/cfbundle_blocker.h"
#include "chrome/common/mac/cfbundle_blocker_private.h"

#import <Foundation/Foundation.h>
#include <stddef.h>

#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/mach_override/mach_override.h"

namespace chrome {
namespace common {
namespace mac {
namespace {

struct IsBundleAllowedTestcase {
  NSString* bundle_id;
  NSString* version;
  bool allowed;
};

TEST(CFBundleBlockerTest, IsBundleAllowed) {
  const IsBundleAllowedTestcase kTestcases[] = {
    // Block things without a bundle ID.
    { nil, nil, false },

    // Block bundle IDs that aren't in the whitelist.
    { @"org.chromium.Chromium.evil", nil, false },

    // The AllowedBundle structure for Google Authetnicator BT doesn't
    // require a version, so this should work equally well with any version
    // including no version at all.
    { @"com.google.osax.Google_Authenticator_BT", nil, true },
    { @"com.google.osax.Google_Authenticator_BT", @"0.5.0.0", true },

    // Typos should be blocked.
    { @"com.google.osax.Google_Authenticator_B", nil, false },
    { @"com.google.osax.Google_Authenticator_BQ", nil, false },
    { @"com.google.osax.Google_Authenticator_BTQ", nil, false },
    { @"com.google.osax", nil, false },
    { @"com.google", nil, false },
    { @"com", nil, false },
    { @"", nil, false },

    // MySpeed requires a version, so make sure that versions below don't work
    // and versions above do.
    { @"com.enounce.MySpeed.osax", nil, false },
    { @"com.enounce.MySpeed.osax", @"", false },
    { @"com.enounce.MySpeed.osax", @"1200", false },
    { @"com.enounce.MySpeed.osax", @"1201", true },
    { @"com.enounce.MySpeed.osax", @"1202", true },

    // DefaultFolderX is whitelisted as com.stclairsoft.DefaultFolderX. Make
    // sure that "child" IDs such as com.stclairsoft.DefaultFolderX.osax work.
    // It uses a dotted versioning scheme, so test the version comparator out.
    { @"com.stclairsoft.DefaultFolderX.osax", nil, false },
    { @"com.stclairsoft.DefaultFolderX.osax", @"", false },
    { @"com.stclairsoft.DefaultFolderX.osax", @"3.5.4", false },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.3.4", false },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.4.2", false },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.4.3", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.4.4", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.4.10", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.5", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.5.2", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.10", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"4.10.2", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"5", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"5.3", true },
    { @"com.stclairsoft.DefaultFolderX.osax", @"5.3.2", true },

    // Other "child" IDs that might want to load.
    { @"com.stclairsoft.DefaultFolderX.CarbonPatcher", @"4.4.3", true },
    { @"com.stclairsoft.DefaultFolderX.CocoaPatcher", @"4.4.3", true },
  };

  for (size_t index = 0; index < arraysize(kTestcases); ++index) {
    const IsBundleAllowedTestcase& testcase = kTestcases[index];
    NSString* bundle_id = testcase.bundle_id;
    NSString* version = testcase.version;
    NSString* version_print = version ? version : @"(nil)";
    EXPECT_EQ(testcase.allowed, IsBundleAllowed(bundle_id, version))
        << "index " << index
        << ", bundle_id " << [bundle_id UTF8String]
        << ", version " << [version_print UTF8String];
  }
}

TEST(CFBundleBlockerTest, EnableCFBundleBlocker_AllocationAttempts) {
  static uint64_t s_num_test_calls = 0;
  ++s_num_test_calls;
  if (g_original_underscore_cfbundle_load_executable_and_return_error) {
    // The override has already happened. Overriding twice may lead to a hang
    // so we need to restore it first.
    mach_error_t err = mach_override_ptr(
        reinterpret_cast<void*>(_CFBundleLoadExecutableAndReturnError),
        reinterpret_cast<void*>(
            g_original_underscore_cfbundle_load_executable_and_return_error),
        nullptr);
    ASSERT_EQ(err_none, err)
        << "Failed to restore CFBundleLoadExecutableAndReturnError";
  }
  uint64_t allocations_num_start = mach_override_ptr_allocation_attempts();
  EXPECT_TRUE(EnableCFBundleBlocker());
  // Note that each time mach_override_ptr is called to override the same
  // function address. Each will allocate a page that will be attempted
  // in the next call. So if this test is called in the same process multiple
  // times, mach_override_ptr will end up attempting to allocate the same
  // addresses over and over. Hence the 2 * s_num_test_calls added.
  ASSERT_LE(mach_override_ptr_allocation_attempts(),
            100UL + allocations_num_start + 2 * s_num_test_calls)
      << "Too many allocation attempts. "
         "See https://bugs.chromium.org/p/chromium/issues/detail?id=730918";
}

}  // namespace
}  // namespace mac
}  // namespace common
}  // namespace chrome

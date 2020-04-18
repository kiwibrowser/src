// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <launch.h>

#include "base/mac/foundation_util.h"
#include "base/mac/launchd.h"
#include "base/mac/scoped_nsobject.h"
#include "base/test/test_timeouts.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/google_toolbox_for_mac/src/Foundation/GTMServiceManagement.h"

namespace {

// Returns the parameters needed to launch a really simple script from launchd.
NSDictionary* TestJobDictionary(NSString* label_ns) {
  NSString* shell_script_ns = @"sleep 10; echo TestGTMSMJobSubmitRemove";
  base::scoped_nsobject<NSMutableArray> job_arguments(
      [[NSMutableArray alloc] init]);
  [job_arguments addObject:@"/bin/sh"];
  [job_arguments addObject:@"-c"];
  [job_arguments addObject:shell_script_ns];

  NSMutableDictionary* job_dictionary_ns = [NSMutableDictionary dictionary];
  [job_dictionary_ns setObject:label_ns forKey:@LAUNCH_JOBKEY_LABEL];
  [job_dictionary_ns setObject:[NSNumber numberWithBool:YES]
                        forKey:@LAUNCH_JOBKEY_RUNATLOAD];
  [job_dictionary_ns setObject:job_arguments
                        forKey:@LAUNCH_JOBKEY_PROGRAMARGUMENTS];
  return job_dictionary_ns;
};

}  // namespace

TEST(ServiceProcessControlMac, TestGTMSMJobSubmitRemove) {
  NSString* label_ns = @"com.chromium.ServiceProcessStateFileManipulationTest";
  std::string label(label_ns.UTF8String);
  CFStringRef label_cf = base::mac::NSToCFCast(label_ns);

  // If the job is loaded or running, remove it.
  pid_t pid = base::mac::PIDForJob(label);
  CFErrorRef error = NULL;
  if (pid >= 0)
    ASSERT_TRUE(GTMSMJobRemove(label_cf, &error));

  // The job should not be loaded or running.
  pid = base::mac::PIDForJob(label);
  EXPECT_LT(pid, 0);

  // Submit a new job.
  NSDictionary* job_dictionary_ns = TestJobDictionary(label_ns);
  CFDictionaryRef job_dictionary_cf = base::mac::NSToCFCast(job_dictionary_ns);
  ASSERT_TRUE(GTMSMJobSubmit(job_dictionary_cf, &error));

  // The new job should be running.
  pid = base::mac::PIDForJob(label);
  EXPECT_GT(pid, 0);

  // Remove the job.
  ASSERT_TRUE(GTMSMJobRemove(label_cf, &error));

  // Wait for the job to be killed.
  base::TimeDelta timeout_in_ms = TestTimeouts::action_timeout();
  base::Time start_time = base::Time::Now();
  while (1) {
    pid = base::mac::PIDForJob(label);
    if (pid < 0)
      break;

    base::Time current_time = base::Time::Now();
    if (current_time - start_time > timeout_in_ms)
      break;
  }

  EXPECT_LT(pid, 0);

  // Attempting to remove the job again should fail.
  EXPECT_FALSE(GTMSMJobRemove(label_cf, &error));
}

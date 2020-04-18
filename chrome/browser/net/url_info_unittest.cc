// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Single threaded tests of UrlInfo functionality.

#include <time.h>
#include <string>

#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "chrome/browser/net/url_info.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;
using base::TimeTicks;

namespace {

class UrlHostInfoTest : public testing::Test {
};

typedef chrome_browser_net::UrlInfo UrlInfo;

// Cycle throught the states held by a UrlInfo instance, and check to see that
// states look reasonable as time ticks away.  If the test bots are too slow,
// we'll just give up on this test and exit from it.
TEST(UrlHostInfoTest, StateChangeTest) {
  UrlInfo info_practice, info;
  GURL url1("http://domain1.com:80"), url2("https://domain2.com:443");

  // First load DLL, so that their load time won't interfere with tests.
  // Some tests involve timing function performance, and DLL time can overwhelm
  // test durations (which are considering network vs cache response times).
  info_practice.SetUrl(url2);
  info_practice.SetQueuedState(UrlInfo::UNIT_TEST_MOTIVATED);
  info_practice.SetAssignedState();
  info_practice.SetFoundState();

  // Start test with actual (long/default) expiration time intact.

  // Complete the construction of real test object.
  info.SetUrl(url1);
  EXPECT_TRUE(info.NeedsDnsUpdate()) << "error in construction state";
  info.SetQueuedState(UrlInfo::UNIT_TEST_MOTIVATED);
  EXPECT_FALSE(info.NeedsDnsUpdate()) << "update needed after being queued";
  info.SetAssignedState();
  EXPECT_FALSE(info.NeedsDnsUpdate())  << "update needed during resolution";
  base::TimeTicks before_resolution_complete = TimeTicks::Now();
  info.SetFoundState();
  // "Immediately" check to see if we need an update yet (we shouldn't).
  if (info.NeedsDnsUpdate()) {
    // The test bot must be really slow, so we can verify that.
    EXPECT_GT((TimeTicks::Now() - before_resolution_complete).InMilliseconds(),
              UrlInfo::get_cache_expiration().InMilliseconds());
    return;  // Lets punt here, the test bot is too slow.
  }

  // Run similar test with a shortened expiration, so we can trigger it.
  const TimeDelta kMockExpirationTime = TimeDelta::FromMilliseconds(300);
  info.set_cache_expiration(kMockExpirationTime);

  // That was a nice life when the object was found.... but next time it won't
  // be found.  We'll sleep for a while, and then come back with not-found.
  info.SetQueuedState(UrlInfo::UNIT_TEST_MOTIVATED);
  EXPECT_FALSE(info.NeedsDnsUpdate());
  info.SetAssignedState();
  EXPECT_FALSE(info.NeedsDnsUpdate());
  // Greater than minimal expected network latency on DNS lookup.
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(25));
  before_resolution_complete = TimeTicks::Now();
  info.SetNoSuchNameState();
  // "Immediately" check to see if we need an update yet (we shouldn't).
  if (info.NeedsDnsUpdate()) {
    // The test bot must be really slow, so we can verify that.
    EXPECT_GT((TimeTicks::Now() - before_resolution_complete),
              kMockExpirationTime);
    return;
  }
  // Wait over 300ms, so it should definately be considered out of cache.
  base::PlatformThread::Sleep(kMockExpirationTime +
                              TimeDelta::FromMilliseconds(20));
  EXPECT_TRUE(info.NeedsDnsUpdate()) << "expiration time not honored";
}

// When a system gets "congested" relative to DNS, it means it is doing too many
// DNS resolutions, and bogging down the system.  When we detect such a
// situation, we divert the sequence of states a UrlInfo instance moves
// through.  Rather than proceeding from QUEUED (waiting in a name queue for a
// worker thread that can resolve the name) to ASSIGNED (where a worker thread
// actively resolves the name), we enter the ASSIGNED state (without actually
// getting sent to a resolver thread) and reset our state to what it was before
// the corresponding name was put in the work_queue_.  This test drives through
// the state transitions used in such congestion handling.
TEST(UrlHostInfoTest, CongestionResetStateTest) {
  UrlInfo info;
  GURL url("http://domain1.com:80");

  info.SetUrl(url);
  info.SetQueuedState(UrlInfo::UNIT_TEST_MOTIVATED);
  info.SetAssignedState();
  EXPECT_TRUE(info.is_assigned());

  info.RemoveFromQueue();  // Do the reset.
  EXPECT_FALSE(info.is_assigned());

  // Since this was a new info instance, and it never got resolved, we land back
  // in a PENDING state rather than FOUND or NO_SUCH_NAME.
  EXPECT_FALSE(info.was_found());
  EXPECT_FALSE(info.was_nonexistent());

  // Make sure we're completely re-usable, by going throug a normal flow.
  info.SetQueuedState(UrlInfo::UNIT_TEST_MOTIVATED);
  info.SetAssignedState();
  info.SetFoundState();
  EXPECT_TRUE(info.was_found());

  // Use the congestion flow, and check that we end up in the found state.
  info.SetQueuedState(UrlInfo::UNIT_TEST_MOTIVATED);
  info.SetAssignedState();
  info.RemoveFromQueue();  // Do the reset.
  EXPECT_FALSE(info.is_assigned());
  EXPECT_TRUE(info.was_found());  // Back to what it was before being queued.
}


// TODO(jar): Add death test for illegal state changes, and also for setting
// hostname when already set.

}  // namespace

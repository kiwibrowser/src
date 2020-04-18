// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/net/network_quality_observer_impl.h"

#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/time/time.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/network_quality_estimator_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace {

TEST(NetworkQualityObserverImplTest, TestObserverNotified) {
  content::TestBrowserThreadBundle thread_bundle(
      content::TestBrowserThreadBundle::Options::IO_MAINLOOP);

  net::TestNetworkQualityEstimator estimator;
  estimator.SetStartTimeNullHttpRtt(base::TimeDelta::FromMilliseconds(1));

  NetworkQualityObserverImpl observer(&estimator);
  // Give a chance for |observer| to register with the |estimator|.
  base::RunLoop().RunUntilIdle();

  base::HistogramTester histogram_tester;
  estimator.SetStartTimeNullHttpRtt(base::TimeDelta::FromMilliseconds(500));
  // RTT changed from 1 msec to 500 msec.
  histogram_tester.ExpectBucketCount(
      "NQE.ContentObserver.NetworkQualityMeaningfullyChanged", 1, 1);

  estimator.SetStartTimeNullHttpRtt(base::TimeDelta::FromMilliseconds(625));
  // RTT changed from 500 msec to 625 msec.
  histogram_tester.ExpectBucketCount(
      "NQE.ContentObserver.NetworkQualityMeaningfullyChanged", 1, 2);

  estimator.SetStartTimeNullHttpRtt(base::TimeDelta::FromMilliseconds(626));
  // RTT changed from 625 msec to 626 msec which is not a meaningful change.
  histogram_tester.ExpectBucketCount(
      "NQE.ContentObserver.NetworkQualityMeaningfullyChanged", 1, 2);
  EXPECT_LE(1, histogram_tester.GetBucketCount(
                   "NQE.ContentObserver.NetworkQualityMeaningfullyChanged", 0));
}

}  // namespace
}  // namespace content

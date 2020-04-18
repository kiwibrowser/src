// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/logging.h"
#import "ios/chrome/browser/web/chrome_web_client.h"
#import "ios/chrome/test/base/perf_test_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

PerfTest::PerfTest(std::string testGroup)
    : BlockCleanupTest(),
      testGroup_(testGroup),
      firstLabel_("1st"),
      averageLabel_("2nd+"),
      isWaterfall_(false),
      verbose_(true),
      repeatCount_(10),
      web_client_(std::make_unique<ChromeWebClient>()) {}
PerfTest::PerfTest(std::string testGroup,
                   std::string firstLabel,
                   std::string averageLabel,
                   bool isWaterfall,
                   bool verbose,
                   int repeat)
    : BlockCleanupTest(),
      testGroup_(testGroup),
      firstLabel_(firstLabel),
      averageLabel_(averageLabel),
      isWaterfall_(isWaterfall),
      verbose_(verbose),
      repeatCount_(repeat),
      web_client_(std::make_unique<ChromeWebClient>()) {}

PerfTest::~PerfTest() {}

void PerfTest::LogPerfTiming(std::string testName, base::TimeDelta elapsed) {
  LogPerfValue(testName, elapsed.InMillisecondsF(), "ms");
}

void PerfTest::LogPerfValue(std::string testName,
                            double value,
                            std::string unit) {
  NSLog(@"%sRESULT %s: %s= %.3f %s\n", isWaterfall_ ? "*" : "",
        testGroup_.c_str(), testName.c_str(), value, unit.c_str());
}

void PerfTest::RepeatTimedRuns(std::string testName,
                               TimedActionBlock timedAction,
                               ProceduralBlock postAction) {
  RepeatTimedRuns(testName, timedAction, postAction, repeatCount_);
}

void PerfTest::RepeatTimedRuns(std::string testName,
                               TimedActionBlock timedAction,
                               ProceduralBlock postAction,
                               int repeat) {
  base::TimeDelta firstElapsed;
  base::TimeDelta totalElapsed;
  base::TimeDelta maxElapsed;
  base::TimeDelta minElapsed = base::TimeDelta::FromSeconds(1000);
  for (int i = 0; i < repeat + 1; ++i) {
    base::TimeDelta elapsed = timedAction(i);
    if (i == 0) {
      std::string label =
          firstLabel_.length() ? testName + " " + firstLabel_ : testName;
      LogPerfTiming(label, elapsed);
      firstElapsed = elapsed;
    } else {
      if (verbose_)
        NSLog(@"%2d: %.3f ms", i, elapsed.InMillisecondsF());
      totalElapsed += elapsed;
      if (elapsed > maxElapsed)
        maxElapsed = elapsed;
      if (elapsed < minElapsed)
        minElapsed = elapsed;
    }
    if (postAction)
      postAction();
  }
  if (repeat > 2) {
    base::TimeDelta average =
        (totalElapsed - maxElapsed - minElapsed) / (repeat - 2);
    std::string label =
        averageLabel_.length() ? testName + " " + averageLabel_ : testName;
    LogPerfTiming(label, average);
  }
}

// TODO(leng): Replace this with RepeatTimedRuns when we have figured out
// the best way to combine various logging requirements in repeated runs.
base::TimeDelta PerfTest::CalculateAverage(base::TimeDelta* times,
                                           int count,
                                           base::TimeDelta* min_time,
                                           base::TimeDelta* max_time) {
  DCHECK(times);
  base::TimeDelta min = times[0];
  base::TimeDelta max = times[0];
  base::TimeDelta sum = times[0];

  for (int i = 1; i < count; i++) {
    sum += times[i];
    if (times[i] > max)
      max = times[i];
    if (times[i] < min)
      min = times[i];
  }
  if (max_time)
    *max_time = max;
  if (min_time)
    *min_time = min;
  return sum / count;
}

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/chrome_browser_main_extra_parts_metrics.h"

#include <memory>

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_service_manager_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/screen.h"
#include "ui/display/test/test_screen.h"
#include "ui/gfx/geometry/size.h"

#if defined(USE_OZONE) || defined(USE_X11)
#include "services/ui/public/cpp/input_devices/input_device_client_test_api.h"
#endif

namespace {

const char kTouchEventFeatureDetectionEnabledHistogramName[] =
    "Touchscreen.TouchEventsEnabled";

}  // namespace

class ChromeBrowserMainExtraPartsMetricsTest : public testing::Test {
 public:
  ChromeBrowserMainExtraPartsMetricsTest();
  ~ChromeBrowserMainExtraPartsMetricsTest() override;

 protected:
#if defined(USE_OZONE) || defined(USE_X11)
  ui::InputDeviceClientTestApi input_device_client_test_api_;
#endif

 private:
  // Provides a message loop and allows the use of the task scheduler
  content::TestBrowserThreadBundle thread_bundle_;
  content::TestServiceManagerContext service_manager_context_;

  // Dummy screen required by a ChromeBrowserMainExtraPartsMetrics test target.
  display::test::TestScreen test_screen_;

  DISALLOW_COPY_AND_ASSIGN(ChromeBrowserMainExtraPartsMetricsTest);
};

ChromeBrowserMainExtraPartsMetricsTest::
    ChromeBrowserMainExtraPartsMetricsTest() {
  display::Screen::SetScreenInstance(&test_screen_);
}

ChromeBrowserMainExtraPartsMetricsTest::
    ~ChromeBrowserMainExtraPartsMetricsTest() {
  display::Screen::SetScreenInstance(nullptr);
}

// Verify a TouchEventsEnabled value isn't recorded during construction.
TEST_F(ChromeBrowserMainExtraPartsMetricsTest,
       VerifyTouchEventsEnabledIsNotRecordedAfterConstruction) {
  base::HistogramTester histogram_tester;
  ChromeBrowserMainExtraPartsMetrics test_target;
  histogram_tester.ExpectTotalCount(
      kTouchEventFeatureDetectionEnabledHistogramName, 0);
}

#if defined(USE_OZONE) || defined(USE_X11)

// Verify a TouchEventsEnabled value isn't recorded during PostBrowserStart if
// the device scan hasn't completed yet.
TEST_F(ChromeBrowserMainExtraPartsMetricsTest,
       VerifyTouchEventsEnabledIsNotRecordedAfterPostBrowserStart) {
  base::HistogramTester histogram_tester;

  ChromeBrowserMainExtraPartsMetrics test_target;

  test_target.PostBrowserStart();
  histogram_tester.ExpectTotalCount(
      kTouchEventFeatureDetectionEnabledHistogramName, 0);
}

// Verify a TouchEventsEnabled value is recorded during PostBrowserStart if the
// device scan has already completed.
TEST_F(ChromeBrowserMainExtraPartsMetricsTest,
       VerifyTouchEventsEnabledIsRecordedAfterPostBrowserStart) {
  base::HistogramTester histogram_tester;

  input_device_client_test_api_.OnDeviceListsComplete();

  ChromeBrowserMainExtraPartsMetrics test_target;

  test_target.PostBrowserStart();
  histogram_tester.ExpectTotalCount(
      kTouchEventFeatureDetectionEnabledHistogramName, 1);
}

// Verify a TouchEventsEnabled value is recorded when an asynchronous device
// scan completes.
TEST_F(ChromeBrowserMainExtraPartsMetricsTest,
       VerifyTouchEventsEnabledIsRecordedWhenDeviceListsComplete) {
  base::HistogramTester histogram_tester;
  ChromeBrowserMainExtraPartsMetrics test_target;

  test_target.PostBrowserStart();
  input_device_client_test_api_.NotifyObserversDeviceListsComplete();
  histogram_tester.ExpectTotalCount(
      kTouchEventFeatureDetectionEnabledHistogramName, 1);
}

// Verify a TouchEventsEnabled value is only recorded once if multiple
// asynchronous device scans happen.
TEST_F(ChromeBrowserMainExtraPartsMetricsTest,
       VerifyTouchEventsEnabledIsOnlyRecordedOnce) {
  base::HistogramTester histogram_tester;
  ChromeBrowserMainExtraPartsMetrics test_target;

  test_target.PostBrowserStart();
  input_device_client_test_api_.NotifyObserversDeviceListsComplete();
  input_device_client_test_api_.NotifyObserversDeviceListsComplete();
  histogram_tester.ExpectTotalCount(
      kTouchEventFeatureDetectionEnabledHistogramName, 1);
}

#else

// Verify a TouchEventsEnabled value is recorded during PostBrowserStart.
TEST_F(ChromeBrowserMainExtraPartsMetricsTest,
       VerifyTouchEventsEnabledIsRecordedAfterPostBrowserStart) {
  base::HistogramTester histogram_tester;
  ChromeBrowserMainExtraPartsMetrics test_target;

  test_target.PostBrowserStart();
  histogram_tester.ExpectTotalCount(
      kTouchEventFeatureDetectionEnabledHistogramName, 1);
}

#endif  // defined(USE_OZONE) || defined(USE_X11)

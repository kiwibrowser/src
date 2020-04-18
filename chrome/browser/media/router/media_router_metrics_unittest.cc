// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/media_router_metrics.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/test/histogram_tester.h"
#include "base/test/simple_test_clock.h"
#include "base/time/time.h"
#include "chrome/browser/ui/media_router/media_cast_mode.h"
#include "chrome/common/media_router/media_sink.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::Bucket;
using testing::ElementsAre;

namespace media_router {

using DialParsingError = SafeDialDeviceDescriptionParser::ParsingError;

TEST(MediaRouterMetricsTest, RecordMediaRouterDialogOrigin) {
  base::HistogramTester tester;
  const MediaRouterDialogOpenOrigin origin1 =
      MediaRouterDialogOpenOrigin::TOOLBAR;
  const MediaRouterDialogOpenOrigin origin2 =
      MediaRouterDialogOpenOrigin::CONTEXTUAL_MENU;

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramIconClickLocation, 0);
  MediaRouterMetrics::RecordMediaRouterDialogOrigin(origin1);
  MediaRouterMetrics::RecordMediaRouterDialogOrigin(origin2);
  MediaRouterMetrics::RecordMediaRouterDialogOrigin(origin1);
  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramIconClickLocation, 3);
  EXPECT_THAT(
      tester.GetAllSamples(MediaRouterMetrics::kHistogramIconClickLocation),
      ElementsAre(Bucket(static_cast<int>(origin1), 2),
                  Bucket(static_cast<int>(origin2), 1)));
}

TEST(MediaRouterMetricsTest, RecordMediaRouterDialogPaint) {
  base::HistogramTester tester;
  const base::TimeDelta delta = base::TimeDelta::FromMilliseconds(10);

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramUiDialogPaint, 0);
  MediaRouterMetrics::RecordMediaRouterDialogPaint(delta);
  tester.ExpectUniqueSample(MediaRouterMetrics::kHistogramUiDialogPaint,
                            delta.InMilliseconds(), 1);
}

TEST(MediaRouterMetricsTest, RecordMediaRouterDialogLoaded) {
  base::HistogramTester tester;
  const base::TimeDelta delta = base::TimeDelta::FromMilliseconds(10);

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramUiDialogLoadedWithData,
                          0);
  MediaRouterMetrics::RecordMediaRouterDialogLoaded(delta);
  tester.ExpectUniqueSample(
      MediaRouterMetrics::kHistogramUiDialogLoadedWithData,
      delta.InMilliseconds(), 1);
}

TEST(MediaRouterMetricsTest, RecordMediaRouterInitialUserAction) {
  base::HistogramTester tester;
  const MediaRouterUserAction action1 = MediaRouterUserAction::START_LOCAL;
  const MediaRouterUserAction action2 = MediaRouterUserAction::CLOSE;
  const MediaRouterUserAction action3 = MediaRouterUserAction::STATUS_REMOTE;

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramUiFirstAction, 0);
  MediaRouterMetrics::RecordMediaRouterInitialUserAction(action3);
  MediaRouterMetrics::RecordMediaRouterInitialUserAction(action2);
  MediaRouterMetrics::RecordMediaRouterInitialUserAction(action3);
  MediaRouterMetrics::RecordMediaRouterInitialUserAction(action1);
  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramUiFirstAction, 4);
  EXPECT_THAT(tester.GetAllSamples(MediaRouterMetrics::kHistogramUiFirstAction),
              ElementsAre(Bucket(static_cast<int>(action1), 1),
                          Bucket(static_cast<int>(action2), 1),
                          Bucket(static_cast<int>(action3), 2)));
}

TEST(MediaRouterMetricsTest, RecordRouteCreationOutcome) {
  base::HistogramTester tester;
  const MediaRouterRouteCreationOutcome outcome1 =
      MediaRouterRouteCreationOutcome::SUCCESS;
  const MediaRouterRouteCreationOutcome outcome2 =
      MediaRouterRouteCreationOutcome::FAILURE_NO_ROUTE;

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramRouteCreationOutcome,
                          0);
  MediaRouterMetrics::RecordRouteCreationOutcome(outcome2);
  MediaRouterMetrics::RecordRouteCreationOutcome(outcome1);
  MediaRouterMetrics::RecordRouteCreationOutcome(outcome2);
  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramRouteCreationOutcome,
                          3);
  EXPECT_THAT(
      tester.GetAllSamples(MediaRouterMetrics::kHistogramRouteCreationOutcome),
      ElementsAre(Bucket(static_cast<int>(outcome1), 1),
                  Bucket(static_cast<int>(outcome2), 2)));
}

TEST(MediaRouterMetricsTest, RecordMediaRouterCastingSource) {
  base::HistogramTester tester;
  const MediaCastMode source1 = MediaCastMode::PRESENTATION;
  const MediaCastMode source2 = MediaCastMode::TAB_MIRROR;
  const MediaCastMode source3 = MediaCastMode::LOCAL_FILE;

  tester.ExpectTotalCount(
      MediaRouterMetrics::kHistogramMediaRouterCastingSource, 0);
  MediaRouterMetrics::RecordMediaRouterCastingSource(source1);
  MediaRouterMetrics::RecordMediaRouterCastingSource(source2);
  MediaRouterMetrics::RecordMediaRouterCastingSource(source2);
  MediaRouterMetrics::RecordMediaRouterCastingSource(source3);
  tester.ExpectTotalCount(
      MediaRouterMetrics::kHistogramMediaRouterCastingSource, 4);
  EXPECT_THAT(tester.GetAllSamples(
                  MediaRouterMetrics::kHistogramMediaRouterCastingSource),
              ElementsAre(Bucket(static_cast<int>(source1), 1),
                          Bucket(static_cast<int>(source2), 2),
                          Bucket(static_cast<int>(source3), 1)));
}

TEST(MediaRouterMetricsTest, RecordDialDeviceDescriptionParsingError) {
  base::HistogramTester tester;
  const DialParsingError action1 = DialParsingError::kMissingUniqueId;
  const DialParsingError action2 = DialParsingError::kMissingFriendlyName;
  const DialParsingError action3 = DialParsingError::kMissingAppUrl;

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramDialParsingError, 0);
  MediaRouterMetrics::RecordDialParsingError(action3);
  MediaRouterMetrics::RecordDialParsingError(action2);
  MediaRouterMetrics::RecordDialParsingError(action3);
  MediaRouterMetrics::RecordDialParsingError(action1);
  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramDialParsingError, 4);
  EXPECT_THAT(
      tester.GetAllSamples(MediaRouterMetrics::kHistogramDialParsingError),
      ElementsAre(Bucket(static_cast<int>(action1), 1),
                  Bucket(static_cast<int>(action2), 1),
                  Bucket(static_cast<int>(action3), 2)));
}

TEST(MediaRouterMetricsTest, RecordPresentationUrlType) {
  base::HistogramTester tester;

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramPresentationUrlType, 0);
  MediaRouterMetrics::RecordPresentationUrlType(GURL("cast:DEADBEEF"));
  MediaRouterMetrics::RecordPresentationUrlType(GURL("dial:AppName"));
  MediaRouterMetrics::RecordPresentationUrlType(GURL("cast-dial:AppName"));
  MediaRouterMetrics::RecordPresentationUrlType(GURL("https://example.com"));
  MediaRouterMetrics::RecordPresentationUrlType(GURL("http://example.com"));
  MediaRouterMetrics::RecordPresentationUrlType(
      GURL("https://google.com/cast#__castAppId__=DEADBEEF"));
  MediaRouterMetrics::RecordPresentationUrlType(GURL("remote-playback:foo"));
  MediaRouterMetrics::RecordPresentationUrlType(GURL("test:test"));

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramPresentationUrlType, 8);
  EXPECT_THAT(
      tester.GetAllSamples(MediaRouterMetrics::kHistogramPresentationUrlType),
      ElementsAre(
          Bucket(static_cast<int>(PresentationUrlType::kOther), 1),
          Bucket(static_cast<int>(PresentationUrlType::kCast), 1),
          Bucket(static_cast<int>(PresentationUrlType::kCastDial), 1),
          Bucket(static_cast<int>(PresentationUrlType::kCastLegacy), 1),
          Bucket(static_cast<int>(PresentationUrlType::kDial), 1),
          Bucket(static_cast<int>(PresentationUrlType::kHttp), 1),
          Bucket(static_cast<int>(PresentationUrlType::kHttps), 1),
          Bucket(static_cast<int>(PresentationUrlType::kRemotePlayback), 1)));
}

TEST(MediaRouterMetricsTest, RecordMediaSinkType) {
  base::HistogramTester tester;
  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramMediaSinkType, 0);

  MediaRouterMetrics::RecordMediaSinkType(SinkIconType::WIRED_DISPLAY);
  MediaRouterMetrics::RecordMediaSinkType(SinkIconType::CAST);
  MediaRouterMetrics::RecordMediaSinkType(SinkIconType::CAST_AUDIO);
  MediaRouterMetrics::RecordMediaSinkType(SinkIconType::HANGOUT);
  MediaRouterMetrics::RecordMediaSinkType(SinkIconType::CAST);
  MediaRouterMetrics::RecordMediaSinkType(SinkIconType::GENERIC);

  tester.ExpectTotalCount(MediaRouterMetrics::kHistogramMediaSinkType, 6);
  EXPECT_THAT(
      tester.GetAllSamples(MediaRouterMetrics::kHistogramMediaSinkType),
      ElementsAre(Bucket(static_cast<int>(SinkIconType::CAST), 2),
                  Bucket(static_cast<int>(SinkIconType::CAST_AUDIO), 1),
                  Bucket(static_cast<int>(SinkIconType::HANGOUT), 1),
                  Bucket(static_cast<int>(SinkIconType::WIRED_DISPLAY), 1),
                  Bucket(static_cast<int>(SinkIconType::GENERIC), 1)));
}

}  // namespace media_router

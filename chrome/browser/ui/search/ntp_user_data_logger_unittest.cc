// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/search/ntp_user_data_logger.h"

#include <memory>
#include <string>
#include <vector>

#include "base/metrics/histogram.h"
#include "base/test/histogram_tester.h"
#include "base/time/time.h"
#include "chrome/common/search/ntp_logging_events.h"
#include "chrome/common/url_constants.h"
#include "components/favicon_base/favicon_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Bucket;
using ntp_tiles::TileTitleSource;
using ntp_tiles::TileSource;
using ntp_tiles::TileVisualType;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::SizeIs;

namespace {

constexpr int kUnknownTitleSource = static_cast<int>(TileTitleSource::UNKNOWN);
constexpr int kManifestTitleSource =
    static_cast<int>(TileTitleSource::MANIFEST);
constexpr int kMetaTagTitleSource = static_cast<int>(TileTitleSource::META_TAG);
constexpr int kTitleTagTitleSource =
    static_cast<int>(TileTitleSource::TITLE_TAG);
constexpr int kInferredTitleSource =
    static_cast<int>(TileTitleSource::INFERRED);

using Sample = base::HistogramBase::Sample;
using Samples = std::vector<Sample>;

// Helper function that uses sensible defaults for irrelevant fields of
// NTPTileImpression.
ntp_tiles::NTPTileImpression MakeNTPTileImpression(int index,
                                                   TileSource source,
                                                   TileTitleSource title_source,
                                                   TileVisualType visual_type) {
  return ntp_tiles::NTPTileImpression(index, source, title_source, visual_type,
                                      favicon_base::IconType::kInvalid,
                                      /*data_generation_time=*/base::Time(),
                                      /*url_for_rappor=*/GURL());
}

class TestNTPUserDataLogger : public NTPUserDataLogger {
 public:
  explicit TestNTPUserDataLogger(const GURL& ntp_url)
      : NTPUserDataLogger(nullptr) {
    set_ntp_url_for_testing(ntp_url);
  }

  ~TestNTPUserDataLogger() override {}

  bool DefaultSearchProviderIsGoogle() const override { return is_google_; }

  bool is_google_ = true;
};

MATCHER_P3(IsBucketBetween, lower_bound, upper_bound, count, "") {
  return arg.min >= lower_bound && arg.min <= upper_bound && arg.count == count;
}

}  // namespace

TEST(NTPUserDataLoggerTest, ShouldRecordNumberOfTiles) {
  base::HistogramTester histogram_tester;

  // Ensure non-zero statistics.
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  const base::TimeDelta delta = base::TimeDelta::FromMilliseconds(73);

  for (int i = 0; i < 8; ++i) {
    logger.LogMostVisitedImpression(MakeNTPTileImpression(
        i, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
        TileVisualType::THUMBNAIL));
  }
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta);
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.NumberOfTiles"),
              ElementsAre(Bucket(8, 1)));

  // We should not log again for the same NTP.
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta);
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.NumberOfTiles"),
              ElementsAre(Bucket(8, 1)));

  // Navigating away and back resets stats.
  logger.NavigatedFromURLToURL(GURL("chrome://newtab/"),
                               GURL("http://chromium.org"));
  logger.NavigatedFromURLToURL(GURL("http://chromium.org"),
                               GURL("chrome://newtab/"));
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta);
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.NumberOfTiles"),
              ElementsAre(Bucket(0, 1), Bucket(8, 1)));
}

TEST(NTPUserDataLoggerTest, ShouldNotRecordImpressionsBeforeAllTilesLoaded) {
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  base::HistogramTester histogram_tester;

  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(1, TileSource::TOP_SITES, TileTitleSource::INFERRED,
                            TileVisualType::THUMBNAIL_FAILED));

  // The actual histograms are emitted only after the ALL_TILES_LOADED event,
  // so at this point everything should still be empty.
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType"), IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.client"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.server"),
              IsEmpty());
}

TEST(NTPUserDataLoggerTest, ShouldRecordImpressions) {
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  base::HistogramTester histogram_tester;

  // Impressions increment the associated bins.
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      1, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL_FAILED));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      2, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      3, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      4, TileSource::TOP_SITES, TileTitleSource::TITLE_TAG,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(5, TileSource::TOP_SITES, TileTitleSource::MANIFEST,
                            TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(6, TileSource::POPULAR, TileTitleSource::TITLE_TAG,
                            TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      7, TileSource::POPULAR_BAKED_IN, TileTitleSource::META_TAG,
      TileVisualType::THUMBNAIL));

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, base::TimeDelta::FromMilliseconds(73));

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1),
                  Bucket(4, 1), Bucket(5, 1), Bucket(6, 1), Bucket(7, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      ElementsAre(Bucket(4, 1), Bucket(5, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.popular_fetched"),
              ElementsAre(Bucket(6, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.popular_baked_in"),
              ElementsAre(Bucket(7, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 7),
                  Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 3),
                  Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
              ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 2)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.popular_fetched"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.popular_baked_in"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle"),
              ElementsAre(Bucket(kManifestTitleSource, 1),
                          Bucket(kMetaTagTitleSource, 1),
                          Bucket(kTitleTagTitleSource, 2),
                          Bucket(kInferredTitleSource, 4)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.server"),
              ElementsAre(Bucket(kInferredTitleSource, 4)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.client"),
              ElementsAre(Bucket(kManifestTitleSource, 1),
                          Bucket(kTitleTagTitleSource, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitle.popular_fetched"),
      ElementsAre(Bucket(kTitleTagTitleSource, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitle.popular_baked_in"),
      ElementsAre(Bucket(kMetaTagTitleSource, 1)));
}

TEST(NTPUserDataLoggerTest, ShouldNotRecordRepeatedImpressions) {
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  base::HistogramTester histogram_tester;

  // Impressions increment the associated bins.
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      1, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL_FAILED));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      2, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      3, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));

  // Repeated impressions for the same bins are ignored.
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
      TileVisualType::THUMBNAIL_FAILED));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(1, TileSource::TOP_SITES, TileTitleSource::UNKNOWN,
                            TileVisualType::THUMBNAIL_FAILED));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      2, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(3, TileSource::TOP_SITES, TileTitleSource::UNKNOWN,
                            TileVisualType::THUMBNAIL));

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, base::TimeDelta::FromMilliseconds(73));

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 3),
                  Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 3),
                  Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle"),
              ElementsAre(Bucket(kInferredTitleSource, 4)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.server"),
              ElementsAre(Bucket(kInferredTitleSource, 4)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.client"),
              IsEmpty());
}

TEST(NTPUserDataLoggerTest, ShouldNotRecordImpressionsForBinsBeyondEight) {
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  base::HistogramTester histogram_tester;

  // Impressions increment the associated bins.
  for (int bin = 0; bin < 8; bin++) {
    logger.LogMostVisitedImpression(MakeNTPTileImpression(
        bin, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
        TileVisualType::THUMBNAIL));
  }

  // Impressions are silently ignored for tiles >= 8.
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(8, TileSource::TOP_SITES, TileTitleSource::UNKNOWN,
                            TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(9, TileSource::TOP_SITES, TileTitleSource::UNKNOWN,
                            TileVisualType::THUMBNAIL_FAILED));

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, base::TimeDelta::FromMilliseconds(73));

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1),
                  Bucket(4, 1), Bucket(5, 1), Bucket(6, 1), Bucket(7, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1),
                  Bucket(4, 1), Bucket(5, 1), Bucket(6, 1), Bucket(7, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType"),
              ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 8)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
              ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 8)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle"),
              ElementsAre(Bucket(kInferredTitleSource, 8)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.server"),
              ElementsAre(Bucket(kInferredTitleSource, 8)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.client"),
              IsEmpty());
}

TEST(NTPUserDataLoggerTest, ShouldRecordImpressionsAgainAfterNavigating) {
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  // Record some previous tile impressions.
  for (int bin = 0; bin < 8; bin++) {
    logger.LogMostVisitedImpression(MakeNTPTileImpression(
        bin, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
        TileVisualType::THUMBNAIL));
  }
  logger.LogEvent(NTP_ALL_TILES_LOADED, base::TimeDelta::FromMilliseconds(73));

  // After navigating away from the NTP and back, we should record again.
  base::HistogramTester histogram_tester;

  logger.NavigatedFromURLToURL(GURL("chrome://newtab/"),
                               GURL("http://chromium.org"));
  logger.NavigatedFromURLToURL(GURL("http://chromium.org"),
                               GURL("chrome://newtab/"));

  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(1, TileSource::POPULAR, TileTitleSource::MANIFEST,
                            TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      2, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::INFERRED,
      TileVisualType::THUMBNAIL));
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(3, TileSource::TOP_SITES, TileTitleSource::MANIFEST,
                            TileVisualType::THUMBNAIL_FAILED));
  logger.LogEvent(NTP_ALL_TILES_LOADED, base::TimeDelta::FromMilliseconds(73));

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      ElementsAre(Bucket(0, 1), Bucket(2, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      ElementsAre(Bucket(3, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.popular_fetched"),
              ElementsAre(Bucket(1, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.popular_baked_in"),
              IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 3),
                  Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
              ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 2)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.popular_fetched"),
      ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.popular_baked_in"),
      IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle"),
              ElementsAre(Bucket(kManifestTitleSource, 2),
                          Bucket(kInferredTitleSource, 2)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.server"),
              ElementsAre(Bucket(kInferredTitleSource, 2)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.client"),
              ElementsAre(Bucket(kManifestTitleSource, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitle.popular_fetched"),
      ElementsAre(Bucket(kManifestTitleSource, 1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitle.popular_baked_in"),
      IsEmpty());
}

TEST(NTPUserDataLoggerTest, ShouldRecordNavigations) {
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  {
    base::HistogramTester histogram_tester;

    logger.LogMostVisitedNavigation(MakeNTPTileImpression(
        0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
        TileVisualType::THUMBNAIL));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
                ElementsAre(Bucket(0, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
                ElementsAre(Bucket(0, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
                IsEmpty());

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked"),
                ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.server"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.client"),
        IsEmpty());
  }

  {
    base::HistogramTester histogram_tester;

    logger.LogMostVisitedNavigation(MakeNTPTileImpression(
        1, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
        TileVisualType::THUMBNAIL_FAILED));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
                ElementsAre(Bucket(1, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
                ElementsAre(Bucket(1, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
                IsEmpty());

    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.server"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.client"),
        IsEmpty());
  }

  {
    base::HistogramTester histogram_tester;

    logger.LogMostVisitedNavigation(MakeNTPTileImpression(
        2, TileSource::TOP_SITES, TileTitleSource::MANIFEST,
        TileVisualType::THUMBNAIL_FAILED));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
                ElementsAre(Bucket(2, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
                IsEmpty());
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
                ElementsAre(Bucket(2, 1)));

    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.server"),
        IsEmpty());
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.client"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL_FAILED, 1)));
  }

  {
    base::HistogramTester histogram_tester;

    logger.LogMostVisitedNavigation(
        MakeNTPTileImpression(3, TileSource::POPULAR, TileTitleSource::META_TAG,
                              TileVisualType::THUMBNAIL));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
                ElementsAre(Bucket(3, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
                IsEmpty());
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
                IsEmpty());
    EXPECT_THAT(histogram_tester.GetAllSamples(
                    "NewTabPage.MostVisited.popular_fetched"),
                ElementsAre(Bucket(3, 1)));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked"),
                ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.server"),
        IsEmpty());
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.client"),
        IsEmpty());
    EXPECT_THAT(histogram_tester.GetAllSamples(
                    "NewTabPage.TileTypeClicked.popular_fetched"),
                ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked"),
                ElementsAre(Bucket(kMetaTagTitleSource, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked.server"),
        IsEmpty());
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked.client"),
        IsEmpty());
    EXPECT_THAT(histogram_tester.GetAllSamples(
                    "NewTabPage.TileTitleClicked.popular_fetched"),
                ElementsAre(Bucket(kMetaTagTitleSource, 1)));
  }

  {
    base::HistogramTester histogram_tester;

    // Navigations always increase.
    logger.LogMostVisitedNavigation(MakeNTPTileImpression(
        0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
        TileVisualType::THUMBNAIL));
    logger.LogMostVisitedNavigation(MakeNTPTileImpression(
        1, TileSource::TOP_SITES, TileTitleSource::TITLE_TAG,
        TileVisualType::THUMBNAIL));
    logger.LogMostVisitedNavigation(MakeNTPTileImpression(
        2, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
        TileVisualType::THUMBNAIL));
    logger.LogMostVisitedNavigation(
        MakeNTPTileImpression(3, TileSource::POPULAR, TileTitleSource::MANIFEST,
                              TileVisualType::THUMBNAIL));

    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
        ElementsAre(Bucket(0, 1), Bucket(1, 1), Bucket(2, 1), Bucket(3, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
                ElementsAre(Bucket(0, 1), Bucket(2, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
                ElementsAre(Bucket(1, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples(
                    "NewTabPage.MostVisited.popular_fetched"),
                ElementsAre(Bucket(3, 1)));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked"),
                ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 4)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.server"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 2)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTypeClicked.client"),
        ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples(
                    "NewTabPage.TileTypeClicked.popular_fetched"),
                ElementsAre(Bucket(ntp_tiles::TileVisualType::THUMBNAIL, 1)));

    EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked"),
                ElementsAre(Bucket(kUnknownTitleSource, 2),
                            Bucket(kManifestTitleSource, 1),
                            Bucket(kTitleTagTitleSource, 1)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked.server"),
        ElementsAre(Bucket(kUnknownTitleSource, 2)));
    EXPECT_THAT(
        histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked.client"),
        ElementsAre(Bucket(kTitleTagTitleSource, 1)));
    EXPECT_THAT(histogram_tester.GetAllSamples(
                    "NewTabPage.TileTitleClicked.popular_fetched"),
                ElementsAre(Bucket(kManifestTitleSource, 1)));
  }
}

TEST(NTPUserDataLoggerTest, ShouldRecordLoadTime) {
  base::HistogramTester histogram_tester;

  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  base::TimeDelta delta_tiles_received = base::TimeDelta::FromMilliseconds(10);
  base::TimeDelta delta_tiles_loaded = base::TimeDelta::FromMilliseconds(100);

  // Send the ALL_TILES_RECEIVED event.
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received);

  // Log a TOP_SITES impression (for the .MostVisited vs .MostLikely split in
  // the time histograms).
  logger.LogMostVisitedImpression(
      MakeNTPTileImpression(0, TileSource::TOP_SITES, TileTitleSource::UNKNOWN,
                            TileVisualType::THUMBNAIL));

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TilesReceivedTime"),
              SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TilesReceivedTime.MostVisited"),
              SizeIs(1));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TilesReceivedTime.MostLikely"),
      IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime"), SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.MostVisited"),
              SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.MostLikely"),
              IsEmpty());

  histogram_tester.ExpectTimeBucketCount("NewTabPage.TilesReceivedTime",
                                         delta_tiles_received, 1);
  histogram_tester.ExpectTimeBucketCount(
      "NewTabPage.TilesReceivedTime.MostVisited", delta_tiles_received, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.MostVisited",
                                         delta_tiles_loaded, 1);

  // We should not log again for the same NTP.
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received);
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.TilesReceivedTime",
                                         delta_tiles_received, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded, 1);

  // After navigating away from the NTP and back, we record again.
  logger.NavigatedFromURLToURL(GURL("chrome://newtab/"),
                               GURL("http://chromium.org"));
  logger.NavigatedFromURLToURL(GURL("http://chromium.org"),
                               GURL("chrome://newtab/"));

  // This time, log a SUGGESTIONS_SERVICE impression, so the times will end up
  // in .MostLikely.
  logger.LogMostVisitedImpression(MakeNTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
      TileVisualType::THUMBNAIL));

  base::TimeDelta delta_tiles_received2 = base::TimeDelta::FromMilliseconds(50);
  base::TimeDelta delta_tiles_loaded2 = base::TimeDelta::FromMilliseconds(500);
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received2);
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded2);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TilesReceivedTime"),
              SizeIs(2));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TilesReceivedTime.MostVisited"),
              SizeIs(1));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TilesReceivedTime.MostLikely"),
      SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime"), SizeIs(2));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.MostVisited"),
              SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.MostLikely"),
              SizeIs(1));

  histogram_tester.ExpectTimeBucketCount("NewTabPage.TilesReceivedTime",
                                         delta_tiles_received2, 1);
  histogram_tester.ExpectTimeBucketCount(
      "NewTabPage.TilesReceivedTime.MostLikely", delta_tiles_received2, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded2, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.MostLikely",
                                         delta_tiles_loaded2, 1);
}

TEST(NTPUserDataLoggerTest, ShouldRecordLoadTimeLocalNTPGoogle) {
  base::HistogramTester histogram_tester;

  TestNTPUserDataLogger logger((GURL(chrome::kChromeSearchLocalNtpUrl)));
  logger.is_google_ = true;

  base::TimeDelta delta_tiles_received = base::TimeDelta::FromMilliseconds(10);
  base::TimeDelta delta_tiles_loaded = base::TimeDelta::FromMilliseconds(100);

  // Send the ALL_TILES_RECEIVED event.
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received);

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime"), SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP"),
              SizeIs(1));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP.Google"),
      SizeIs(1));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP.Other"),
      IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web"),
              IsEmpty());

  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.LocalNTP",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.LocalNTP.Google",
                                         delta_tiles_loaded, 1);
}

TEST(NTPUserDataLoggerTest, ShouldRecordLoadTimeLocalNTPOther) {
  base::HistogramTester histogram_tester;

  TestNTPUserDataLogger logger((GURL(chrome::kChromeSearchLocalNtpUrl)));
  logger.is_google_ = false;

  base::TimeDelta delta_tiles_received = base::TimeDelta::FromMilliseconds(10);
  base::TimeDelta delta_tiles_loaded = base::TimeDelta::FromMilliseconds(100);

  // Send the ALL_TILES_RECEIVED event.
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received);

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime"), SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP"),
              SizeIs(1));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP.Google"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP.Other"),
      SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web"),
              IsEmpty());

  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.LocalNTP",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.LocalNTP.Other",
                                         delta_tiles_loaded, 1);
}

TEST(NTPUserDataLoggerTest, ShouldRecordLoadTimeRemoteNTPGoogle) {
  base::HistogramTester histogram_tester;

  TestNTPUserDataLogger logger(GURL("https://www.google.com/_/chrome/newtab"));
  logger.is_google_ = true;

  base::TimeDelta delta_tiles_received = base::TimeDelta::FromMilliseconds(10);
  base::TimeDelta delta_tiles_loaded = base::TimeDelta::FromMilliseconds(100);

  // Send the ALL_TILES_RECEIVED event.
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received);

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime"), SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web"),
              SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web.Google"),
              SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web.Other"),
              IsEmpty());

  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.Web",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.Web.Google",
                                         delta_tiles_loaded, 1);
}

TEST(NTPUserDataLoggerTest, ShouldRecordLoadTimeRemoteNTPOther) {
  base::HistogramTester histogram_tester;

  TestNTPUserDataLogger logger(GURL("https://www.notgoogle.com/newtab"));
  logger.is_google_ = false;

  base::TimeDelta delta_tiles_received = base::TimeDelta::FromMilliseconds(10);
  base::TimeDelta delta_tiles_loaded = base::TimeDelta::FromMilliseconds(100);

  // Send the ALL_TILES_RECEIVED event.
  logger.LogEvent(NTP_ALL_TILES_RECEIVED, delta_tiles_received);

  // Send the ALL_TILES_LOADED event, this should trigger emitting histograms.
  logger.LogEvent(NTP_ALL_TILES_LOADED, delta_tiles_loaded);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime"), SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.LocalNTP"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web"),
              SizeIs(1));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web.Google"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.LoadTime.Web.Other"),
              SizeIs(1));

  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.Web",
                                         delta_tiles_loaded, 1);
  histogram_tester.ExpectTimeBucketCount("NewTabPage.LoadTime.Web.Other",
                                         delta_tiles_loaded, 1);
}

TEST(NTPUserDataLoggerTest, ShouldRecordImpressionsAge) {
  base::HistogramTester histogram_tester;

  // Ensure non-zero statistics.
  TestNTPUserDataLogger logger(GURL("chrome://newtab/"));

  constexpr base::TimeDelta delta = base::TimeDelta::FromMilliseconds(0);
  constexpr base::TimeDelta kSuggestionAge = base::TimeDelta::FromMinutes(1);
  constexpr base::TimeDelta kBucketTolerance = base::TimeDelta::FromSeconds(20);

  logger.LogMostVisitedImpression(ntp_tiles::NTPTileImpression(
      0, TileSource::SUGGESTIONS_SERVICE, TileTitleSource::UNKNOWN,
      TileVisualType::THUMBNAIL, favicon_base::IconType::kInvalid,
      base::Time::Now() - kSuggestionAge, GURL()));

  logger.LogEvent(NTP_ALL_TILES_LOADED, delta);

  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpressionAge.server"),
              ElementsAre(IsBucketBetween(
                  (kSuggestionAge - kBucketTolerance).InSeconds(),
                  (kSuggestionAge + kBucketTolerance).InSeconds(),
                  /*count=*/1)));
}

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/metrics.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "components/rappor/test_rappor_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_tiles {
namespace metrics {
namespace {

constexpr int kUnknownTitleSource = static_cast<int>(TileTitleSource::UNKNOWN);
constexpr int kManifestTitleSource =
    static_cast<int>(TileTitleSource::MANIFEST);
constexpr int kMetaTagTitleSource = static_cast<int>(TileTitleSource::META_TAG);
constexpr int kTitleTagTitleSource =
    static_cast<int>(TileTitleSource::TITLE_TAG);
constexpr int kInferredTitleSource =
    static_cast<int>(TileTitleSource::INFERRED);

using favicon_base::IconType;
using testing::ElementsAre;
using testing::IsEmpty;

MATCHER_P3(IsBucketBetween, lower_bound, upper_bound, count, "") {
  return arg.min >= lower_bound && arg.min <= upper_bound && arg.count == count;
}

// Builder for instances of NTPTileImpression that uses sensible defaults.
class Builder {
 public:
  Builder() {}

  Builder& WithIndex(int index) {
    impression_.index = index;
    return *this;
  }
  Builder& WithSource(TileSource source) {
    impression_.source = source;
    return *this;
  }
  Builder& WithTitleSource(TileTitleSource title_source) {
    impression_.title_source = title_source;
    return *this;
  }
  Builder& WithVisualType(TileVisualType visual_type) {
    impression_.visual_type = visual_type;
    return *this;
  }
  Builder& WithIconType(favicon_base::IconType icon_type) {
    impression_.icon_type = icon_type;
    return *this;
  }
  Builder& WithDataGenerationTime(base::Time data_generation_time) {
    impression_.data_generation_time = data_generation_time;
    return *this;
  }
  Builder& WithUrl(const GURL& url) {
    impression_.url_for_rappor = url;
    return *this;
  }

  NTPTileImpression Build() { return impression_; }

 private:
  NTPTileImpression impression_;
};

TEST(RecordPageImpressionTest, ShouldRecordNumberOfTiles) {
  base::HistogramTester histogram_tester;
  RecordPageImpression(5);
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.NumberOfTiles"),
              ElementsAre(base::Bucket(/*min=*/5, /*count=*/1)));
}

TEST(RecordTileImpressionTest, ShouldRecordUmaForIcons) {
  base::HistogramTester histogram_tester;

  RecordTileImpression(Builder()
                           .WithIndex(0)
                           .WithSource(TileSource::TOP_SITES)
                           .WithVisualType(ICON_REAL)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(1)
                           .WithSource(TileSource::TOP_SITES)
                           .WithVisualType(ICON_REAL)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(2)
                           .WithSource(TileSource::TOP_SITES)
                           .WithVisualType(ICON_REAL)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(3)
                           .WithSource(TileSource::TOP_SITES)
                           .WithVisualType(ICON_COLOR)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(4)
                           .WithSource(TileSource::TOP_SITES)
                           .WithVisualType(ICON_COLOR)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(5)
                           .WithSource(TileSource::SUGGESTIONS_SERVICE)
                           .WithVisualType(ICON_REAL)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(6)
                           .WithSource(TileSource::SUGGESTIONS_SERVICE)
                           .WithVisualType(ICON_DEFAULT)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(7)
                           .WithSource(TileSource::POPULAR)
                           .WithVisualType(ICON_COLOR)
                           .Build(),
                       /*rappor_service=*/nullptr);

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      ElementsAre(base::Bucket(/*min=*/0, /*count=*/1),
                  base::Bucket(/*min=*/1, /*count=*/1),
                  base::Bucket(/*min=*/2, /*count=*/1),
                  base::Bucket(/*min=*/3, /*count=*/1),
                  base::Bucket(/*min=*/4, /*count=*/1),
                  base::Bucket(/*min=*/5, /*count=*/1),
                  base::Bucket(/*min=*/6, /*count=*/1),
                  base::Bucket(/*min=*/7, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      ElementsAre(base::Bucket(/*min=*/5, /*count=*/1),
                  base::Bucket(/*min=*/6, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      ElementsAre(base::Bucket(/*min=*/0, /*count=*/1),
                  base::Bucket(/*min=*/1, /*count=*/1),
                  base::Bucket(/*min=*/2, /*count=*/1),
                  base::Bucket(/*min=*/3, /*count=*/1),
                  base::Bucket(/*min=*/4, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.popular_fetched"),
              ElementsAre(base::Bucket(/*min=*/7, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType"),
              ElementsAre(base::Bucket(/*min=*/ICON_REAL, /*count=*/4),
                          base::Bucket(/*min=*/ICON_COLOR, /*count=*/3),
                          base::Bucket(/*min=*/ICON_DEFAULT, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
              ElementsAre(base::Bucket(/*min=*/ICON_REAL, /*count=*/1),
                          base::Bucket(/*min=*/ICON_DEFAULT, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
              ElementsAre(base::Bucket(/*min=*/ICON_REAL, /*count=*/3),
                          base::Bucket(/*min=*/ICON_COLOR, /*count=*/2)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.popular_fetched"),
      ElementsAre(base::Bucket(/*min=*/ICON_COLOR,
                               /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.IconsReal"),
              ElementsAre(base::Bucket(/*min=*/0, /*count=*/1),
                          base::Bucket(/*min=*/1, /*count=*/1),
                          base::Bucket(/*min=*/2, /*count=*/1),
                          base::Bucket(/*min=*/5, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.IconsColor"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1),
                          base::Bucket(/*min=*/4, /*count=*/1),
                          base::Bucket(/*min=*/7, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.IconsGray"),
              ElementsAre(base::Bucket(/*min=*/6, /*count=*/1)));
}

TEST(RecordTileImpressionTest, ShouldRecordUmaForThumbnails) {
  base::HistogramTester histogram_tester;

  RecordTileImpression(Builder()
                           .WithIndex(0)
                           .WithSource(TileSource::TOP_SITES)
                           .WithVisualType(THUMBNAIL_FAILED)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(1)
                           .WithSource(TileSource::SUGGESTIONS_SERVICE)
                           .WithVisualType(THUMBNAIL)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithIndex(2)
                           .WithSource(TileSource::POPULAR)
                           .WithVisualType(THUMBNAIL)
                           .Build(),
                       /*rappor_service=*/nullptr);

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression"),
      ElementsAre(base::Bucket(/*min=*/0, /*count=*/1),
                  base::Bucket(/*min=*/1, /*count=*/1),
                  base::Bucket(/*min=*/2, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.server"),
      ElementsAre(base::Bucket(/*min=*/1, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpression.client"),
      ElementsAre(base::Bucket(/*min=*/0, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.popular_fetched"),
              ElementsAre(base::Bucket(/*min=*/2, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType"),
              ElementsAre(base::Bucket(/*min=*/THUMBNAIL, /*count=*/2),
                          base::Bucket(/*min=*/THUMBNAIL_FAILED, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.server"),
              ElementsAre(base::Bucket(/*min=*/THUMBNAIL, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileType.client"),
              ElementsAre(base::Bucket(/*min=*/THUMBNAIL_FAILED, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileType.popular_fetched"),
      ElementsAre(base::Bucket(/*min=*/THUMBNAIL, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.IconsReal"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.IconsColor"),
              IsEmpty());
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpression.IconsGray"),
              IsEmpty());
}

TEST(RecordTileImpressionTest, ShouldRecordImpressionsForTileTitleSource) {
  base::HistogramTester histogram_tester;
  RecordTileImpression(Builder()
                           .WithSource(TileSource::TOP_SITES)
                           .WithTitleSource(TileTitleSource::UNKNOWN)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithSource(TileSource::SUGGESTIONS_SERVICE)
                           .WithTitleSource(TileTitleSource::INFERRED)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithSource(TileSource::POPULAR)
                           .WithTitleSource(TileTitleSource::TITLE_TAG)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithSource(TileSource::POPULAR)
                           .WithTitleSource(TileTitleSource::MANIFEST)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithSource(TileSource::POPULAR_BAKED_IN)
                           .WithTitleSource(TileTitleSource::TITLE_TAG)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithSource(TileSource::POPULAR_BAKED_IN)
                           .WithTitleSource(TileTitleSource::META_TAG)
                           .Build(),
                       /*rappor_service=*/nullptr);

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.client"),
              ElementsAre(base::Bucket(kUnknownTitleSource, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle.server"),
              ElementsAre(base::Bucket(kInferredTitleSource, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitle.popular_fetched"),
      ElementsAre(base::Bucket(kManifestTitleSource, /*count=*/1),
                  base::Bucket(kTitleTagTitleSource, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitle.popular_baked_in"),
      ElementsAre(base::Bucket(kMetaTagTitleSource, /*count=*/1),
                  base::Bucket(kTitleTagTitleSource, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitle"),
              ElementsAre(base::Bucket(kUnknownTitleSource, /*count=*/1),
                          base::Bucket(kManifestTitleSource, /*count=*/1),
                          base::Bucket(kMetaTagTitleSource, /*count=*/1),
                          base::Bucket(kTitleTagTitleSource, /*count=*/2),
                          base::Bucket(kInferredTitleSource, /*count=*/1)));
}

TEST(RecordTileImpressionTest, ShouldRecordAge) {
  const base::TimeDelta kSuggestionAge = base::TimeDelta::FromMinutes(1);
  const base::TimeDelta kBucketTolerance = base::TimeDelta::FromSeconds(20);
  base::HistogramTester histogram_tester;
  RecordTileImpression(
      Builder()
          .WithSource(TileSource::SUGGESTIONS_SERVICE)
          .WithDataGenerationTime(base::Time::Now() - kSuggestionAge)
          .Build(),
      /*rappor_service=*/nullptr);

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.SuggestionsImpressionAge"),
      ElementsAre(
          IsBucketBetween((kSuggestionAge - kBucketTolerance).InSeconds(),
                          (kSuggestionAge + kBucketTolerance).InSeconds(),
                          /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.SuggestionsImpressionAge.server"),
              ElementsAre(IsBucketBetween(
                  (kSuggestionAge - kBucketTolerance).InSeconds(),
                  (kSuggestionAge + kBucketTolerance).InSeconds(),
                  /*count=*/1)));
}

TEST(RecordTileImpressionTest, ShouldRecordUmaForIconType) {
  base::HistogramTester histogram_tester;

  RecordTileImpression(Builder()
                           .WithVisualType(ICON_COLOR)
                           .WithIconType(IconType::kTouchIcon)
                           .Build(),
                       /*rappor_service=*/nullptr);
  RecordTileImpression(Builder()
                           .WithVisualType(ICON_REAL)
                           .WithIconType(IconType::kWebManifestIcon)
                           .Build(),
                       /*rappor_service=*/nullptr);

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileFaviconType.IconsColor"),
      ElementsAre(base::Bucket(/*min=*/2, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileFaviconType.IconsReal"),
      ElementsAre(base::Bucket(/*min=*/4, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileFaviconType"),
              ElementsAre(base::Bucket(/*min=*/2, /*count=*/1),
                          base::Bucket(/*min=*/4, /*count=*/1)));
}

TEST(RecordTileClickTest, ShouldRecordUmaForIcon) {
  base::HistogramTester histogram_tester;
  RecordTileClick(Builder()
                      .WithIndex(3)
                      .WithSource(TileSource::TOP_SITES)
                      .WithVisualType(ICON_REAL)
                      .Build());

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
              IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.popular_fetched"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsReal"),
      ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsColor"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsGray"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.Thumbnail"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.ThumbnailFailed"),
      IsEmpty());
}

TEST(RecordTileClickTest, ShouldRecordUmaForThumbnail) {
  base::HistogramTester histogram_tester;
  RecordTileClick(Builder()
                      .WithIndex(3)
                      .WithSource(TileSource::TOP_SITES)
                      .WithVisualType(THUMBNAIL)
                      .Build());

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
              IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.popular_fetched"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsReal"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsColor"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsGray"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.Thumbnail"),
      ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.ThumbnailFailed"),
      IsEmpty());
}

TEST(RecordTileClickTest, ShouldNotRecordUnknownTileType) {
  base::HistogramTester histogram_tester;
  RecordTileClick(Builder()
                      .WithIndex(3)
                      .WithSource(TileSource::TOP_SITES)
                      .WithVisualType(UNKNOWN_TILE_TYPE)
                      .Build());

  // The click should still get recorded.
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.client"),
              ElementsAre(base::Bucket(/*min=*/3, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisited.server"),
              IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.popular_fetched"),
      IsEmpty());
  // But all of the tile type histograms should be empty.
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsReal"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsColor"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.IconsGray"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.Thumbnail"),
      IsEmpty());
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisited.ThumbnailFailed"),
      IsEmpty());
}

TEST(RecordTileImpressionTest, ShouldRecordRappor) {
  rappor::TestRapporServiceImpl rappor_service;

  RecordTileImpression(Builder()
                           .WithVisualType(ICON_REAL)
                           .WithUrl(GURL("http://www.site1.com/"))
                           .Build(),
                       &rappor_service);
  RecordTileImpression(Builder()
                           .WithVisualType(ICON_COLOR)
                           .WithUrl(GURL("http://www.site2.com/"))
                           .Build(),
                       &rappor_service);
  RecordTileImpression(Builder()
                           .WithVisualType(ICON_DEFAULT)
                           .WithUrl(GURL("http://www.site3.com/"))
                           .Build(),
                       &rappor_service);

  EXPECT_EQ(3, rappor_service.GetReportsCount());

  {
    std::string sample;
    rappor::RapporType type;
    EXPECT_TRUE(rappor_service.GetRecordedSampleForMetric(
        "NTP.SuggestionsImpressions.IconsReal", &sample, &type));
    EXPECT_EQ("site1.com", sample);
    EXPECT_EQ(rappor::ETLD_PLUS_ONE_RAPPOR_TYPE, type);
  }

  {
    std::string sample;
    rappor::RapporType type;
    EXPECT_TRUE(rappor_service.GetRecordedSampleForMetric(
        "NTP.SuggestionsImpressions.IconsColor", &sample, &type));
    EXPECT_EQ("site2.com", sample);
    EXPECT_EQ(rappor::ETLD_PLUS_ONE_RAPPOR_TYPE, type);
  }

  {
    std::string sample;
    rappor::RapporType type;
    EXPECT_TRUE(rappor_service.GetRecordedSampleForMetric(
        "NTP.SuggestionsImpressions.IconsGray", &sample, &type));
    EXPECT_EQ("site3.com", sample);
    EXPECT_EQ(rappor::ETLD_PLUS_ONE_RAPPOR_TYPE, type);
  }
}

TEST(RecordTileImpressionTest, ShouldNotRecordRapporForUnknownTileType) {
  rappor::TestRapporServiceImpl rappor_service;

  RecordTileImpression(Builder()
                           .WithVisualType(UNKNOWN_TILE_TYPE)
                           .WithUrl(GURL("http://www.s1.com/"))
                           .Build(),
                       &rappor_service);

  // Unknown tile type shouldn't get reported.
  EXPECT_EQ(0, rappor_service.GetReportsCount());
}

TEST(RecordTileClickTest, ShouldRecordClicksForTileTitleSource) {
  base::HistogramTester histogram_tester;
  RecordTileClick(Builder()
                      .WithSource(TileSource::TOP_SITES)
                      .WithTitleSource(TileTitleSource::UNKNOWN)
                      .Build());
  RecordTileClick(Builder()
                      .WithSource(TileSource::SUGGESTIONS_SERVICE)
                      .WithTitleSource(TileTitleSource::UNKNOWN)
                      .Build());
  RecordTileClick(Builder()
                      .WithSource(TileSource::POPULAR)
                      .WithTitleSource(TileTitleSource::TITLE_TAG)
                      .Build());
  RecordTileClick(Builder()
                      .WithSource(TileSource::POPULAR)
                      .WithTitleSource(TileTitleSource::MANIFEST)
                      .Build());
  RecordTileClick(Builder()
                      .WithSource(TileSource::POPULAR_BAKED_IN)
                      .WithTitleSource(TileTitleSource::TITLE_TAG)
                      .Build());
  RecordTileClick(Builder()
                      .WithSource(TileSource::POPULAR_BAKED_IN)
                      .WithTitleSource(TileTitleSource::META_TAG)
                      .Build());

  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked.client"),
      ElementsAre(base::Bucket(kUnknownTitleSource, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked.server"),
      ElementsAre(base::Bucket(kUnknownTitleSource, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileTitleClicked.popular_fetched"),
              ElementsAre(base::Bucket(kManifestTitleSource, /*count=*/1),
                          base::Bucket(kTitleTagTitleSource, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileTitleClicked.popular_baked_in"),
              ElementsAre(base::Bucket(kMetaTagTitleSource, /*count=*/1),
                          base::Bucket(kTitleTagTitleSource, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.TileTitleClicked"),
              ElementsAre(base::Bucket(kUnknownTitleSource, /*count=*/2),
                          base::Bucket(kManifestTitleSource, /*count=*/1),
                          base::Bucket(kMetaTagTitleSource, /*count=*/1),
                          base::Bucket(kTitleTagTitleSource, /*count=*/2)));
}

TEST(RecordTileClickTest, ShouldRecordClickAge) {
  const base::TimeDelta kSuggestionAge = base::TimeDelta::FromMinutes(1);
  const base::TimeDelta kBucketTolerance = base::TimeDelta::FromSeconds(20);
  base::HistogramTester histogram_tester;
  RecordTileClick(
      Builder()
          .WithSource(TileSource::SUGGESTIONS_SERVICE)
          .WithDataGenerationTime(base::Time::Now() - kSuggestionAge)
          .Build());

  EXPECT_THAT(histogram_tester.GetAllSamples("NewTabPage.MostVisitedAge"),
              ElementsAre(IsBucketBetween(
                  (kSuggestionAge - kBucketTolerance).InSeconds(),
                  (kSuggestionAge + kBucketTolerance).InSeconds(),
                  /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.MostVisitedAge.server"),
      ElementsAre(
          IsBucketBetween((kSuggestionAge - kBucketTolerance).InSeconds(),
                          (kSuggestionAge + kBucketTolerance).InSeconds(),
                          /*count=*/1)));
}

TEST(RecordTileClickTest, ShouldRecordClicksForIconType) {
  base::HistogramTester histogram_tester;

  RecordTileClick(Builder()
                      .WithVisualType(ICON_COLOR)
                      .WithIconType(IconType::kTouchIcon)
                      .Build());
  RecordTileClick(Builder()
                      .WithVisualType(ICON_REAL)
                      .WithIconType(IconType::kWebManifestIcon)
                      .Build());

  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconTypeClicked.IconsColor"),
              ElementsAre(base::Bucket(/*min=*/2, /*count=*/1)));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconTypeClicked.IconsReal"),
              ElementsAre(base::Bucket(/*min=*/4, /*count=*/1)));
  EXPECT_THAT(
      histogram_tester.GetAllSamples("NewTabPage.TileFaviconTypeClicked"),
      ElementsAre(base::Bucket(/*min=*/2, /*count=*/1),
                  base::Bucket(/*min=*/4, /*count=*/1)));
}

}  // namespace
}  // namespace metrics
}  // namespace ntp_tiles

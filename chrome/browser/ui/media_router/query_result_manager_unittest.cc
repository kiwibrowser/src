// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/media_router/query_result_manager.h"

#include "base/bind.h"
#include "base/containers/hash_tables.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/browser/media/router/test/mock_media_router.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::Eq;
using testing::IsEmpty;
using testing::Eq;
using testing::Mock;
using testing::Return;
using testing::_;

namespace media_router {

namespace {

const char kOrigin[] = "https://origin.com";

class MockObserver : public QueryResultManager::Observer {
 public:
  MOCK_METHOD1(OnResultsUpdated,
               void(const std::vector<MediaSinkWithCastModes>& sinks));
};

}  // namespace

class QueryResultManagerTest : public ::testing::Test {
 public:
  QueryResultManagerTest()
      : mock_router_(), query_result_manager_(&mock_router_) {}

  void DiscoverSinks(MediaCastMode cast_mode, const MediaSource& source) {
    EXPECT_CALL(mock_router_, RegisterMediaSinksObserver(_))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_observer_, OnResultsUpdated(_)).Times(1);
    query_result_manager_.SetSourcesForCastMode(
        cast_mode, {source}, url::Origin::Create(GURL(kOrigin)));
  }

  bool IsDefaultSourceForSink(const MediaSource* source,
                              const MediaSink& sink) {
    return IsPreferredSourceForSink(MediaCastMode::PRESENTATION, source, sink);
  }

  bool IsTabSourceForSink(const MediaSource* source, const MediaSink& sink) {
    return IsPreferredSourceForSink(MediaCastMode::TAB_MIRROR, source, sink);
  }

  bool IsPreferredSourceForSink(MediaCastMode cast_mode,
                                const MediaSource* source,
                                const MediaSink& sink) {
    std::unique_ptr<MediaSource> default_source =
        query_result_manager_.GetSourceForCastModeAndSink(cast_mode, sink.id());
    return (!(default_source || source)) ||
           (default_source && source && *default_source.get() == *source);
  }

  content::TestBrowserThreadBundle thread_bundle_;
  MockMediaRouter mock_router_;
  QueryResultManager query_result_manager_;
  MockObserver mock_observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(QueryResultManagerTest);
};

MATCHER_P(VectorEquals, expected, "") {
  if (expected.size() != arg.size()) {
    return false;
  }
  for (size_t i = 0; i < expected.size(); ++i) {
    if (!expected[i].Equals(arg[i])) {
      return false;
    }
  }
  return true;
}

TEST_F(QueryResultManagerTest, Observers) {
  MockObserver ob1;
  MockObserver ob2;
  query_result_manager_.AddObserver(&ob1);
  query_result_manager_.AddObserver(&ob2);

  EXPECT_CALL(ob1, OnResultsUpdated(_)).Times(1);
  EXPECT_CALL(ob2, OnResultsUpdated(_)).Times(1);
  query_result_manager_.NotifyOnResultsUpdated();

  query_result_manager_.RemoveObserver(&ob2);
  EXPECT_CALL(ob1, OnResultsUpdated(_)).Times(1);
  query_result_manager_.NotifyOnResultsUpdated();

  query_result_manager_.RemoveObserver(&ob1);
  query_result_manager_.NotifyOnResultsUpdated();
}

TEST_F(QueryResultManagerTest, StartStopSinksQuery) {
  CastModeSet cast_modes = query_result_manager_.GetSupportedCastModes();
  EXPECT_TRUE(cast_modes.empty());
  std::vector<MediaSource> actual_sources =
      query_result_manager_.GetSourcesForCastMode(MediaCastMode::PRESENTATION);
  EXPECT_EQ(0u, actual_sources.size());

  MediaSource source(MediaSourceForPresentationUrl(GURL("http://foo.com")));
  EXPECT_CALL(mock_router_, RegisterMediaSinksObserver(_))
      .WillOnce(Return(true));
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::PRESENTATION, {source},
      url::Origin::Create(GURL(kOrigin)));

  cast_modes = query_result_manager_.GetSupportedCastModes();
  EXPECT_EQ(1u, cast_modes.size());
  EXPECT_TRUE(base::ContainsKey(cast_modes, MediaCastMode::PRESENTATION));
  actual_sources =
      query_result_manager_.GetSourcesForCastMode(MediaCastMode::PRESENTATION);
  EXPECT_EQ(1u, actual_sources.size());
  EXPECT_EQ(source, actual_sources[0]);

  // Register a different set of sources for the same cast mode.
  MediaSource another_source(
      MediaSourceForPresentationUrl(GURL("http://bar.com")));
  EXPECT_CALL(mock_router_, UnregisterMediaSinksObserver(_)).Times(1);
  EXPECT_CALL(mock_router_, RegisterMediaSinksObserver(_))
      .WillOnce(Return(true));
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::PRESENTATION, {another_source},
      url::Origin::Create(GURL(kOrigin)));

  cast_modes = query_result_manager_.GetSupportedCastModes();
  EXPECT_EQ(1u, cast_modes.size());
  EXPECT_TRUE(base::ContainsKey(cast_modes, MediaCastMode::PRESENTATION));
  actual_sources =
      query_result_manager_.GetSourcesForCastMode(MediaCastMode::PRESENTATION);
  EXPECT_EQ(1u, actual_sources.size());
  EXPECT_EQ(another_source, actual_sources[0]);

  EXPECT_CALL(mock_router_, UnregisterMediaSinksObserver(_)).Times(1);
  query_result_manager_.RemoveSourcesForCastMode(MediaCastMode::PRESENTATION);

  cast_modes = query_result_manager_.GetSupportedCastModes();
  EXPECT_TRUE(cast_modes.empty());
  actual_sources =
      query_result_manager_.GetSourcesForCastMode(MediaCastMode::PRESENTATION);
  EXPECT_EQ(0u, actual_sources.size());
}

TEST_F(QueryResultManagerTest, MultipleQueries) {
  MediaSink sink1("sinkId1", "Sink 1", SinkIconType::CAST);
  MediaSink sink2("sinkId2", "Sink 2", SinkIconType::CAST);
  MediaSink sink3("sinkId3", "Sink 3", SinkIconType::CAST);
  MediaSink sink4("sinkId4", "Sink 4", SinkIconType::CAST);
  MediaSink sink5("sinkId5", "Sink 5", SinkIconType::CAST);
  MediaSource presentation_source1 =
      MediaSourceForPresentationUrl(GURL("http://bar.com"));
  MediaSource presentation_source2 =
      MediaSourceForPresentationUrl(GURL("http://baz.com"));
  MediaSource tab_source = MediaSourceForTab(123);

  query_result_manager_.AddObserver(&mock_observer_);
  DiscoverSinks(MediaCastMode::PRESENTATION, presentation_source1);
  DiscoverSinks(MediaCastMode::TAB_MIRROR, tab_source);

  // Scenario (results in this order):
  // Action: PRESENTATION -> [1, 2, 3]
  // Expected result:
  // Sinks: [1 -> {PRESENTATION}, 2 -> {PRESENTATION}, 3 -> {PRESENTATION}]
  std::vector<MediaSinkWithCastModes> expected_sinks;
  expected_sinks.push_back(MediaSinkWithCastModes(sink1));
  expected_sinks.back().cast_modes.insert(MediaCastMode::PRESENTATION);
  expected_sinks.push_back(MediaSinkWithCastModes(sink2));
  expected_sinks.back().cast_modes.insert(MediaCastMode::PRESENTATION);
  expected_sinks.push_back(MediaSinkWithCastModes(sink3));
  expected_sinks.back().cast_modes.insert(MediaCastMode::PRESENTATION);

  const auto& sinks_observers = query_result_manager_.sinks_observers_;
  auto sinks_observer_it = sinks_observers.find(presentation_source1);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());

  std::vector<MediaSink> sinks_query_result;
  sinks_query_result.push_back(sink1);
  sinks_query_result.push_back(sink2);
  sinks_query_result.push_back(sink3);
  EXPECT_CALL(mock_observer_, OnResultsUpdated(VectorEquals(expected_sinks)))
      .Times(1);
  sinks_observer_it->second->OnSinksUpdated(sinks_query_result,
                                            std::vector<url::Origin>());

  // Action: TAB_MIRROR -> [2, 3, 4]
  // Expected result:
  // Sinks: [1 -> {PRESENTATION}, 2 -> {PRESENTATION, TAB_MIRROR},
  //         3 -> {PRESENTATION, TAB_MIRROR}, 4 -> {TAB_MIRROR}]
  expected_sinks.clear();
  expected_sinks.push_back(MediaSinkWithCastModes(sink1));
  expected_sinks.back().cast_modes.insert(MediaCastMode::PRESENTATION);
  expected_sinks.push_back(MediaSinkWithCastModes(sink2));
  expected_sinks.back().cast_modes.insert(MediaCastMode::PRESENTATION);
  expected_sinks.back().cast_modes.insert(MediaCastMode::TAB_MIRROR);
  expected_sinks.push_back(MediaSinkWithCastModes(sink3));
  expected_sinks.back().cast_modes.insert(MediaCastMode::PRESENTATION);
  expected_sinks.back().cast_modes.insert(MediaCastMode::TAB_MIRROR);
  expected_sinks.push_back(MediaSinkWithCastModes(sink4));
  expected_sinks.back().cast_modes.insert(MediaCastMode::TAB_MIRROR);

  sinks_query_result.clear();
  sinks_query_result.push_back(sink2);
  sinks_query_result.push_back(sink3);
  sinks_query_result.push_back(sink4);

  sinks_observer_it = sinks_observers.find(tab_source);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());
  EXPECT_CALL(mock_observer_, OnResultsUpdated(VectorEquals(expected_sinks)))
      .Times(1);
  sinks_observer_it->second->OnSinksUpdated(
      sinks_query_result, {url::Origin::Create(GURL(kOrigin))});

  // Action: Update presentation URL
  // Expected result:
  // Sinks: [2 -> {TAB_MIRROR}, 3 -> {TAB_MIRROR}, 4 -> {TAB_MIRROR}]
  expected_sinks.clear();
  expected_sinks.push_back(MediaSinkWithCastModes(sink2));
  expected_sinks.back().cast_modes.insert(MediaCastMode::TAB_MIRROR);
  expected_sinks.push_back(MediaSinkWithCastModes(sink3));
  expected_sinks.back().cast_modes.insert(MediaCastMode::TAB_MIRROR);
  expected_sinks.push_back(MediaSinkWithCastModes(sink4));
  expected_sinks.back().cast_modes.insert(MediaCastMode::TAB_MIRROR);

  // The observer for the old source will be unregistered.
  EXPECT_CALL(mock_router_, UnregisterMediaSinksObserver(_)).Times(1);
  // The observer for the new source will be registered.
  EXPECT_CALL(mock_router_, RegisterMediaSinksObserver(_))
      .WillOnce(Return(true));
  EXPECT_CALL(mock_observer_, OnResultsUpdated(VectorEquals(expected_sinks)))
      .Times(1);
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::PRESENTATION, {presentation_source2},
      url::Origin::Create(GURL(kOrigin)));

  // Action: PRESENTATION -> [1], origins don't match
  // Expected result: [2 -> {TAB_MIRROR}, 3 -> {TAB_MIRROR}, 4 -> {TAB_MIRROR}]
  // (No change)
  sinks_query_result.clear();
  sinks_query_result.push_back(sink1);
  sinks_observer_it = sinks_observers.find(presentation_source2);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());
  EXPECT_CALL(mock_observer_, OnResultsUpdated(VectorEquals(expected_sinks)))
      .Times(1);
  sinks_observer_it->second->OnSinksUpdated(
      sinks_query_result,
      {url::Origin::Create(GURL("https://differentOrigin.com"))});

  // Action: Remove TAB_MIRROR observer
  // Expected result:
  // Sinks: []
  expected_sinks.clear();
  EXPECT_CALL(mock_observer_, OnResultsUpdated(VectorEquals(expected_sinks)))
      .Times(1);
  EXPECT_CALL(mock_router_, UnregisterMediaSinksObserver(_)).Times(1);
  query_result_manager_.RemoveSourcesForCastMode(MediaCastMode::TAB_MIRROR);

  // Remaining observers: PRESENTATION observer, which will be removed on
  // destruction
  EXPECT_CALL(mock_router_, UnregisterMediaSinksObserver(_)).Times(1);
}

TEST_F(QueryResultManagerTest, MultipleUrls) {
  const MediaSink sink1("sinkId1", "Sink 1", SinkIconType::CAST);
  const MediaSink sink2("sinkId2", "Sink 2", SinkIconType::CAST);
  const MediaSink sink3("sinkId3", "Sink 3", SinkIconType::CAST);
  const MediaSink sink4("sinkId4", "Sink 4", SinkIconType::CAST);
  const MediaSource source_a(
      MediaSourceForPresentationUrl(GURL("http://urlA.com")));
  const MediaSource source_b(
      MediaSourceForPresentationUrl(GURL("http://urlB.com")));
  const MediaSource source_c(
      MediaSourceForPresentationUrl(GURL("http://urlC.com")));
  const MediaSource source_tab(MediaSourceForTab(1));
  // The sources are in decreasing order of priority.
  const std::vector<MediaSource> presentation_sources = {source_a, source_b,
                                                         source_c};
  const std::vector<MediaSource> tab_sources = {source_tab};
  const auto& sinks_observers = query_result_manager_.sinks_observers_;

  // There should be one MediaSinksObserver per source.
  EXPECT_CALL(mock_router_, RegisterMediaSinksObserver(_))
      .Times(4)
      .WillRepeatedly(Return(true));
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::PRESENTATION, presentation_sources,
      url::Origin::Create(GURL(kOrigin)));
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::TAB_MIRROR, tab_sources,
      url::Origin::Create(GURL(kOrigin)));

  // Scenario (results in this order):
  // Action: URL_B -> [2, 4]
  // Expected result:
  // Sinks: [1 -> {},
  //         2 -> {URL_B},
  //         3 -> {},
  //         4 -> {URL_B}]
  auto sinks_observer_it = sinks_observers.find(source_b);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());

  auto& source_b_observer = sinks_observer_it->second;
  source_b_observer->OnSinksUpdated({sink2, sink4}, std::vector<url::Origin>());
  EXPECT_TRUE(IsDefaultSourceForSink(nullptr, sink1));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_b, sink2));
  EXPECT_TRUE(IsDefaultSourceForSink(nullptr, sink3));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_b, sink4));

  // Action: URL_C -> [1, 2, 3]
  // Expected result:
  // Sinks: [1 -> {URL_C},
  //         2 -> {URL_B, URL_C},
  //         3 -> {URL_C},
  //         4 -> {URL_B}]
  sinks_observer_it = sinks_observers.find(source_c);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());

  auto& source_c_observer = sinks_observer_it->second;
  source_c_observer->OnSinksUpdated({sink1, sink2, sink3},
                                    std::vector<url::Origin>());
  EXPECT_TRUE(IsDefaultSourceForSink(&source_c, sink1));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_b, sink2));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_c, sink3));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_b, sink4));

  // Action: URL_A -> [2, 3, 4]
  // Expected result:
  // Sinks: [1 -> {URL_C},
  //         2 -> {URL_A, URL_B, URL_C},
  //         3 -> {URL_A, URL_C},
  //         4 -> {URL_A, URL_B}]
  sinks_observer_it = sinks_observers.find(source_a);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());

  auto& source_a_observer = sinks_observer_it->second;
  source_a_observer->OnSinksUpdated({sink2, sink3, sink4},
                                    std::vector<url::Origin>());
  EXPECT_TRUE(IsDefaultSourceForSink(&source_c, sink1));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_a, sink2));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_a, sink3));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_a, sink4));

  // Action: TAB -> [1, 2]
  // Expected result:
  // Sinks: [1 -> {URL_C, TAB},
  //         2 -> {URL_A, URL_B, URL_C, TAB},
  //         3 -> {URL_A, URL_C},
  //         4 -> {URL_A, URL_B}]
  sinks_observer_it = sinks_observers.find(source_tab);
  ASSERT_TRUE(sinks_observer_it != sinks_observers.end());
  ASSERT_TRUE(sinks_observer_it->second.get());

  auto& source_tab_observer = sinks_observer_it->second;
  source_tab_observer->OnSinksUpdated({sink1, sink2},
                                      std::vector<url::Origin>());
  EXPECT_TRUE(IsDefaultSourceForSink(&source_c, sink1));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_a, sink2));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_a, sink3));
  EXPECT_TRUE(IsDefaultSourceForSink(&source_a, sink4));
  EXPECT_TRUE(IsTabSourceForSink(&source_tab, sink1));
  EXPECT_TRUE(IsTabSourceForSink(&source_tab, sink2));
  EXPECT_TRUE(IsTabSourceForSink(nullptr, sink3));
  EXPECT_TRUE(IsTabSourceForSink(nullptr, sink4));

  // The observers for the four sources should get unregistered.
  EXPECT_CALL(mock_router_, UnregisterMediaSinksObserver(_)).Times(4);
  query_result_manager_.RemoveSourcesForCastMode(MediaCastMode::PRESENTATION);
  query_result_manager_.RemoveSourcesForCastMode(MediaCastMode::TAB_MIRROR);
}

TEST_F(QueryResultManagerTest, AddInvalidSource) {
  const MediaSource source(
      MediaSourceForPresentationUrl(GURL("http://url.com")));

  EXPECT_CALL(mock_router_, RegisterMediaSinksObserver(_))
      .Times(1)
      .WillRepeatedly(Return(true));
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::PRESENTATION, {source},
      url::Origin::Create(GURL(kOrigin)));
  // |source| has already been registered with the PRESENTATION cast mode, so it
  // shouldn't get registered with tab mirroring.
  query_result_manager_.SetSourcesForCastMode(
      MediaCastMode::TAB_MIRROR, {source}, url::Origin::Create(GURL(kOrigin)));

  const auto& cast_mode_sources = query_result_manager_.cast_mode_sources_;
  const auto& presentation_sources =
      cast_mode_sources.at(MediaCastMode::PRESENTATION);
  EXPECT_TRUE(
      base::ContainsKey(cast_mode_sources, MediaCastMode::PRESENTATION));
  EXPECT_EQ(presentation_sources.size(), 1u);
  EXPECT_EQ(presentation_sources.at(0), source);
  EXPECT_FALSE(base::ContainsKey(cast_mode_sources, MediaCastMode::TAB_MIRROR));
}

}  // namespace media_router

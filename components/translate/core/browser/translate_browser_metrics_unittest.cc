// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/translate/core/browser/translate_browser_metrics.h"

#include <memory>

#include "base/macros.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/statistics_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::HistogramBase;
using base::HistogramSamples;
using base::StatisticsRecorder;

namespace {

class MetricsRecorder {
 public:
  explicit MetricsRecorder(const char* key) : key_(key) {
    HistogramBase* histogram = StatisticsRecorder::FindHistogram(key_);
    if (histogram)
      base_samples_ = histogram->SnapshotSamples();
  }

  void CheckInitiationStatus(
      int expected_disabled_by_prefs,
      int expected_disabled_by_config,
      int expected_disabled_by_build,
      int expected_language_is_not_supported,
      int expected_mime_type_is_not_supported,
      int expected_url_is_not_supported,
      int expected_similar_languages,
      int expected_accept_languages,
      int expected_auto_by_config,
      int expected_auto_by_link,
      int expected_show_infobar,
      int expected_language_in_ulp,
      int expected_aborted_by_ranker,
      int expected_aborted_by_too_often_denied,
      int expected_aborted_by_matches_previous_language) {
    Snapshot();

    EXPECT_EQ(expected_disabled_by_prefs,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_DISABLED_BY_PREFS));
    EXPECT_EQ(
        expected_disabled_by_config,
        GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                    INITIATION_STATUS_DISABLED_BY_CONFIG));
    EXPECT_EQ(
        expected_disabled_by_build,
        GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                    INITIATION_STATUS_DISABLED_BY_KEY));
    EXPECT_EQ(expected_language_is_not_supported,
              GetCountWithoutSnapshot(
                  translate::TranslateBrowserMetrics::
                      INITIATION_STATUS_LANGUAGE_IS_NOT_SUPPORTED));
    EXPECT_EQ(expected_mime_type_is_not_supported,
              GetCountWithoutSnapshot(
                  translate::TranslateBrowserMetrics::
                      INITIATION_STATUS_MIME_TYPE_IS_NOT_SUPPORTED));
    EXPECT_EQ(
        expected_url_is_not_supported,
        GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                    INITIATION_STATUS_URL_IS_NOT_SUPPORTED));
    EXPECT_EQ(expected_similar_languages,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_SIMILAR_LANGUAGES));
    EXPECT_EQ(expected_accept_languages,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_ACCEPT_LANGUAGES));
    EXPECT_EQ(expected_auto_by_config,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_AUTO_BY_CONFIG));
    EXPECT_EQ(expected_auto_by_link,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_AUTO_BY_LINK));
    EXPECT_EQ(expected_show_infobar,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_SHOW_INFOBAR));
    EXPECT_EQ(expected_language_in_ulp,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_LANGUAGE_IN_ULP));
    EXPECT_EQ(expected_aborted_by_ranker,
              GetCountWithoutSnapshot(translate::TranslateBrowserMetrics::
                                          INITIATION_STATUS_ABORTED_BY_RANKER));
    EXPECT_EQ(expected_aborted_by_too_often_denied,
              GetCountWithoutSnapshot(
                  translate::TranslateBrowserMetrics::
                      INITIATION_STATUS_ABORTED_BY_TOO_OFTEN_DENIED));
    EXPECT_EQ(expected_aborted_by_matches_previous_language,
              GetCountWithoutSnapshot(
                  translate::TranslateBrowserMetrics::
                      INITIATION_STATUS_ABORTED_BY_MATCHES_PREVIOUS_LANGUAGE));
  }

  HistogramBase::Count GetTotalCount() {
    Snapshot();
    if (!samples_.get())
      return 0;
    HistogramBase::Count count = samples_->TotalCount();
    if (!base_samples_.get())
      return count;
    return count - base_samples_->TotalCount();
  }

  HistogramBase::Count GetCount(HistogramBase::Sample value) {
    Snapshot();
    return GetCountWithoutSnapshot(value);
  }

 private:
  void Snapshot() {
    HistogramBase* histogram = StatisticsRecorder::FindHistogram(key_);
    if (!histogram)
      return;
    samples_ = histogram->SnapshotSamples();
  }

  HistogramBase::Count GetCountWithoutSnapshot(HistogramBase::Sample value) {
    if (!samples_.get())
      return 0;
    HistogramBase::Count count = samples_->GetCount(value);
    if (!base_samples_.get())
      return count;
    return count - base_samples_->GetCount(value);
  }

  std::string key_;
  std::unique_ptr<HistogramSamples> base_samples_;
  std::unique_ptr<HistogramSamples> samples_;

  DISALLOW_COPY_AND_ASSIGN(MetricsRecorder);
};

}  // namespace

TEST(TranslateBrowserMetricsTest, ReportInitiationStatus) {
  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::UMA_INITIATION_STATUS));

  recorder.CheckInitiationStatus(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_DISABLED_BY_PREFS);
  recorder.CheckInitiationStatus(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_DISABLED_BY_CONFIG);
  recorder.CheckInitiationStatus(1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_DISABLED_BY_KEY);
  recorder.CheckInitiationStatus(1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::
          INITIATION_STATUS_LANGUAGE_IS_NOT_SUPPORTED);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::
          INITIATION_STATUS_MIME_TYPE_IS_NOT_SUPPORTED);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::
          INITIATION_STATUS_URL_IS_NOT_SUPPORTED);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_SIMILAR_LANGUAGES);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_ACCEPT_LANGUAGES);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_AUTO_BY_CONFIG);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_AUTO_BY_LINK);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_SHOW_INFOBAR);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_LANGUAGE_IN_ULP);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::INITIATION_STATUS_ABORTED_BY_RANKER);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::
          INITIATION_STATUS_ABORTED_BY_TOO_OFTEN_DENIED);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0);
  translate::TranslateBrowserMetrics::ReportInitiationStatus(
      translate::TranslateBrowserMetrics::
          INITIATION_STATUS_ABORTED_BY_MATCHES_PREVIOUS_LANGUAGE);
  recorder.CheckInitiationStatus(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
}

TEST(TranslateBrowserMetricsTest, ReportLanguageDetectionError) {
  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::UMA_LANGUAGE_DETECTION_ERROR));
  EXPECT_EQ(0, recorder.GetTotalCount());
  translate::TranslateBrowserMetrics::ReportLanguageDetectionError();
  EXPECT_EQ(1, recorder.GetTotalCount());
}


TEST(TranslateBrowserMetricsTest, ReportedLocalesOnDisabledByPrefs) {
  const int ENGLISH = 25966;

  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::UMA_LOCALES_ON_DISABLED_BY_PREFS));
  EXPECT_EQ(0, recorder.GetTotalCount());
  translate::TranslateBrowserMetrics::ReportLocalesOnDisabledByPrefs("en");
  EXPECT_EQ(1, recorder.GetCount(ENGLISH));
}

TEST(TranslateBrowserMetricsTest, ReportedUndisplayableLanguage) {
  const int ENGLISH = 25966;

  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::UMA_UNDISPLAYABLE_LANGUAGE));
  EXPECT_EQ(0, recorder.GetTotalCount());
  translate::TranslateBrowserMetrics::ReportUndisplayableLanguage("en");
  EXPECT_EQ(1, recorder.GetCount(ENGLISH));
}

TEST(TranslateBrowserMetricsTest, ReportedUnsupportedLanguageAtInitiation) {
  const int ENGLISH = 25966;

  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::
          UMA_UNSUPPORTED_LANGUAGE_AT_INITIATION));
  EXPECT_EQ(0, recorder.GetTotalCount());
  translate::TranslateBrowserMetrics::ReportUnsupportedLanguageAtInitiation(
      "en");
  EXPECT_EQ(1, recorder.GetCount(ENGLISH));
}

TEST(TranslateBrowserMetricsTest, ReportedTranslateSourceLanguage) {
  const int ENGLISH = -74147910;
  const int FRENCH = 1704315002;

  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::UMA_TRANSLATE_SOURCE_LANGUAGE));
  EXPECT_EQ(0, recorder.GetTotalCount());

  translate::TranslateBrowserMetrics::ReportTranslateSourceLanguage("en");
  translate::TranslateBrowserMetrics::ReportTranslateSourceLanguage("fr");
  translate::TranslateBrowserMetrics::ReportTranslateSourceLanguage("en");

  EXPECT_EQ(2, recorder.GetCount(ENGLISH));
  EXPECT_EQ(1, recorder.GetCount(FRENCH));
}

TEST(TranslateBrowserMetricsTest, ReportedTranslateTargetLanguage) {
  const int ENGLISH = -74147910;
  const int FRENCH = 1704315002;

  MetricsRecorder recorder(translate::TranslateBrowserMetrics::GetMetricsName(
      translate::TranslateBrowserMetrics::UMA_TRANSLATE_TARGET_LANGUAGE));
  EXPECT_EQ(0, recorder.GetTotalCount());

  translate::TranslateBrowserMetrics::ReportTranslateTargetLanguage("en");
  translate::TranslateBrowserMetrics::ReportTranslateTargetLanguage("fr");
  translate::TranslateBrowserMetrics::ReportTranslateTargetLanguage("en");

  EXPECT_EQ(2, recorder.GetCount(ENGLISH));
  EXPECT_EQ(1, recorder.GetCount(FRENCH));
}

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ukm/ukm_service.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/hash.h"
#include "base/metrics/metrics_hashes.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/metrics/test_metrics_provider.h"
#include "components/metrics/test_metrics_service_client.h"
#include "components/prefs/testing_pref_service.h"
#include "components/ukm/persisted_logs_metrics_impl.h"
#include "components/ukm/ukm_pref_names.h"
#include "components/ukm/ukm_source.h"
#include "components/variations/variations_associated_data.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/metrics_proto/ukm/report.pb.h"
#include "third_party/metrics_proto/ukm/source.pb.h"
#include "third_party/zlib/google/compression_utils.h"

namespace ukm {

// Some arbitrary events used in tests.
using TestEvent1 = ukm::builders::PageLoad;
const char* kTestEvent1Metric1 =
    TestEvent1::kPaintTiming_NavigationToFirstContentfulPaintName;
const char* kTestEvent1Metric2 = TestEvent1::kNet_CacheBytesName;
using TestEvent2 = ukm::builders::Memory_Experimental;
const char* kTestEvent2Metric1 = TestEvent2::kArrayBufferName;
const char* kTestEvent2Metric2 = TestEvent2::kBlinkGCName;
using TestEvent3 = ukm::builders::Previews;

std::string Entry1And2Whitelist() {
  return std::string(TestEvent1::kEntryName) + ',' + TestEvent2::kEntryName;
}

// A small shim exposing UkmRecorder methods to tests.
class TestRecordingHelper {
 public:
  explicit TestRecordingHelper(UkmRecorder* recorder) : recorder_(recorder) {}

  void UpdateSourceURL(SourceId source_id, const GURL& url) {
    recorder_->UpdateSourceURL(source_id, url);
  }

 private:
  UkmRecorder* recorder_;

  DISALLOW_COPY_AND_ASSIGN(TestRecordingHelper);
};

namespace {

bool TestIsWebstoreExtension(base::StringPiece id) {
  return (id == "bhcnanendmgjjeghamaccjnochlnhcgj");
}

// TODO(rkaplow): consider making this a generic testing class in
// components/variations.
class ScopedUkmFeatureParams {
 public:
  ScopedUkmFeatureParams(
      base::FeatureList::OverrideState feature_state,
      const std::map<std::string, std::string>& variation_params) {
    static const char kTestFieldTrialName[] = "TestTrial";
    static const char kTestExperimentGroupName[] = "TestGroup";

    variations::testing::ClearAllVariationParams();

    EXPECT_TRUE(variations::AssociateVariationParams(
        kTestFieldTrialName, kTestExperimentGroupName, variation_params));

    base::FieldTrial* field_trial = base::FieldTrialList::CreateFieldTrial(
        kTestFieldTrialName, kTestExperimentGroupName);

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(kUkmFeature.name, feature_state,
                                             field_trial);

    // Since we are adding a scoped feature list after browser start, copy over
    // the existing feature list to prevent inconsistency.
    base::FeatureList* existing_feature_list = base::FeatureList::GetInstance();
    if (existing_feature_list) {
      std::string enabled_features;
      std::string disabled_features;
      base::FeatureList::GetInstance()->GetFeatureOverrides(&enabled_features,
                                                            &disabled_features);
      feature_list->InitializeFromCommandLine(enabled_features,
                                              disabled_features);
    }

    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

  ~ScopedUkmFeatureParams() { variations::testing::ClearAllVariationParams(); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ScopedUkmFeatureParams);
};

class UkmServiceTest : public testing::Test {
 public:
  UkmServiceTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        task_runner_handle_(task_runner_) {
    UkmService::RegisterPrefs(prefs_.registry());
    ClearPrefs();
  }

  void ClearPrefs() {
    prefs_.ClearPref(prefs::kUkmClientId);
    prefs_.ClearPref(prefs::kUkmSessionId);
    prefs_.ClearPref(prefs::kUkmPersistedLogs);
  }

  int GetPersistedLogCount() {
    const base::ListValue* list_value =
        prefs_.GetList(prefs::kUkmPersistedLogs);
    return list_value->GetSize();
  }

  Report GetPersistedReport() {
    EXPECT_GE(GetPersistedLogCount(), 1);
    metrics::PersistedLogs result_persisted_logs(
        std::make_unique<ukm::PersistedLogsMetricsImpl>(), &prefs_,
        prefs::kUkmPersistedLogs,
        3,     // log count limit
        1000,  // byte limit
        0);

    result_persisted_logs.LoadPersistedUnsentLogs();
    result_persisted_logs.StageNextLog();

    std::string uncompressed_log_data;
    EXPECT_TRUE(compression::GzipUncompress(result_persisted_logs.staged_log(),
                                            &uncompressed_log_data));

    Report report;
    EXPECT_TRUE(report.ParseFromString(uncompressed_log_data));
    return report;
  }

  static SourceId GetWhitelistedSourceId(int64_t id) {
    return ConvertToSourceId(id, SourceIdType::NAVIGATION_ID);
  }

  static SourceId GetNonWhitelistedSourceId(int64_t id) {
    return ConvertToSourceId(id, SourceIdType::UKM);
  }

 protected:
  TestingPrefServiceSimple prefs_;
  metrics::TestMetricsServiceClient client_;

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UkmServiceTest);
};

}  // namespace

TEST_F(UkmServiceTest, EnableDisableSchedule) {
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  EXPECT_FALSE(task_runner_->HasPendingTask());
  service.Initialize();
  EXPECT_FALSE(task_runner_->HasPendingTask());
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();
  EXPECT_TRUE(task_runner_->HasPendingTask());
  service.DisableReporting();
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(task_runner_->HasPendingTask());
}

TEST_F(UkmServiceTest, PersistAndPurge) {
  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  SourceId id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
  // Should init, generate a log, and start an upload for source.
  task_runner_->RunPendingTasks();
  EXPECT_TRUE(client_.uploader()->is_uploading());
  // Flushes the generated log to disk and generates a new entry.
  TestEvent1(id).Record(&service);
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 2);
  service.Purge();
  EXPECT_EQ(GetPersistedLogCount(), 0);
}

TEST_F(UkmServiceTest, SourceSerialization) {
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  ukm::SourceId id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/initial"));
  recorder.UpdateSourceURL(id, GURL("https://google.com/intermediate"));
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));

  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 1);

  Report proto_report = GetPersistedReport();
  EXPECT_EQ(1, proto_report.sources_size());
  EXPECT_TRUE(proto_report.has_session_id());
  const Source& proto_source = proto_report.sources(0);

  EXPECT_EQ(id, proto_source.id());
  EXPECT_EQ(GURL("https://google.com/foobar").spec(), proto_source.url());
  EXPECT_FALSE(proto_source.has_initial_url());
}

TEST_F(UkmServiceTest, AddEntryWithEmptyMetrics) {
  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  ASSERT_EQ(0, GetPersistedLogCount());
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  ukm::SourceId id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));

  TestEvent1(id).Record(&service);
  service.Flush();
  ASSERT_EQ(1, GetPersistedLogCount());
  Report proto_report = GetPersistedReport();
  EXPECT_EQ(1, proto_report.entries_size());
}

TEST_F(UkmServiceTest, MetricsProviderTest) {
  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);

  metrics::TestMetricsProvider* provider = new metrics::TestMetricsProvider();
  service.RegisterMetricsProvider(
      std::unique_ptr<metrics::MetricsProvider>(provider));

  service.Initialize();

  // Providers have not supplied system profile information yet.
  EXPECT_FALSE(provider->provide_system_profile_metrics_called());

  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  ukm::SourceId id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
  TestEvent1(id).Record(&service);
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 1);

  Report proto_report = GetPersistedReport();
  EXPECT_EQ(1, proto_report.sources_size());
  EXPECT_EQ(1, proto_report.entries_size());

  // Providers have now supplied system profile information.
  EXPECT_TRUE(provider->provide_system_profile_metrics_called());
}

TEST_F(UkmServiceTest, LogsRotation) {
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  EXPECT_EQ(0, service.report_count());

  // Log rotation should generate a log.
  const ukm::SourceId id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
  task_runner_->RunPendingTasks();
  EXPECT_EQ(1, service.report_count());
  EXPECT_TRUE(client_.uploader()->is_uploading());

  // Rotation shouldn't generate a log due to one being pending.
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
  task_runner_->RunPendingTasks();
  EXPECT_EQ(1, service.report_count());
  EXPECT_TRUE(client_.uploader()->is_uploading());

  // Completing the upload should clear pending log, then log rotation should
  // generate another log.
  client_.uploader()->CompleteUpload(200);
  task_runner_->RunPendingTasks();
  EXPECT_EQ(2, service.report_count());

  // Check that rotations keep working.
  for (int i = 3; i < 6; i++) {
    task_runner_->RunPendingTasks();
    client_.uploader()->CompleteUpload(200);
    recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
    task_runner_->RunPendingTasks();
    EXPECT_EQ(i, service.report_count());
  }
}

TEST_F(UkmServiceTest, LogsUploadedOnlyWhenHavingSourcesOrEntries) {
  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  // Testing two whitelisted Entries.
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  EXPECT_TRUE(task_runner_->HasPendingTask());
  // Neither rotation or Flush should generate logs
  task_runner_->RunPendingTasks();
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 0);

  ukm::SourceId id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
  // Includes a Source, so will persist.
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 1);

  TestEvent1(id).Record(&service);
  // Includes an Entry, so will persist.
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 2);

  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));
  TestEvent1(id).Record(&service);
  // Includes a Source and an Entry, so will persist.
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 3);

  // Current log has no Sources.
  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 3);
}

TEST_F(UkmServiceTest, GetNewSourceID) {
  ukm::SourceId id1 = UkmRecorder::GetNewSourceID();
  ukm::SourceId id2 = UkmRecorder::GetNewSourceID();
  ukm::SourceId id3 = UkmRecorder::GetNewSourceID();
  EXPECT_NE(id1, id2);
  EXPECT_NE(id1, id3);
  EXPECT_NE(id2, id3);
}

TEST_F(UkmServiceTest, RecordInitialUrl) {
  for (bool should_record_initial_url : {true, false}) {
    base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
    ScopedUkmFeatureParams params(
        base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        {{"RecordInitialUrl", should_record_initial_url ? "true" : "false"}});

    ClearPrefs();
    UkmService service(&prefs_, &client_,
                       true /* restrict_to_whitelisted_entries */);
    TestRecordingHelper recorder(&service);
    EXPECT_EQ(GetPersistedLogCount(), 0);
    service.Initialize();
    task_runner_->RunUntilIdle();
    service.EnableRecording(/*extensions=*/false);
    service.EnableReporting();

    ukm::SourceId id = GetWhitelistedSourceId(0);
    recorder.UpdateSourceURL(id, GURL("https://google.com/initial"));
    recorder.UpdateSourceURL(id, GURL("https://google.com/intermediate"));
    recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));

    service.Flush();
    EXPECT_EQ(GetPersistedLogCount(), 1);

    Report proto_report = GetPersistedReport();
    EXPECT_EQ(1, proto_report.sources_size());
    const Source& proto_source = proto_report.sources(0);

    EXPECT_EQ(id, proto_source.id());
    EXPECT_EQ(GURL("https://google.com/foobar").spec(), proto_source.url());
    EXPECT_EQ(should_record_initial_url, proto_source.has_initial_url());
    if (should_record_initial_url) {
      EXPECT_EQ(GURL("https://google.com/initial").spec(),
                proto_source.initial_url());
    }
  }
}

TEST_F(UkmServiceTest, RestrictToWhitelistedSourceIds) {
  const GURL kURL = GURL("https://example.com/");
  for (bool restrict_to_whitelisted_source_ids : {true, false}) {
    base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
    ScopedUkmFeatureParams params(
        base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        {{"RestrictToWhitelistedSourceIds",
          restrict_to_whitelisted_source_ids ? "true" : "false"},
         {"WhitelistEntries", Entry1And2Whitelist()}});

    ClearPrefs();
    UkmService service(&prefs_, &client_,
                       true /* restrict_to_whitelisted_entries */);
    TestRecordingHelper recorder(&service);
    EXPECT_EQ(GetPersistedLogCount(), 0);
    service.Initialize();
    task_runner_->RunUntilIdle();
    service.EnableRecording(/*extensions=*/false);
    service.EnableReporting();

    ukm::SourceId id1 = GetWhitelistedSourceId(0);
    recorder.UpdateSourceURL(id1, kURL);
    TestEvent1(id1).Record(&service);

    // Create a non-navigation-based sourceid, which should not be whitelisted.
    ukm::SourceId id2 = GetNonWhitelistedSourceId(1);
    recorder.UpdateSourceURL(id2, kURL);
    TestEvent1(id2).Record(&service);

    service.Flush();
    ASSERT_EQ(GetPersistedLogCount(), 1);
    Report proto_report = GetPersistedReport();
    ASSERT_GE(proto_report.sources_size(), 1);

    // The whitelisted source should always be recorded.
    const Source& proto_source1 = proto_report.sources(0);
    EXPECT_EQ(id1, proto_source1.id());
    EXPECT_EQ(kURL.spec(), proto_source1.url());

    // The non-whitelisted source should only be recorded if we aren't
    // restricted to whitelisted source ids.
    if (restrict_to_whitelisted_source_ids) {
      ASSERT_EQ(1, proto_report.sources_size());
    } else {
      ASSERT_EQ(2, proto_report.sources_size());
      const Source& proto_source2 = proto_report.sources(1);
      EXPECT_EQ(id2, proto_source2.id());
      EXPECT_EQ(kURL.spec(), proto_source2.url());
    }
  }
}

TEST_F(UkmServiceTest, RecordSessionId) {
  ClearPrefs();
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(0, GetPersistedLogCount());
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  auto id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar"));

  service.Flush();
  EXPECT_EQ(1, GetPersistedLogCount());

  auto proto_report = GetPersistedReport();
  EXPECT_TRUE(proto_report.has_session_id());
  EXPECT_EQ(1, proto_report.report_id());
}

TEST_F(UkmServiceTest, SourceSize) {
  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  // Set a threshold of number of Sources via Feature Params.
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"MaxSources", "2"}});

  ClearPrefs();
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(0, GetPersistedLogCount());
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  auto id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar1"));
  id = GetWhitelistedSourceId(1);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar2"));
  id = GetWhitelistedSourceId(2);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar3"));

  service.Flush();
  EXPECT_EQ(1, GetPersistedLogCount());

  auto proto_report = GetPersistedReport();
  // Note, 2 instead of 3 sources, since we overrode the max number of sources
  // via Feature params.
  EXPECT_EQ(2, proto_report.sources_size());
}

TEST_F(UkmServiceTest, PurgeMidUpload) {
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();
  auto id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar1"));
  // Should init, generate a log, and start an upload.
  task_runner_->RunPendingTasks();
  EXPECT_TRUE(client_.uploader()->is_uploading());
  // Purge should delete all logs, including the one being sent.
  service.Purge();
  // Upload succeeds after logs was deleted.
  client_.uploader()->CompleteUpload(200);
  EXPECT_EQ(GetPersistedLogCount(), 0);
  EXPECT_FALSE(client_.uploader()->is_uploading());
}

TEST_F(UkmServiceTest, WhitelistEntryTest) {
  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  // Testing two whitelisted Entries.
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  ClearPrefs();
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(0, GetPersistedLogCount());
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  auto id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://google.com/foobar1"));

  TestEvent1(id).Record(&service);
  TestEvent2(id).Record(&service);
  // Note that this third entry is not in the whitelist.
  TestEvent3(id).Record(&service);

  service.Flush();
  EXPECT_EQ(1, GetPersistedLogCount());
  Report proto_report = GetPersistedReport();

  // Verify we've added one source and 2 entries.
  EXPECT_EQ(1, proto_report.sources_size());
  ASSERT_EQ(2, proto_report.entries_size());

  const Entry& proto_entry_a = proto_report.entries(0);
  EXPECT_EQ(id, proto_entry_a.source_id());
  EXPECT_EQ(base::HashMetricName(TestEvent1::kEntryName),
            proto_entry_a.event_hash());

  const Entry& proto_entry_b = proto_report.entries(1);
  EXPECT_EQ(id, proto_entry_b.source_id());
  EXPECT_EQ(base::HashMetricName(TestEvent2::kEntryName),
            proto_entry_b.event_hash());
}

TEST_F(UkmServiceTest, SourceURLLength) {
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(0, GetPersistedLogCount());
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  auto id = GetWhitelistedSourceId(0);

  // This URL is too long to be recorded fully.
  const std::string long_string =
      "https://example.com/" + std::string(10000, 'a');
  recorder.UpdateSourceURL(id, GURL(long_string));

  service.Flush();
  EXPECT_EQ(1, GetPersistedLogCount());

  auto proto_report = GetPersistedReport();
  ASSERT_EQ(1, proto_report.sources_size());
  const Source& proto_source = proto_report.sources(0);
  EXPECT_EQ("URLTooLong", proto_source.url());
}

TEST_F(UkmServiceTest, UnreferencedNonWhitelistedSources) {
  const GURL kURL("https://google.com/foobar");
  for (bool restrict_to_whitelisted_source_ids : {true, false}) {
    base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
    // Set a threshold of number of Sources via Feature Params.
    ScopedUkmFeatureParams params(
        base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        {{"MaxKeptSources", "3"},
         {"WhitelistEntries", Entry1And2Whitelist()},
         {"RestrictToWhitelistedSourceIds",
          restrict_to_whitelisted_source_ids ? "true" : "false"}});

    ClearPrefs();
    UkmService service(&prefs_, &client_,
                       true /* restrict_to_whitelisted_entries */);
    TestRecordingHelper recorder(&service);
    EXPECT_EQ(0, GetPersistedLogCount());
    service.Initialize();
    task_runner_->RunUntilIdle();
    service.EnableRecording(/*extensions=*/false);
    service.EnableReporting();

    // Record with whitelisted ID to whitelist the URL.
    // Use a larger ID to make it last in the proto.
    ukm::SourceId whitelisted_id = GetWhitelistedSourceId(100);
    recorder.UpdateSourceURL(whitelisted_id, kURL);

    std::vector<SourceId> ids;
    base::TimeTicks last_time = base::TimeTicks::Now();
    for (int i = 0; i < 6; ++i) {
      // Wait until base::TimeTicks::Now() no longer equals |last_time|. This
      // ensures each source has a unique timestamp to avoid flakes. Should take
      // between 1-15ms per documented resolution of base::TimeTicks.
      while (base::TimeTicks::Now() == last_time) {
        base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(1));
      }

      ids.push_back(GetNonWhitelistedSourceId(i));
      recorder.UpdateSourceURL(ids.back(), kURL);
      last_time = base::TimeTicks::Now();
    }

    // Add whitelisted entries for 0, 2 and non-whitelisted entries for 2, 3.
    TestEvent1(ids[0]).Record(&service);
    TestEvent2(ids[2]).Record(&service);
    TestEvent3(ids[2]).Record(&service);
    TestEvent3(ids[3]).Record(&service);

    service.Flush();
    EXPECT_EQ(1, GetPersistedLogCount());
    auto proto_report = GetPersistedReport();

    if (restrict_to_whitelisted_source_ids) {
      EXPECT_EQ(1, proto_report.source_counts().observed());
      EXPECT_EQ(1, proto_report.source_counts().navigation_sources());
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(0, proto_report.source_counts().deferred_sources());
      EXPECT_EQ(0, proto_report.source_counts().carryover_sources());

      ASSERT_EQ(1, proto_report.sources_size());
    } else {
      EXPECT_EQ(7, proto_report.source_counts().observed());
      EXPECT_EQ(1, proto_report.source_counts().navigation_sources());
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(4, proto_report.source_counts().deferred_sources());
      EXPECT_EQ(0, proto_report.source_counts().carryover_sources());

      ASSERT_EQ(3, proto_report.sources_size());
      EXPECT_EQ(ids[0], proto_report.sources(0).id());
      EXPECT_EQ(kURL.spec(), proto_report.sources(0).url());
      EXPECT_EQ(ids[2], proto_report.sources(1).id());
      EXPECT_EQ(kURL.spec(), proto_report.sources(1).url());
    }

    // Since MaxKeptSources is 3, only Sources 5, 4, 3 should be retained.
    // Log entries under 0, 1, 3 and 4. Log them in reverse order - which
    // shouldn't affect source ordering in the output.
    //  - Source 0 should not be re-transmitted since it was sent before.
    //  - Source 1 should not be transmitted due to MaxKeptSources param.
    //  - Sources 3 and 4 should be transmitted since they were not sent before.
    TestEvent1(ids[4]).Record(&service);
    TestEvent1(ids[3]).Record(&service);
    TestEvent1(ids[1]).Record(&service);
    TestEvent1(ids[0]).Record(&service);

    service.Flush();
    EXPECT_EQ(2, GetPersistedLogCount());
    proto_report = GetPersistedReport();

    if (restrict_to_whitelisted_source_ids) {
      EXPECT_EQ(0, proto_report.source_counts().observed());
      EXPECT_EQ(0, proto_report.source_counts().navigation_sources());
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(0, proto_report.source_counts().deferred_sources());
      EXPECT_EQ(0, proto_report.source_counts().carryover_sources());

      ASSERT_EQ(0, proto_report.sources_size());
    } else {
      EXPECT_EQ(0, proto_report.source_counts().observed());
      EXPECT_EQ(0, proto_report.source_counts().navigation_sources());
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(1, proto_report.source_counts().deferred_sources());
      EXPECT_EQ(3, proto_report.source_counts().carryover_sources());

      ASSERT_EQ(2, proto_report.sources_size());
      EXPECT_EQ(ids[3], proto_report.sources(0).id());
      EXPECT_EQ(kURL.spec(), proto_report.sources(0).url());
      EXPECT_EQ(ids[4], proto_report.sources(1).id());
      EXPECT_EQ(kURL.spec(), proto_report.sources(1).url());
    }
  }
}

TEST_F(UkmServiceTest, NonWhitelistedUrls) {
  const GURL kURL("https://google.com/foobar");
  struct {
    GURL url;
    bool expected_kept;
  } test_cases[] = {
      {GURL("https://google.com/foobar"), true},
      // For origin-only URLs, only the origin needs to be matched.
      {GURL("https://google.com"), true},
      {GURL("https://google.com/foobar2"), false},
      {GURL("https://other.com"), false},
  };

  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  for (const auto& test : test_cases) {
    ClearPrefs();
    UkmService service(&prefs_, &client_,
                       true /* restrict_to_whitelisted_entries */);
    TestRecordingHelper recorder(&service);

    ASSERT_EQ(GetPersistedLogCount(), 0);
    service.Initialize();
    task_runner_->RunUntilIdle();
    service.EnableRecording(/*extensions=*/false);
    service.EnableReporting();

    // Record with whitelisted ID to whitelist the URL.
    ukm::SourceId whitelist_id = GetWhitelistedSourceId(1);
    recorder.UpdateSourceURL(whitelist_id, kURL);

    // Record non whitelisted ID with a entry.
    ukm::SourceId nonwhitelist_id = GetNonWhitelistedSourceId(100);
    recorder.UpdateSourceURL(nonwhitelist_id, test.url);
    TestEvent1(nonwhitelist_id).Record(&service);

    service.Flush();
    ASSERT_EQ(1, GetPersistedLogCount());
    auto proto_report = GetPersistedReport();

    EXPECT_EQ(2, proto_report.source_counts().observed());
    EXPECT_EQ(1, proto_report.source_counts().navigation_sources());
    if (test.expected_kept) {
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      ASSERT_EQ(2, proto_report.sources_size());
      EXPECT_EQ(whitelist_id, proto_report.sources(0).id());
      EXPECT_EQ(kURL, proto_report.sources(0).url());
      EXPECT_EQ(nonwhitelist_id, proto_report.sources(1).id());
      EXPECT_EQ(test.url, proto_report.sources(1).url());
    } else {
      EXPECT_EQ(1, proto_report.source_counts().unmatched_sources());
      ASSERT_EQ(1, proto_report.sources_size());
      EXPECT_EQ(whitelist_id, proto_report.sources(0).id());
      EXPECT_EQ(kURL, proto_report.sources(0).url());
    }
  }
}

TEST_F(UkmServiceTest, NonWhitelistedCarryoverUrls) {
  const GURL kURL("https://google.com/foobar");

  struct {
    // Source1 is recorded during the first rotation with no entry.
    // An entry for it is recorded in the second rotation.
    GURL source1_url;
    // Should Source1 be seen in second rotation's log.
    bool expect_source1;
    // Source2 is recorded during the second rotation with an entry.
    GURL source2_url;
    // Should Source2 be seen in second rotation's log.
    bool expect_source2;
  } test_cases[] = {
      // Recording the URL captures in the whitelist, which will also allow
      // exact matches of the same URL.
      {GURL("https://google.com/foobar"), true,
       GURL("https://google.com/foobar"), true},
      // Capturing a full URL shouldn't allow origin matches.
      {GURL("https://google.com/foobar"), true, GURL("https://google.com"),
       false},
      // Uncaptured URLs won't get matched.
      {GURL("https://google.com/foobar"), true, GURL("https://other.com"),
       false},
      // Origin should be capturable, and will remember the same origin.
      {GURL("https://google.com"), true, GURL("https://google.com"), true},
      // If the origin is captured, only the origin is remembered.
      {GURL("https://google.com"), true, GURL("https://google.com/foobar"),
       false},
      // Uncaptured URLs won't get matched.
      {GURL("https://google.com"), true, GURL("https://other.com"), false},
      // If the URL isn't captured in the first round, it won't capture later.
      {GURL("https://other.com"), false, GURL("https://google.com/foobar"),
       false},
      {GURL("https://other.com"), false, GURL("https://google.com"), false},
      // Entries shouldn't whitelist themselves.
      {GURL("https://other.com"), false, GURL("https://other.com"), false},
  };

  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE,
                                {{"WhitelistEntries", Entry1And2Whitelist()}});

  for (const auto& test : test_cases) {
    ClearPrefs();
    UkmService service(&prefs_, &client_,
                       true /* restrict_to_whitelisted_entries */);
    TestRecordingHelper recorder(&service);

    EXPECT_EQ(GetPersistedLogCount(), 0);
    service.Initialize();
    task_runner_->RunUntilIdle();
    service.EnableRecording(/*extensions=*/false);
    service.EnableReporting();

    // Record with whitelisted ID to whitelist the URL.
    ukm::SourceId whitelist_id = GetWhitelistedSourceId(1);
    recorder.UpdateSourceURL(whitelist_id, kURL);

    // Record test Source1 without an event.
    ukm::SourceId nonwhitelist_id1 = GetNonWhitelistedSourceId(100);
    recorder.UpdateSourceURL(nonwhitelist_id1, test.source1_url);

    service.Flush();
    ASSERT_EQ(1, GetPersistedLogCount());
    auto proto_report = GetPersistedReport();

    EXPECT_EQ(2, proto_report.source_counts().observed());
    EXPECT_EQ(1, proto_report.source_counts().navigation_sources());
    EXPECT_EQ(0, proto_report.source_counts().carryover_sources());
    if (test.expect_source1) {
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(1, proto_report.source_counts().deferred_sources());
    } else {
      EXPECT_EQ(1, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(0, proto_report.source_counts().deferred_sources());
    }
    ASSERT_EQ(1, proto_report.sources_size());
    EXPECT_EQ(whitelist_id, proto_report.sources(0).id());
    EXPECT_EQ(kURL, proto_report.sources(0).url());

    // Record the Source2 and events for Source1 and Source2.
    ukm::SourceId nonwhitelist_id2 = GetNonWhitelistedSourceId(101);
    recorder.UpdateSourceURL(nonwhitelist_id2, test.source2_url);
    TestEvent1(nonwhitelist_id1).Record(&service);
    TestEvent1(nonwhitelist_id2).Record(&service);

    service.Flush();
    ASSERT_EQ(2, GetPersistedLogCount());
    proto_report = GetPersistedReport();

    EXPECT_EQ(1, proto_report.source_counts().observed());
    EXPECT_EQ(0, proto_report.source_counts().navigation_sources());
    EXPECT_EQ(0, proto_report.source_counts().deferred_sources());
    if (!test.expect_source1) {
      EXPECT_FALSE(test.expect_source2);
      EXPECT_EQ(1, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(0, proto_report.source_counts().carryover_sources());
      ASSERT_EQ(0, proto_report.sources_size());
    } else if (!test.expect_source2) {
      EXPECT_EQ(1, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(1, proto_report.source_counts().carryover_sources());
      ASSERT_EQ(1, proto_report.sources_size());
      EXPECT_EQ(nonwhitelist_id1, proto_report.sources(0).id());
      EXPECT_EQ(test.source1_url, proto_report.sources(0).url());
    } else {
      EXPECT_EQ(0, proto_report.source_counts().unmatched_sources());
      EXPECT_EQ(1, proto_report.source_counts().carryover_sources());
      ASSERT_EQ(2, proto_report.sources_size());
      EXPECT_EQ(nonwhitelist_id1, proto_report.sources(0).id());
      EXPECT_EQ(test.source1_url, proto_report.sources(0).url());
      EXPECT_EQ(nonwhitelist_id2, proto_report.sources(1).id());
      EXPECT_EQ(test.source2_url, proto_report.sources(1).url());
    }
  }
}

TEST_F(UkmServiceTest, SupportedSchemes) {
  struct {
    const char* url;
    bool expected_kept;
  } test_cases[] = {
      {"http://google.ca/", true},
      {"https://google.ca/", true},
      {"ftp://google.ca/", true},
      {"about:blank", true},
      {"chrome://version/", true},
      // chrome-extension are controlled by TestIsWebstoreExtension, above.
      {"chrome-extension://bhcnanendmgjjeghamaccjnochlnhcgj/", true},
      {"chrome-extension://abcdefghijklmnopqrstuvwxyzabcdef/", false},
      {"file:///tmp/", false},
      {"abc://google.ca/", false},
      {"www.google.ca/", false},
  };

  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE, {});
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  service.SetIsWebstoreExtensionCallback(
      base::BindRepeating(&TestIsWebstoreExtension));

  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/true);
  service.EnableReporting();

  int64_t id_counter = 1;
  int expected_kept_count = 0;
  for (const auto& test : test_cases) {
    auto source_id = GetWhitelistedSourceId(id_counter++);
    recorder.UpdateSourceURL(source_id, GURL(test.url));
    TestEvent1(source_id).Record(&service);
    if (test.expected_kept)
      ++expected_kept_count;
  }

  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 1);
  Report proto_report = GetPersistedReport();

  EXPECT_EQ(expected_kept_count, proto_report.sources_size());
  for (const auto& test : test_cases) {
    bool found = false;
    for (int i = 0; i < proto_report.sources_size(); ++i) {
      if (proto_report.sources(i).url() == test.url) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(test.expected_kept, found) << test.url;
  }
}

TEST_F(UkmServiceTest, SupportedSchemesNoExtensions) {
  struct {
    const char* url;
    bool expected_kept;
  } test_cases[] = {
      {"http://google.ca/", true},
      {"https://google.ca/", true},
      {"ftp://google.ca/", true},
      {"about:blank", true},
      {"chrome://version/", true},
      {"chrome-extension://bhcnanendmgjjeghamaccjnochlnhcgj/", false},
      {"chrome-extension://abcdefghijklmnopqrstuvwxyzabcdef/", false},
      {"file:///tmp/", false},
      {"abc://google.ca/", false},
      {"www.google.ca/", false},
  };

  base::FieldTrialList field_trial_list(nullptr /* entropy_provider */);
  ScopedUkmFeatureParams params(base::FeatureList::OVERRIDE_ENABLE_FEATURE, {});
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);

  EXPECT_EQ(GetPersistedLogCount(), 0);
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  int64_t id_counter = 1;
  int expected_kept_count = 0;
  for (const auto& test : test_cases) {
    auto source_id = GetWhitelistedSourceId(id_counter++);
    recorder.UpdateSourceURL(source_id, GURL(test.url));
    TestEvent1(source_id).Record(&service);
    if (test.expected_kept)
      ++expected_kept_count;
  }

  service.Flush();
  EXPECT_EQ(GetPersistedLogCount(), 1);
  Report proto_report = GetPersistedReport();

  EXPECT_EQ(expected_kept_count, proto_report.sources_size());
  for (const auto& test : test_cases) {
    bool found = false;
    for (int i = 0; i < proto_report.sources_size(); ++i) {
      if (proto_report.sources(i).url() == test.url) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(test.expected_kept, found) << test.url;
  }
}

TEST_F(UkmServiceTest, SanitizeUrlAuthParams) {
  UkmService service(&prefs_, &client_,
                     true /* restrict_to_whitelisted_entries */);
  TestRecordingHelper recorder(&service);
  EXPECT_EQ(0, GetPersistedLogCount());
  service.Initialize();
  task_runner_->RunUntilIdle();
  service.EnableRecording(/*extensions=*/false);
  service.EnableReporting();

  auto id = GetWhitelistedSourceId(0);
  recorder.UpdateSourceURL(id, GURL("https://username:password@example.com/"));

  service.Flush();
  EXPECT_EQ(1, GetPersistedLogCount());

  auto proto_report = GetPersistedReport();
  ASSERT_EQ(1, proto_report.sources_size());
  const Source& proto_source = proto_report.sources(0);
  EXPECT_EQ("https://example.com/", proto_source.url());
}

TEST_F(UkmServiceTest, SanitizeChromeUrlParams) {
  struct {
    const char* url;
    const char* expected_url;
  } test_cases[] = {
      {"chrome://version/?foo=bar", "chrome://version/"},
      {"about:blank?foo=bar", "about:blank"},
      {"chrome://histograms/Variations", "chrome://histograms/Variations"},
      {"http://google.ca/?foo=bar", "http://google.ca/?foo=bar"},
      {"https://google.ca/?foo=bar", "https://google.ca/?foo=bar"},
      {"ftp://google.ca/?foo=bar", "ftp://google.ca/?foo=bar"},
      {"chrome-extension://bhcnanendmgjjeghamaccjnochlnhcgj/foo.html?a=b",
       "chrome-extension://bhcnanendmgjjeghamaccjnochlnhcgj/"},
  };

  for (const auto& test : test_cases) {
    ClearPrefs();

    UkmService service(&prefs_, &client_,
                       true /* restrict_to_whitelisted_entries */);
    TestRecordingHelper recorder(&service);
    service.SetIsWebstoreExtensionCallback(
        base::BindRepeating(&TestIsWebstoreExtension));

    EXPECT_EQ(0, GetPersistedLogCount());
    service.Initialize();
    task_runner_->RunUntilIdle();
    service.EnableRecording(/*extensions=*/true);
    service.EnableReporting();

    auto id = GetWhitelistedSourceId(0);
    recorder.UpdateSourceURL(id, GURL(test.url));

    service.Flush();
    EXPECT_EQ(1, GetPersistedLogCount());

    auto proto_report = GetPersistedReport();
    ASSERT_EQ(1, proto_report.sources_size());
    const Source& proto_source = proto_report.sources(0);
    EXPECT_EQ(test.expected_url, proto_source.url());
  }
}

}  // namespace ukm

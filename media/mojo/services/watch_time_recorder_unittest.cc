// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/watch_time_recorder.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/hash.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/test_message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/ukm/test_ukm_recorder.h"
#include "media/base/watch_time_keys.h"
#include "media/mojo/services/media_metrics_provider.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using UkmEntry = ukm::builders::Media_BasicPlayback;

namespace media {

constexpr char kTestOrigin[] = "https://test.google.com/";

class WatchTimeRecorderTest : public testing::Test {
 public:
  WatchTimeRecorderTest()
      : computation_keys_(
            {WatchTimeKey::kAudioSrc, WatchTimeKey::kAudioMse,
             WatchTimeKey::kAudioEme, WatchTimeKey::kAudioVideoSrc,
             WatchTimeKey::kAudioVideoMse, WatchTimeKey::kAudioVideoEme}),
        mtbr_keys_({kMeanTimeBetweenRebuffersAudioSrc,
                    kMeanTimeBetweenRebuffersAudioMse,
                    kMeanTimeBetweenRebuffersAudioEme,
                    kMeanTimeBetweenRebuffersAudioVideoSrc,
                    kMeanTimeBetweenRebuffersAudioVideoMse,
                    kMeanTimeBetweenRebuffersAudioVideoEme}),
        smooth_keys_({kRebuffersCountAudioSrc, kRebuffersCountAudioMse,
                      kRebuffersCountAudioEme, kRebuffersCountAudioVideoSrc,
                      kRebuffersCountAudioVideoMse,
                      kRebuffersCountAudioVideoEme}),
        discard_keys_({kDiscardedWatchTimeAudioSrc, kDiscardedWatchTimeAudioMse,
                       kDiscardedWatchTimeAudioEme,
                       kDiscardedWatchTimeAudioVideoSrc,
                       kDiscardedWatchTimeAudioVideoMse,
                       kDiscardedWatchTimeAudioVideoEme}) {
    ResetMetricRecorders();
    MediaMetricsProvider::Create(nullptr, mojo::MakeRequest(&provider_));
  }

  ~WatchTimeRecorderTest() override { base::RunLoop().RunUntilIdle(); }

  void Initialize(mojom::PlaybackPropertiesPtr properties) {
    provider_->Initialize(properties->is_mse, true /* is_top_frame */,
                          url::Origin::Create(GURL(kTestOrigin)));
    provider_->AcquireWatchTimeRecorder(std::move(properties),
                                        mojo::MakeRequest(&wtr_));
  }

  void Initialize(bool has_audio,
                  bool has_video,
                  bool is_mse,
                  bool is_encrypted) {
    Initialize(mojom::PlaybackProperties::New(
        kUnknownAudioCodec, kUnknownVideoCodec, has_audio, has_video, false,
        false, is_mse, is_encrypted, false, gfx::Size(800, 600)));
  }

  void ExpectWatchTime(const std::vector<base::StringPiece>& keys,
                       base::TimeDelta value) {
    for (int i = 0; i <= static_cast<int>(WatchTimeKey::kWatchTimeKeyMax);
         ++i) {
      const base::StringPiece test_key =
          ConvertWatchTimeKeyToStringForUma(static_cast<WatchTimeKey>(i));
      if (test_key.empty())
        continue;
      auto it = std::find(keys.begin(), keys.end(), test_key);
      if (it == keys.end()) {
        histogram_tester_->ExpectTotalCount(test_key.as_string(), 0);
      } else {
        histogram_tester_->ExpectUniqueSample(test_key.as_string(),
                                              value.InMilliseconds(), 1);
      }
    }
  }

  void ExpectHelper(const std::vector<base::StringPiece>& full_key_list,
                    const std::vector<base::StringPiece>& keys,
                    int64_t value) {
    for (auto key : full_key_list) {
      auto it = std::find(keys.begin(), keys.end(), key);
      if (it == keys.end())
        histogram_tester_->ExpectTotalCount(key.as_string(), 0);
      else
        histogram_tester_->ExpectUniqueSample(key.as_string(), value, 1);
    }
  }

  void ExpectMtbrTime(const std::vector<base::StringPiece>& keys,
                      base::TimeDelta value) {
    ExpectHelper(mtbr_keys_, keys, value.InMilliseconds());
  }

  void ExpectZeroRebuffers(const std::vector<base::StringPiece>& keys) {
    ExpectHelper(smooth_keys_, keys, 0);
  }

  void ExpectRebuffers(const std::vector<base::StringPiece>& keys, int count) {
    ExpectHelper(smooth_keys_, keys, count);
  }

  void ExpectNoUkmWatchTime() {
    ASSERT_EQ(0u, test_recorder_->sources_count());
    ASSERT_EQ(0u, test_recorder_->entries_count());
  }

  void ExpectUkmWatchTime(const std::vector<base::StringPiece>& keys,
                          base::TimeDelta value) {
    const auto& entries =
        test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
    EXPECT_EQ(1u, entries.size());
    for (const auto* entry : entries) {
      test_recorder_->ExpectEntrySourceHasUrl(entry, GURL(kTestOrigin));
      for (auto key : keys) {
        test_recorder_->ExpectEntryMetric(entry, key.data(),
                                          value.InMilliseconds());
      }
    }
  }

  void ResetMetricRecorders() {
    histogram_tester_.reset(new base::HistogramTester());
    // Ensure cleared global before attempting to create a new TestUkmReporter.
    test_recorder_.reset();
    test_recorder_.reset(new ukm::TestAutoSetUkmRecorder());
  }

  MOCK_METHOD0(GetCurrentMediaTime, base::TimeDelta());

 protected:
  base::MessageLoop message_loop_;
  mojom::MediaMetricsProviderPtr provider_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<ukm::TestAutoSetUkmRecorder> test_recorder_;
  mojom::WatchTimeRecorderPtr wtr_;
  const std::vector<WatchTimeKey> computation_keys_;
  const std::vector<base::StringPiece> mtbr_keys_;
  const std::vector<base::StringPiece> smooth_keys_;
  const std::vector<base::StringPiece> discard_keys_;

  DISALLOW_COPY_AND_ASSIGN(WatchTimeRecorderTest);
};

TEST_F(WatchTimeRecorderTest, TestBasicReporting) {
  constexpr base::TimeDelta kWatchTime1 = base::TimeDelta::FromSeconds(25);
  constexpr base::TimeDelta kWatchTime2 = base::TimeDelta::FromSeconds(50);

  for (int i = 0; i <= static_cast<int>(WatchTimeKey::kWatchTimeKeyMax); ++i) {
    const WatchTimeKey key = static_cast<WatchTimeKey>(i);

    auto key_str = ConvertWatchTimeKeyToStringForUma(key);
    SCOPED_TRACE(key_str.empty() ? base::NumberToString(i)
                                 : key_str.as_string());

    // Values for |is_background| and |is_muted| don't matter in this test since
    // they don't prevent the muted or background keys from being recorded.
    Initialize(true, false, true, true);
    wtr_->RecordWatchTime(WatchTimeKey::kWatchTimeKeyMax, kWatchTime1);
    wtr_->RecordWatchTime(key, kWatchTime1);
    wtr_->RecordWatchTime(key, kWatchTime2);
    base::RunLoop().RunUntilIdle();

    // Nothing should be recorded yet since we haven't finalized.
    ExpectWatchTime({}, base::TimeDelta());

    // Only the requested key should be finalized.
    wtr_->FinalizeWatchTime({key});
    base::RunLoop().RunUntilIdle();

    if (!key_str.empty())
      ExpectWatchTime({key_str}, kWatchTime2);

    // These keys are only reported for a full finalize.
    ExpectMtbrTime({}, base::TimeDelta());
    ExpectZeroRebuffers({});
    ExpectNoUkmWatchTime();

    // Verify nothing else is recorded except for what we finalized above.
    ResetMetricRecorders();
    wtr_.reset();
    base::RunLoop().RunUntilIdle();
    ExpectWatchTime({}, base::TimeDelta());
    ExpectMtbrTime({}, base::TimeDelta());
    ExpectZeroRebuffers({});

    switch (key) {
      case WatchTimeKey::kAudioAll:
      case WatchTimeKey::kAudioBackgroundAll:
      case WatchTimeKey::kAudioVideoAll:
      case WatchTimeKey::kAudioVideoBackgroundAll:
      case WatchTimeKey::kAudioVideoMutedAll:
      case WatchTimeKey::kVideoAll:
      case WatchTimeKey::kVideoBackgroundAll:
        ExpectUkmWatchTime({UkmEntry::kWatchTimeName}, kWatchTime2);
        break;

      // These keys are not reported, instead we boolean flags for each type.
      case WatchTimeKey::kAudioMse:
      case WatchTimeKey::kAudioEme:
      case WatchTimeKey::kAudioSrc:
      case WatchTimeKey::kAudioEmbeddedExperience:
      case WatchTimeKey::kAudioBackgroundMse:
      case WatchTimeKey::kAudioBackgroundEme:
      case WatchTimeKey::kAudioBackgroundSrc:
      case WatchTimeKey::kAudioBackgroundEmbeddedExperience:
      case WatchTimeKey::kAudioVideoMse:
      case WatchTimeKey::kAudioVideoEme:
      case WatchTimeKey::kAudioVideoSrc:
      case WatchTimeKey::kAudioVideoEmbeddedExperience:
      case WatchTimeKey::kAudioVideoMutedMse:
      case WatchTimeKey::kAudioVideoMutedEme:
      case WatchTimeKey::kAudioVideoMutedSrc:
      case WatchTimeKey::kAudioVideoMutedEmbeddedExperience:
      case WatchTimeKey::kAudioVideoBackgroundMse:
      case WatchTimeKey::kAudioVideoBackgroundEme:
      case WatchTimeKey::kAudioVideoBackgroundSrc:
      case WatchTimeKey::kAudioVideoBackgroundEmbeddedExperience:
      case WatchTimeKey::kVideoMse:
      case WatchTimeKey::kVideoEme:
      case WatchTimeKey::kVideoSrc:
      case WatchTimeKey::kVideoEmbeddedExperience:
      case WatchTimeKey::kVideoBackgroundMse:
      case WatchTimeKey::kVideoBackgroundEme:
      case WatchTimeKey::kVideoBackgroundSrc:
      case WatchTimeKey::kVideoBackgroundEmbeddedExperience:
        ExpectUkmWatchTime({}, base::TimeDelta());
        break;

      // These keys roll up into the battery watch time field.
      case WatchTimeKey::kAudioBattery:
      case WatchTimeKey::kAudioBackgroundBattery:
      case WatchTimeKey::kAudioVideoBattery:
      case WatchTimeKey::kAudioVideoMutedBattery:
      case WatchTimeKey::kAudioVideoBackgroundBattery:
      case WatchTimeKey::kVideoBattery:
      case WatchTimeKey::kVideoBackgroundBattery:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_BatteryName}, kWatchTime2);
        break;

      // These keys roll up into the AC watch time field.
      case WatchTimeKey::kAudioAc:
      case WatchTimeKey::kAudioBackgroundAc:
      case WatchTimeKey::kAudioVideoAc:
      case WatchTimeKey::kAudioVideoBackgroundAc:
      case WatchTimeKey::kAudioVideoMutedAc:
      case WatchTimeKey::kVideoAc:
      case WatchTimeKey::kVideoBackgroundAc:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_ACName}, kWatchTime2);
        break;

      case WatchTimeKey::kAudioVideoDisplayFullscreen:
      case WatchTimeKey::kAudioVideoMutedDisplayFullscreen:
      case WatchTimeKey::kVideoDisplayFullscreen:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_DisplayFullscreenName},
                           kWatchTime2);
        break;

      case WatchTimeKey::kAudioVideoDisplayInline:
      case WatchTimeKey::kAudioVideoMutedDisplayInline:
      case WatchTimeKey::kVideoDisplayInline:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_DisplayInlineName},
                           kWatchTime2);
        break;

      case WatchTimeKey::kAudioVideoDisplayPictureInPicture:
      case WatchTimeKey::kAudioVideoMutedDisplayPictureInPicture:
      case WatchTimeKey::kVideoDisplayPictureInPicture:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_DisplayPictureInPictureName},
                           kWatchTime2);
        break;

      case WatchTimeKey::kAudioNativeControlsOn:
      case WatchTimeKey::kAudioVideoNativeControlsOn:
      case WatchTimeKey::kAudioVideoMutedNativeControlsOn:
      case WatchTimeKey::kVideoNativeControlsOn:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_NativeControlsOnName},
                           kWatchTime2);
        break;

      case WatchTimeKey::kAudioNativeControlsOff:
      case WatchTimeKey::kAudioVideoNativeControlsOff:
      case WatchTimeKey::kAudioVideoMutedNativeControlsOff:
      case WatchTimeKey::kVideoNativeControlsOff:
        ExpectUkmWatchTime({UkmEntry::kWatchTime_NativeControlsOffName},
                           kWatchTime2);
        break;
    }

    ResetMetricRecorders();
  }
}

TEST_F(WatchTimeRecorderTest, TestRebufferingMetrics) {
  Initialize(true, false, true, true);

  constexpr base::TimeDelta kWatchTime = base::TimeDelta::FromSeconds(50);
  for (auto key : computation_keys_)
    wtr_->RecordWatchTime(key, kWatchTime);
  wtr_->UpdateUnderflowCount(1);
  wtr_->UpdateUnderflowCount(2);

  // Trigger finalization of everything.
  wtr_->FinalizeWatchTime({});
  base::RunLoop().RunUntilIdle();

  ExpectMtbrTime(mtbr_keys_, kWatchTime / 2);
  ExpectRebuffers(smooth_keys_, 2);

  // Now rerun the test without any rebuffering.
  ResetMetricRecorders();
  for (auto key : computation_keys_)
    wtr_->RecordWatchTime(key, kWatchTime);
  wtr_->FinalizeWatchTime({});
  base::RunLoop().RunUntilIdle();

  ExpectMtbrTime({}, base::TimeDelta());
  ExpectZeroRebuffers(smooth_keys_);

  // Now rerun the test with a small amount of watch time and ensure rebuffering
  // isn't recorded because we haven't met the watch time requirements.
  ResetMetricRecorders();
  constexpr base::TimeDelta kWatchTimeShort = base::TimeDelta::FromSeconds(5);
  for (auto key : computation_keys_)
    wtr_->RecordWatchTime(key, kWatchTimeShort);
  wtr_->UpdateUnderflowCount(1);
  wtr_->UpdateUnderflowCount(2);
  wtr_->FinalizeWatchTime({});
  base::RunLoop().RunUntilIdle();

  // Nothing should be logged since this doesn't meet requirements.
  ExpectMtbrTime({}, base::TimeDelta());
  for (auto key : smooth_keys_)
    histogram_tester_->ExpectTotalCount(key.as_string(), 0);
}

TEST_F(WatchTimeRecorderTest, TestDiscardMetrics) {
  Initialize(true, false, true, true);

  constexpr base::TimeDelta kWatchTime = base::TimeDelta::FromSeconds(5);
  for (auto key : computation_keys_)
    wtr_->RecordWatchTime(key, kWatchTime);

  // Trigger finalization of everything.
  wtr_.reset();
  base::RunLoop().RunUntilIdle();

  // No standard watch time should be recorded because it falls below the
  // reporting threshold.
  ExpectWatchTime({}, base::TimeDelta());

  // Verify the time was instead logged to the discard keys.
  for (auto key : discard_keys_) {
    histogram_tester_->ExpectUniqueSample(key.as_string(),
                                          kWatchTime.InMilliseconds(), 1);
  }

  // UKM watch time won't be logged because we aren't sending "All" keys.
  ExpectUkmWatchTime({}, base::TimeDelta());
}

#define EXPECT_UKM(name, value) \
  test_recorder_->ExpectEntryMetric(entry, name, value)
#define EXPECT_NO_UKM(name) \
  EXPECT_FALSE(test_recorder_->EntryHasMetric(entry, name))
#define EXPECT_HAS_UKM(name) \
  EXPECT_TRUE(test_recorder_->EntryHasMetric(entry, name));

TEST_F(WatchTimeRecorderTest, TestFinalizeNoDuplication) {
  mojom::PlaybackPropertiesPtr properties = mojom::PlaybackProperties::New(
      kCodecAAC, kCodecH264, true, true, false, false, false, false, false,
      gfx::Size(800, 600));
  Initialize(properties.Clone());

  // Verify that UKM is reported along with the watch time.
  constexpr base::TimeDelta kWatchTime = base::TimeDelta::FromSeconds(4);
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoAll, kWatchTime);

  // Finalize everything. UKM is only recorded at destruction, so this should do
  // nothing.
  wtr_->FinalizeWatchTime({});
  base::RunLoop().RunUntilIdle();

  // No watch time should have been recorded since this is below the UMA report
  // threshold.
  ExpectWatchTime({}, base::TimeDelta());
  ExpectMtbrTime({}, base::TimeDelta());
  ExpectZeroRebuffers({});
  ExpectNoUkmWatchTime();

  const auto& empty_entries =
      test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(0u, empty_entries.size());

  // Verify UKM is logged at destruction time.
  ResetMetricRecorders();
  wtr_.reset();
  base::RunLoop().RunUntilIdle();
  const auto& entries = test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_recorder_->ExpectEntrySourceHasUrl(entry, GURL(kTestOrigin));

    EXPECT_UKM(UkmEntry::kIsBackgroundName, properties->is_background);
    EXPECT_UKM(UkmEntry::kIsMutedName, properties->is_muted);
    EXPECT_UKM(UkmEntry::kAudioCodecName, properties->audio_codec);
    EXPECT_UKM(UkmEntry::kVideoCodecName, properties->video_codec);
    EXPECT_UKM(UkmEntry::kHasAudioName, properties->has_audio);
    EXPECT_UKM(UkmEntry::kHasVideoName, properties->has_video);
    EXPECT_UKM(UkmEntry::kIsEMEName, properties->is_eme);
    EXPECT_UKM(UkmEntry::kIsMSEName, properties->is_mse);
    EXPECT_UKM(UkmEntry::kLastPipelineStatusName, PIPELINE_OK);
    EXPECT_UKM(UkmEntry::kRebuffersCountName, 0);
    EXPECT_UKM(UkmEntry::kVideoNaturalWidthName,
               properties->natural_size.width());
    EXPECT_UKM(UkmEntry::kVideoNaturalHeightName,
               properties->natural_size.height());
    EXPECT_UKM(UkmEntry::kWatchTimeName, kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kAudioDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kVideoDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kAutoplayInitiatedName, false);
    EXPECT_HAS_UKM(UkmEntry::kPlayerIDName);

    EXPECT_NO_UKM(UkmEntry::kMeanTimeBetweenRebuffersName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_ACName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_BatteryName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOnName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOffName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayFullscreenName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayInlineName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayPictureInPictureName);
  }
}

TEST_F(WatchTimeRecorderTest, FinalizeWithoutWatchTime) {
  mojom::PlaybackPropertiesPtr properties = mojom::PlaybackProperties::New(
      kCodecAAC, kCodecH264, true, true, false, false, false, false, false,
      gfx::Size(800, 600));
  Initialize(properties.Clone());

  // Finalize everything. UKM is only recorded at destruction, so this should do
  // nothing.
  wtr_->FinalizeWatchTime({});
  base::RunLoop().RunUntilIdle();

  // No watch time should have been recorded even though a finalize event will
  // be sent, however a UKM entry with the playback properties will still be
  // generated.
  ExpectWatchTime({}, base::TimeDelta());
  ExpectMtbrTime({}, base::TimeDelta());
  ExpectZeroRebuffers({});
  ExpectNoUkmWatchTime();

  const auto& empty_entries =
      test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(0u, empty_entries.size());

  // Destructing the recorder should generate a UKM report though.
  ResetMetricRecorders();
  wtr_.reset();
  base::RunLoop().RunUntilIdle();
  const auto& entries = test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_recorder_->ExpectEntrySourceHasUrl(entry, GURL(kTestOrigin));

    EXPECT_UKM(UkmEntry::kIsBackgroundName, properties->is_background);
    EXPECT_UKM(UkmEntry::kIsMutedName, properties->is_muted);
    EXPECT_UKM(UkmEntry::kAudioCodecName, properties->audio_codec);
    EXPECT_UKM(UkmEntry::kVideoCodecName, properties->video_codec);
    EXPECT_UKM(UkmEntry::kHasAudioName, properties->has_audio);
    EXPECT_UKM(UkmEntry::kHasVideoName, properties->has_video);
    EXPECT_UKM(UkmEntry::kIsEMEName, properties->is_eme);
    EXPECT_UKM(UkmEntry::kIsMSEName, properties->is_mse);
    EXPECT_UKM(UkmEntry::kLastPipelineStatusName, PIPELINE_OK);
    EXPECT_UKM(UkmEntry::kRebuffersCountName, 0);
    EXPECT_UKM(UkmEntry::kVideoNaturalWidthName,
               properties->natural_size.width());
    EXPECT_UKM(UkmEntry::kVideoNaturalHeightName,
               properties->natural_size.height());
    EXPECT_UKM(UkmEntry::kAudioDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kVideoDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kAutoplayInitiatedName, false);
    EXPECT_HAS_UKM(UkmEntry::kPlayerIDName);

    EXPECT_NO_UKM(UkmEntry::kMeanTimeBetweenRebuffersName);
    EXPECT_NO_UKM(UkmEntry::kWatchTimeName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_ACName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_BatteryName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOnName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOffName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayFullscreenName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayInlineName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayPictureInPictureName);
  }
}

TEST_F(WatchTimeRecorderTest, BasicUkmAudioVideo) {
  mojom::PlaybackPropertiesPtr properties = mojom::PlaybackProperties::New(
      kCodecAAC, kCodecH264, true, true, false, false, false, false, false,
      gfx::Size(800, 600));
  Initialize(properties.Clone());

  constexpr base::TimeDelta kWatchTime = base::TimeDelta::FromSeconds(4);
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoAll, kWatchTime);
  wtr_.reset();
  base::RunLoop().RunUntilIdle();

  const auto& entries = test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_recorder_->ExpectEntrySourceHasUrl(entry, GURL(kTestOrigin));

    EXPECT_UKM(UkmEntry::kWatchTimeName, kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kIsBackgroundName, properties->is_background);
    EXPECT_UKM(UkmEntry::kIsMutedName, properties->is_muted);
    EXPECT_UKM(UkmEntry::kAudioCodecName, properties->audio_codec);
    EXPECT_UKM(UkmEntry::kVideoCodecName, properties->video_codec);
    EXPECT_UKM(UkmEntry::kHasAudioName, properties->has_audio);
    EXPECT_UKM(UkmEntry::kHasVideoName, properties->has_video);
    EXPECT_UKM(UkmEntry::kIsEMEName, properties->is_eme);
    EXPECT_UKM(UkmEntry::kIsMSEName, properties->is_mse);
    EXPECT_UKM(UkmEntry::kLastPipelineStatusName, PIPELINE_OK);
    EXPECT_UKM(UkmEntry::kRebuffersCountName, 0);
    EXPECT_UKM(UkmEntry::kVideoNaturalWidthName,
               properties->natural_size.width());
    EXPECT_UKM(UkmEntry::kVideoNaturalHeightName,
               properties->natural_size.height());
    EXPECT_HAS_UKM(UkmEntry::kPlayerIDName);
    EXPECT_UKM(UkmEntry::kAudioDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kVideoDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kAutoplayInitiatedName, false);

    EXPECT_NO_UKM(UkmEntry::kMeanTimeBetweenRebuffersName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_ACName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_BatteryName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOnName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOffName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayFullscreenName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayInlineName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayPictureInPictureName);
  }
}

TEST_F(WatchTimeRecorderTest, BasicUkmAudioVideoWithExtras) {
  mojom::PlaybackPropertiesPtr properties = mojom::PlaybackProperties::New(
      kCodecOpus, kCodecVP9, true, true, false, false, true, true, false,
      gfx::Size(800, 600));
  Initialize(properties.Clone());

  constexpr base::TimeDelta kWatchTime = base::TimeDelta::FromSeconds(54);
  const base::TimeDelta kWatchTime2 = kWatchTime * 2;
  const base::TimeDelta kWatchTime3 = kWatchTime / 3;
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoAll, kWatchTime2);
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoAc, kWatchTime);

  // Ensure partial finalize does not affect final report.
  wtr_->FinalizeWatchTime({WatchTimeKey::kAudioVideoAc});
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoBattery, kWatchTime);
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoNativeControlsOn, kWatchTime);
  wtr_->FinalizeWatchTime({WatchTimeKey::kAudioVideoNativeControlsOn});
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoNativeControlsOff, kWatchTime);
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoDisplayFullscreen,
                        kWatchTime3);
  wtr_->FinalizeWatchTime({WatchTimeKey::kAudioVideoDisplayFullscreen});
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoDisplayInline, kWatchTime3);
  wtr_->FinalizeWatchTime({WatchTimeKey::kAudioVideoDisplayInline});
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoDisplayPictureInPicture,
                        kWatchTime3);
  wtr_->UpdateUnderflowCount(3);
  wtr_->OnError(PIPELINE_ERROR_DECODE);

  const std::string kAudioDecoderName = "MojoAudioDecoder";
  const std::string kVideoDecoderName = "MojoVideoDecoder";
  wtr_->SetAudioDecoderName(kAudioDecoderName);
  wtr_->SetVideoDecoderName(kVideoDecoderName);

  wtr_->SetAutoplayInitiated(true);

  wtr_.reset();
  base::RunLoop().RunUntilIdle();

  const auto& entries = test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_recorder_->ExpectEntrySourceHasUrl(entry, GURL(kTestOrigin));
    EXPECT_UKM(UkmEntry::kWatchTimeName, kWatchTime2.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_ACName, kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_BatteryName, kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_NativeControlsOnName,
               kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_NativeControlsOffName,
               kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_DisplayFullscreenName,
               kWatchTime3.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_DisplayInlineName,
               kWatchTime3.InMilliseconds());
    EXPECT_UKM(UkmEntry::kWatchTime_DisplayPictureInPictureName,
               kWatchTime3.InMilliseconds());
    EXPECT_UKM(UkmEntry::kMeanTimeBetweenRebuffersName,
               kWatchTime2.InMilliseconds() / 3);
    EXPECT_HAS_UKM(UkmEntry::kPlayerIDName);

    // Values taken from .cc private enumeration (and should never change).
    EXPECT_UKM(UkmEntry::kAudioDecoderNameName, 2);
    EXPECT_UKM(UkmEntry::kVideoDecoderNameName, 5);

    EXPECT_UKM(UkmEntry::kIsBackgroundName, properties->is_background);
    EXPECT_UKM(UkmEntry::kIsMutedName, properties->is_muted);
    EXPECT_UKM(UkmEntry::kAudioCodecName, properties->audio_codec);
    EXPECT_UKM(UkmEntry::kVideoCodecName, properties->video_codec);
    EXPECT_UKM(UkmEntry::kHasAudioName, properties->has_audio);
    EXPECT_UKM(UkmEntry::kHasVideoName, properties->has_video);
    EXPECT_UKM(UkmEntry::kIsEMEName, properties->is_eme);
    EXPECT_UKM(UkmEntry::kIsMSEName, properties->is_mse);
    EXPECT_UKM(UkmEntry::kLastPipelineStatusName, PIPELINE_ERROR_DECODE);
    EXPECT_UKM(UkmEntry::kRebuffersCountName, 3);
    EXPECT_UKM(UkmEntry::kVideoNaturalWidthName,
               properties->natural_size.width());
    EXPECT_UKM(UkmEntry::kVideoNaturalHeightName,
               properties->natural_size.height());
    EXPECT_UKM(UkmEntry::kAutoplayInitiatedName, true);
  }
}

TEST_F(WatchTimeRecorderTest, BasicUkmAudioVideoBackgroundMuted) {
  mojom::PlaybackPropertiesPtr properties = mojom::PlaybackProperties::New(
      kCodecAAC, kCodecH264, true, true, true, true, false, false, false,
      gfx::Size(800, 600));
  Initialize(properties.Clone());

  constexpr base::TimeDelta kWatchTime = base::TimeDelta::FromSeconds(54);
  wtr_->RecordWatchTime(WatchTimeKey::kAudioVideoBackgroundAll, kWatchTime);
  wtr_.reset();
  base::RunLoop().RunUntilIdle();

  const auto& entries = test_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  EXPECT_EQ(1u, entries.size());
  for (const auto* entry : entries) {
    test_recorder_->ExpectEntrySourceHasUrl(entry, GURL(kTestOrigin));

    EXPECT_UKM(UkmEntry::kWatchTimeName, kWatchTime.InMilliseconds());
    EXPECT_UKM(UkmEntry::kIsBackgroundName, properties->is_background);
    EXPECT_UKM(UkmEntry::kIsMutedName, properties->is_muted);
    EXPECT_UKM(UkmEntry::kAudioCodecName, properties->audio_codec);
    EXPECT_UKM(UkmEntry::kVideoCodecName, properties->video_codec);
    EXPECT_UKM(UkmEntry::kHasAudioName, properties->has_audio);
    EXPECT_UKM(UkmEntry::kHasVideoName, properties->has_video);
    EXPECT_UKM(UkmEntry::kIsEMEName, properties->is_eme);
    EXPECT_UKM(UkmEntry::kIsMSEName, properties->is_mse);
    EXPECT_UKM(UkmEntry::kLastPipelineStatusName, PIPELINE_OK);
    EXPECT_UKM(UkmEntry::kRebuffersCountName, 0);
    EXPECT_UKM(UkmEntry::kVideoNaturalWidthName,
               properties->natural_size.width());
    EXPECT_UKM(UkmEntry::kVideoNaturalHeightName,
               properties->natural_size.height());
    EXPECT_HAS_UKM(UkmEntry::kPlayerIDName);
    EXPECT_UKM(UkmEntry::kAudioDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kVideoDecoderNameName, 0);
    EXPECT_UKM(UkmEntry::kAutoplayInitiatedName, false);

    EXPECT_NO_UKM(UkmEntry::kMeanTimeBetweenRebuffersName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_ACName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_BatteryName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOnName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_NativeControlsOffName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayFullscreenName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayInlineName);
    EXPECT_NO_UKM(UkmEntry::kWatchTime_DisplayPictureInPictureName);
  }
}

#undef EXPECT_UKM
#undef EXPECT_NO_UKM
#undef EXPECT_HAS_UKM

TEST_F(WatchTimeRecorderTest, DISABLED_PrintExpectedDecoderNameHashes) {
  const std::string kDecoderNames[] = {
      "FFmpegAudioDecoder", "FFmpegVideoDecoder",     "GpuVideoDecoder",
      "MojoVideoDecoder",   "MojoAudioDecoder",       "VpxVideoDecoder",
      "AomVideoDecoder",    "DecryptingAudioDecoder", "DecryptingVideoDecoder"};
  printf("%18s = 0\n", "None");
  for (const auto& name : kDecoderNames)
    printf("%18s = 0x%x\n", name.c_str(), base::PersistentHash(name));
}

}  // namespace media

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_impl.h"

#include <map>
#include <memory>

#include "base/metrics/histogram_samples.h"
#include "base/test/metrics/histogram_tester.h"
#include "content/browser/media/session/media_session_player_observer.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "media/base/media_content_type.h"
#include "third_party/blink/public/platform/modules/mediasession/media_session.mojom.h"

namespace content {

using MediaSessionUserAction = MediaSessionUmaHelper::MediaSessionUserAction;
using SuspendType = MediaSession::SuspendType;
using MediaSessionAction = blink::mojom::MediaSessionAction;

namespace {

static const int kPlayerId = 0;

class MockMediaSessionPlayerObserver : public MediaSessionPlayerObserver {
 public:
  explicit MockMediaSessionPlayerObserver(RenderFrameHost* rfh)
      : render_frame_host_(rfh) {}

  ~MockMediaSessionPlayerObserver() override = default;

  void OnSuspend(int player_id) override {}
  void OnResume(int player_id) override {}
  void OnSeekForward(int player_id, base::TimeDelta seek_time) override {}
  void OnSeekBackward(int player_id, base::TimeDelta seek_time) override {}
  void OnSetVolumeMultiplier(int player_id, double volume_multiplier) override {
  }

  RenderFrameHost* render_frame_host() const override {
    return render_frame_host_;
  }

 private:
  RenderFrameHost* render_frame_host_;
};

struct ActionMappingEntry {
  blink::mojom::MediaSessionAction action;
  MediaSessionUserAction user_action;
};

ActionMappingEntry kActionMappings[] = {
    {MediaSessionAction::PLAY, MediaSessionUserAction::Play},
    {MediaSessionAction::PAUSE, MediaSessionUserAction::Pause},
    {MediaSessionAction::PREVIOUS_TRACK, MediaSessionUserAction::PreviousTrack},
    {MediaSessionAction::NEXT_TRACK, MediaSessionUserAction::NextTrack},
    {MediaSessionAction::SEEK_BACKWARD, MediaSessionUserAction::SeekBackward},
    {MediaSessionAction::SEEK_FORWARD, MediaSessionUserAction::SeekForward},
};

}  // anonymous namespace

class MediaSessionImplUmaTest : public RenderViewHostImplTestHarness {
 public:
  MediaSessionImplUmaTest() = default;
  ~MediaSessionImplUmaTest() override = default;

  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    contents()->GetMainFrame()->InitializeRenderFrameIfNeeded();
    StartPlayer();
  }

  void TearDown() override { RenderViewHostImplTestHarness::TearDown(); }

 protected:
  MediaSessionImpl* GetSession() { return MediaSessionImpl::Get(contents()); }

  void StartPlayer() {
    player_.reset(
        new MockMediaSessionPlayerObserver(contents()->GetMainFrame()));
    GetSession()->AddPlayer(player_.get(), kPlayerId,
                            media::MediaContentType::Persistent);
  }

  std::unique_ptr<base::HistogramSamples> GetHistogramSamplesSinceTestStart(
      const std::string& name) {
    return histogram_tester_.GetHistogramSamplesSinceCreation(name);
  }

  std::unique_ptr<MockMediaSessionPlayerObserver> player_;
  base::HistogramTester histogram_tester_;
};

TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnUISuspend) {
  GetSession()->Suspend(SuspendType::UI);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(static_cast<base::HistogramBase::Sample>(
                   MediaSessionUserAction::PauseDefault)));
}

TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnSystemSuspend) {
  GetSession()->Suspend(SuspendType::SYSTEM);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(0, samples->TotalCount());
}

TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnContentSuspend) {
  GetSession()->Suspend(SuspendType::CONTENT);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(0, samples->TotalCount());
}

TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnUIResume) {
  GetSession()->Suspend(SuspendType::SYSTEM);
  GetSession()->Resume(SuspendType::UI);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(static_cast<base::HistogramBase::Sample>(
                   MediaSessionUserAction::PlayDefault)));
}

TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnSystemResume) {
  GetSession()->Suspend(SuspendType::SYSTEM);
  GetSession()->Resume(SuspendType::SYSTEM);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(0, samples->TotalCount());
}

// This should never happen but just check this to be safe.
TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnContentResume) {
  GetSession()->Suspend(SuspendType::SYSTEM);
  GetSession()->Resume(SuspendType::CONTENT);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(0, samples->TotalCount());
}

TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnUIStop) {
  GetSession()->Stop(SuspendType::UI);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(static_cast<base::HistogramBase::Sample>(
                   MediaSessionUserAction::StopDefault)));
}

// This should never happen but just check this to be safe.
TEST_F(MediaSessionImplUmaTest, RecordPauseDefaultOnSystemStop) {
  GetSession()->Stop(SuspendType::SYSTEM);
  std::unique_ptr<base::HistogramSamples> samples(
      GetHistogramSamplesSinceTestStart("Media.Session.UserAction"));
  EXPECT_EQ(0, samples->TotalCount());
}

TEST_F(MediaSessionImplUmaTest, RecordMediaSessionAction) {
  for (const auto& mapping_entry : kActionMappings) {
    // Uniquely create a HistogramTester for each action to check the histograms
    // for each action independently.
    base::HistogramTester histogram_tester;
    GetSession()->DidReceiveAction(mapping_entry.action);

    std::unique_ptr<base::HistogramSamples> samples(
        histogram_tester.GetHistogramSamplesSinceCreation(
            "Media.Session.UserAction"));
    EXPECT_EQ(1, samples->TotalCount());
    EXPECT_EQ(1, samples->GetCount(static_cast<base::HistogramBase::Sample>(
                     mapping_entry.user_action)));
  }
}

}  // namespace content

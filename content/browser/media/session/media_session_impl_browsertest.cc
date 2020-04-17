// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/media_session_impl.h"

#include <stddef.h>

#include <list>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_samples.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/simple_test_tick_clock.h"
#include "content/browser/media/session/audio_focus_delegate.h"
#include "content/browser/media/session/media_session_service_impl.h"
#include "content/browser/media/session/mock_media_session_observer.h"
#include "content/browser/media/session/mock_media_session_player_observer.h"
#include "content/public/browser/media_session.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/content_browser_test.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_content_type.h"
#include "testing/gmock/include/gmock/gmock.h"

using content::WebContents;
using content::MediaSession;
using content::MediaSessionImpl;
using content::MediaSessionObserver;
using content::AudioFocusDelegate;
using content::MediaSessionPlayerObserver;
using content::MediaSessionUmaHelper;
using content::MockMediaSessionPlayerObserver;

using ::testing::Eq;
using ::testing::Expectation;
using ::testing::NiceMock;
using ::testing::_;

namespace {

const double kDefaultVolumeMultiplier = 1.0;
const double kDuckingVolumeMultiplier = 0.2;
const double kDifferentDuckingVolumeMultiplier = 0.018;

class MockAudioFocusDelegate : public AudioFocusDelegate {
 public:
  MockAudioFocusDelegate() {
    ON_CALL(*this, RequestAudioFocus(_)).WillByDefault(::testing::Return(true));
  }

  MOCK_METHOD1(RequestAudioFocus,
               bool(content::AudioFocusManager::AudioFocusType));
  MOCK_METHOD0(AbandonAudioFocus, void());
};

class MockMediaSessionServiceImpl : public content::MediaSessionServiceImpl {
 public:
  explicit MockMediaSessionServiceImpl(content::RenderFrameHost* rfh)
      : MediaSessionServiceImpl(rfh) {}
  ~MockMediaSessionServiceImpl() override = default;
};

}  // namespace

class MediaSessionImplBrowserTest : public content::ContentBrowserTest {
 protected:
  MediaSessionImplBrowserTest() = default;

  void SetUpOnMainThread() override {
    ContentBrowserTest::SetUpOnMainThread();

    media_session_ = MediaSessionImpl::Get(shell()->web_contents());
    mock_media_session_observer_.reset(
        new NiceMock<content::MockMediaSessionObserver>(media_session_));
    mock_audio_focus_delegate_ = new NiceMock<MockAudioFocusDelegate>;
    media_session_->SetDelegateForTests(
        base::WrapUnique(mock_audio_focus_delegate_));
    ASSERT_TRUE(media_session_);
  }

  void TearDownOnMainThread() override {
    mock_media_session_observer_.reset();
    media_session_->RemoveAllPlayersForTest();
    mock_media_session_service_.reset();

    media_session_ = nullptr;

    ContentBrowserTest::TearDownOnMainThread();
  }

  void StartNewPlayer(MockMediaSessionPlayerObserver* player_observer,
                      media::MediaContentType media_content_type) {
    bool result = AddPlayer(player_observer, player_observer->StartNewPlayer(),
                            media_content_type);
    EXPECT_TRUE(result);
  }

  bool AddPlayer(MockMediaSessionPlayerObserver* player_observer,
                 int player_id,
                 media::MediaContentType type) {
    return media_session_->AddPlayer(player_observer, player_id, type);
  }

  void RemovePlayer(MockMediaSessionPlayerObserver* player_observer,
                    int player_id) {
    media_session_->RemovePlayer(player_observer, player_id);
  }

  void RemovePlayers(MockMediaSessionPlayerObserver* player_observer) {
    media_session_->RemovePlayers(player_observer);
  }

  void OnPlayerPaused(MockMediaSessionPlayerObserver* player_observer,
                      int player_id) {
    media_session_->OnPlayerPaused(player_observer, player_id);
  }

  bool IsActive() { return media_session_->IsActive(); }

  content::AudioFocusManager::AudioFocusType GetSessionAudioFocusType() {
    return media_session_->audio_focus_type();
  }

  bool IsControllable() { return media_session_->IsControllable(); }

  void UIResume() { media_session_->Resume(MediaSession::SuspendType::UI); }

  void SystemResume() {
    media_session_->OnResumeInternal(MediaSession::SuspendType::SYSTEM);
  }

  void UISuspend() { media_session_->Suspend(MediaSession::SuspendType::UI); }

  void SystemSuspend(bool temporary) {
    media_session_->OnSuspendInternal(MediaSession::SuspendType::SYSTEM,
                                      temporary
                                          ? MediaSessionImpl::State::SUSPENDED
                                          : MediaSessionImpl::State::INACTIVE);
  }

  void UISeekForward() {
    media_session_->SeekForward(base::TimeDelta::FromSeconds(1));
  }

  void UISeekBackward() {
    media_session_->SeekBackward(base::TimeDelta::FromSeconds(1));
  }

  void SystemStartDucking() { media_session_->StartDucking(); }

  void SystemStopDucking() { media_session_->StopDucking(); }

  void EnsureMediaSessionService() {
    mock_media_session_service_.reset(new NiceMock<MockMediaSessionServiceImpl>(
        shell()->web_contents()->GetMainFrame()));
  }

  void SetPlaybackState(blink::mojom::MediaSessionPlaybackState state) {
    mock_media_session_service_->SetPlaybackState(state);
  }

  content::MockMediaSessionObserver* mock_media_session_observer() {
    return mock_media_session_observer_.get();
  }

  MockAudioFocusDelegate* mock_audio_focus_delegate() {
    return mock_audio_focus_delegate_;
  }

  std::unique_ptr<MediaSessionImpl> CreateDummyMediaSession() {
    return base::WrapUnique<MediaSessionImpl>(new MediaSessionImpl(nullptr));
  }

  MediaSessionUmaHelper* GetMediaSessionUMAHelper() {
    return media_session_->uma_helper_for_test();
  }

 protected:
  MediaSessionImpl* media_session_;
  std::unique_ptr<content::MockMediaSessionObserver>
      mock_media_session_observer_;
  MockAudioFocusDelegate* mock_audio_focus_delegate_;
  std::unique_ptr<MockMediaSessionServiceImpl> mock_media_session_service_;

  DISALLOW_COPY_AND_ASSIGN(MediaSessionImplBrowserTest);
};

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       PlayersFromSameObserverDoNotStopEachOtherInSameSession) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(player_observer->IsPlaying(0));
  EXPECT_TRUE(player_observer->IsPlaying(1));
  EXPECT_TRUE(player_observer->IsPlaying(2));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       PlayersFromManyObserverDoNotStopEachOtherInSameSession) {
  auto player_observer_1 = std::make_unique<MockMediaSessionPlayerObserver>();
  auto player_observer_2 = std::make_unique<MockMediaSessionPlayerObserver>();
  auto player_observer_3 = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer_1.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_2.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_3.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(player_observer_1->IsPlaying(0));
  EXPECT_TRUE(player_observer_2->IsPlaying(0));
  EXPECT_TRUE(player_observer_3->IsPlaying(0));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       SuspendedMediaSessionStopsPlayers) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);

  EXPECT_FALSE(player_observer->IsPlaying(0));
  EXPECT_FALSE(player_observer->IsPlaying(1));
  EXPECT_FALSE(player_observer->IsPlaying(2));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ResumedMediaSessionRestartsPlayers) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);
  SystemResume();

  EXPECT_TRUE(player_observer->IsPlaying(0));
  EXPECT_TRUE(player_observer->IsPlaying(1));
  EXPECT_TRUE(player_observer->IsPlaying(2));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       StartedPlayerOnSuspendedSessionPlaysAlone) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(player_observer->IsPlaying(0));

  SystemSuspend(true);

  EXPECT_FALSE(player_observer->IsPlaying(0));

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_FALSE(player_observer->IsPlaying(0));
  EXPECT_TRUE(player_observer->IsPlaying(1));

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_FALSE(player_observer->IsPlaying(0));
  EXPECT_TRUE(player_observer->IsPlaying(1));
  EXPECT_TRUE(player_observer->IsPlaying(2));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, InitialVolumeMultiplier) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(0));
  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(1));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       StartDuckingReducesVolumeMultiplier) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemStartDucking();

  EXPECT_EQ(kDuckingVolumeMultiplier, player_observer->GetVolumeMultiplier(0));
  EXPECT_EQ(kDuckingVolumeMultiplier, player_observer->GetVolumeMultiplier(1));

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_EQ(kDuckingVolumeMultiplier, player_observer->GetVolumeMultiplier(2));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       StopDuckingRecoversVolumeMultiplier) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemStartDucking();
  SystemStopDucking();

  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(0));
  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(1));

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(2));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       DuckingUsesConfiguredMultiplier) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  media_session_->SetDuckingVolumeMultiplier(kDifferentDuckingVolumeMultiplier);
  SystemStartDucking();
  EXPECT_EQ(kDifferentDuckingVolumeMultiplier,
            player_observer->GetVolumeMultiplier(0));
  EXPECT_EQ(kDifferentDuckingVolumeMultiplier,
            player_observer->GetVolumeMultiplier(1));
  SystemStopDucking();
  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(0));
  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(1));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, AudioFocusInitialState) {
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       AddPlayerOnSuspendedFocusUnducks) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  UISuspend();
  EXPECT_FALSE(IsActive());

  SystemStartDucking();
  EXPECT_EQ(kDuckingVolumeMultiplier, player_observer->GetVolumeMultiplier(0));

  EXPECT_TRUE(
      AddPlayer(player_observer.get(), 0, media::MediaContentType::Persistent));
  EXPECT_EQ(kDefaultVolumeMultiplier, player_observer->GetVolumeMultiplier(0));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       CanRequestFocusBeforePlayerCreation) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  media_session_->RequestSystemAudioFocus(
      content::AudioFocusManager::AudioFocusType::Gain);
  EXPECT_TRUE(IsActive());

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, StartPlayerGivesFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       SuspendGivesAwayAudioFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);

  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, StopGivesAwayAudioFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  media_session_->Stop(MediaSession::SuspendType::UI);

  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, ResumeGivesBackAudioFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);
  SystemResume();

  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       RemovingLastPlayerDropsAudioFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayer(player_observer.get(), 0);
  EXPECT_TRUE(IsActive());
  RemovePlayer(player_observer.get(), 1);
  EXPECT_TRUE(IsActive());
  RemovePlayer(player_observer.get(), 2);
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       RemovingLastPlayerFromManyObserversDropsAudioFocus) {
  auto player_observer_1 = std::make_unique<MockMediaSessionPlayerObserver>();
  auto player_observer_2 = std::make_unique<MockMediaSessionPlayerObserver>();
  auto player_observer_3 = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer_1.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_2.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_3.get(), media::MediaContentType::Persistent);

  RemovePlayer(player_observer_1.get(), 0);
  EXPECT_TRUE(IsActive());
  RemovePlayer(player_observer_2.get(), 0);
  EXPECT_TRUE(IsActive());
  RemovePlayer(player_observer_3.get(), 0);
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       RemovingAllPlayersFromObserversDropsAudioFocus) {
  auto player_observer_1 = std::make_unique<MockMediaSessionPlayerObserver>();
  auto player_observer_2 = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer_1.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_1.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_2.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer_2.get(), media::MediaContentType::Persistent);

  RemovePlayers(player_observer_1.get());
  EXPECT_TRUE(IsActive());
  RemovePlayers(player_observer_2.get());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, ResumePlayGivesAudioFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayer(player_observer.get(), 0);
  EXPECT_FALSE(IsActive());

  EXPECT_TRUE(
      AddPlayer(player_observer.get(), 0, media::MediaContentType::Persistent));
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ResumeSuspendSeekAreSentOnlyOncePerPlayers) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_EQ(0, player_observer->received_suspend_calls());
  EXPECT_EQ(0, player_observer->received_resume_calls());
  EXPECT_EQ(0, player_observer->received_seek_forward_calls());
  EXPECT_EQ(0, player_observer->received_seek_backward_calls());

  SystemSuspend(true);
  EXPECT_EQ(3, player_observer->received_suspend_calls());

  SystemResume();
  EXPECT_EQ(3, player_observer->received_resume_calls());

  UISeekForward();
  EXPECT_EQ(3, player_observer->received_seek_forward_calls());

  UISeekBackward();
  EXPECT_EQ(3, player_observer->received_seek_backward_calls());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ResumeSuspendSeekAreSentOnlyOncePerPlayersAddedTwice) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  // Adding the three players above again.
  EXPECT_TRUE(
      AddPlayer(player_observer.get(), 0, media::MediaContentType::Persistent));
  EXPECT_TRUE(
      AddPlayer(player_observer.get(), 1, media::MediaContentType::Persistent));
  EXPECT_TRUE(
      AddPlayer(player_observer.get(), 2, media::MediaContentType::Persistent));

  EXPECT_EQ(0, player_observer->received_suspend_calls());
  EXPECT_EQ(0, player_observer->received_resume_calls());
  EXPECT_EQ(0, player_observer->received_seek_forward_calls());
  EXPECT_EQ(0, player_observer->received_seek_backward_calls());

  SystemSuspend(true);
  EXPECT_EQ(3, player_observer->received_suspend_calls());

  SystemResume();
  EXPECT_EQ(3, player_observer->received_resume_calls());

  UISeekForward();
  EXPECT_EQ(3, player_observer->received_seek_forward_calls());

  UISeekBackward();
  EXPECT_EQ(3, player_observer->received_seek_backward_calls());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       RemovingTheSamePlayerTwiceIsANoop) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayer(player_observer.get(), 0);
  RemovePlayer(player_observer.get(), 0);
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, AudioFocusType) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  // Starting a player with a given type should set the session to that type.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  EXPECT_EQ(content::AudioFocusManager::AudioFocusType::GainTransientMayDuck,
            GetSessionAudioFocusType());

  // Adding a player of the same type should have no effect on the type.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  EXPECT_EQ(content::AudioFocusManager::AudioFocusType::GainTransientMayDuck,
            GetSessionAudioFocusType());

  // Adding a player of Content type should override the current type.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  EXPECT_EQ(content::AudioFocusManager::AudioFocusType::Gain,
            GetSessionAudioFocusType());

  // Adding a player of the Transient type should have no effect on the type.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  EXPECT_EQ(content::AudioFocusManager::AudioFocusType::Gain,
            GetSessionAudioFocusType());

  EXPECT_TRUE(player_observer->IsPlaying(0));
  EXPECT_TRUE(player_observer->IsPlaying(1));
  EXPECT_TRUE(player_observer->IsPlaying(2));
  EXPECT_TRUE(player_observer->IsPlaying(3));

  SystemSuspend(true);

  EXPECT_FALSE(player_observer->IsPlaying(0));
  EXPECT_FALSE(player_observer->IsPlaying(1));
  EXPECT_FALSE(player_observer->IsPlaying(2));
  EXPECT_FALSE(player_observer->IsPlaying(3));

  EXPECT_EQ(content::AudioFocusManager::AudioFocusType::Gain,
            GetSessionAudioFocusType());

  SystemResume();

  EXPECT_TRUE(player_observer->IsPlaying(0));
  EXPECT_TRUE(player_observer->IsPlaying(1));
  EXPECT_TRUE(player_observer->IsPlaying(2));
  EXPECT_TRUE(player_observer->IsPlaying(3));

  EXPECT_EQ(content::AudioFocusManager::AudioFocusType::Gain,
            GetSessionAudioFocusType());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, ControlsShowForContent) {
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false));

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  // Starting a player with a content type should show the media controls.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsNoShowForTransient) {
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, false));

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  // Starting a player with a transient type should not show the media controls.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);

  EXPECT_FALSE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, ControlsHideWhenStopped) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayers(player_observer.get());

  EXPECT_FALSE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsShownAcceptTransient) {
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false));

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  // Transient player join the session without affecting the controls.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsShownAfterContentAdded) {
  Expectation dontShowControls = EXPECT_CALL(
      *mock_media_session_observer(), MediaSessionStateChanged(false, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .After(dontShowControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);

  // The controls are shown when the content player is added.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsStayIfOnlyOnePlayerHasBeenPaused) {
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false));

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);

  // Removing only content player doesn't hide the controls since the session
  // is still active.
  RemovePlayer(player_observer.get(), 0);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsHideWhenTheLastPlayerIsRemoved) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayer(player_observer.get(), 0);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());

  RemovePlayer(player_observer.get(), 1);

  EXPECT_FALSE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsHideWhenAllThePlayersAreRemoved) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayers(player_observer.get());

  EXPECT_FALSE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsNotHideWhenTheLastPlayerIsPaused) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  OnPlayerPaused(player_observer.get(), 0);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());

  OnPlayerPaused(player_observer.get(), 1);

  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       SuspendTemporaryUpdatesControls) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);

  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsUpdatedWhenResumed) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  Expectation pauseControls = EXPECT_CALL(*mock_media_session_observer(),
                                          MediaSessionStateChanged(true, true))
                                  .After(showControls);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .After(pauseControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemSuspend(true);
  SystemResume();

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsHideWhenSessionSuspendedPermanently) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(false);

  EXPECT_FALSE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ConstrolsHideWhenSessionStops) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  Expectation pauseControls = EXPECT_CALL(*mock_media_session_observer(),
                                          MediaSessionStateChanged(true, true))
                                  .After(showControls);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, true))
      .After(pauseControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  media_session_->Stop(MediaSession::SuspendType::UI);

  EXPECT_FALSE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsHideWhenSessionChangesFromContentToTransient) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  Expectation pauseControls = EXPECT_CALL(*mock_media_session_observer(),
                                          MediaSessionStateChanged(true, true))
                                  .After(showControls);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, false))
      .After(pauseControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemSuspend(true);

  // This should reset the session and change it to a transient, so
  // hide the controls.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);

  EXPECT_FALSE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsUpdatedWhenNewPlayerResetsSession) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  Expectation pauseControls = EXPECT_CALL(*mock_media_session_observer(),
                                          MediaSessionStateChanged(true, true))
                                  .After(showControls);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .After(pauseControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemSuspend(true);

  // This should reset the session and update the controls.
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsResumedWhenPlayerIsResumed) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  Expectation pauseControls = EXPECT_CALL(*mock_media_session_observer(),
                                          MediaSessionStateChanged(true, true))
                                  .After(showControls);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .After(pauseControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemSuspend(true);

  // This should resume the session and update the controls.
  AddPlayer(player_observer.get(), 0, media::MediaContentType::Persistent);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsUpdatedDueToResumeSessionAction) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, true))
      .After(showControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  UISuspend();

  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsUpdatedDueToSuspendSessionAction) {
  Expectation showControls = EXPECT_CALL(*mock_media_session_observer(),
                                         MediaSessionStateChanged(true, false));
  Expectation pauseControls = EXPECT_CALL(*mock_media_session_observer(),
                                          MediaSessionStateChanged(true, true))
                                  .After(showControls);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .After(pauseControls);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  UISuspend();
  UIResume();

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsDontShowWhenOneShotIsPresent) {
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, false));

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);

  EXPECT_FALSE(IsControllable());
  EXPECT_TRUE(IsActive());

  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  EXPECT_FALSE(IsControllable());
  EXPECT_TRUE(IsActive());

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  EXPECT_FALSE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsHiddenAfterRemoveOneShotWithoutOtherPlayers) {
  Expectation expect_1 = EXPECT_CALL(*mock_media_session_observer(),
                                     MediaSessionStateChanged(false, false));
  Expectation expect_2 = EXPECT_CALL(*mock_media_session_observer(),
                                     MediaSessionStateChanged(false, true))
                             .After(expect_1);
  EXPECT_CALL(*mock_media_session_observer(), MediaSessionStateChanged(true, _))
      .Times(0)
      .After(expect_2);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);
  RemovePlayer(player_observer.get(), 0);

  EXPECT_FALSE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ControlsShowAfterRemoveOneShotWithPersistentPresent) {
  Expectation uncontrollable = EXPECT_CALL(
      *mock_media_session_observer(), MediaSessionStateChanged(false, false));

  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .After(uncontrollable);

  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  RemovePlayer(player_observer.get(), 0);

  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       DontSuspendWhenOneShotIsPresent) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(false);

  EXPECT_FALSE(IsControllable());
  EXPECT_TRUE(IsActive());

  EXPECT_EQ(0, player_observer->received_suspend_calls());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       DontResumeBySystemUISuspendedSessions) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  UISuspend();
  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());

  SystemResume();
  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       AllowUIResumeForSystemSuspend) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);
  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());

  UIResume();
  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, ResumeSuspendFromUI) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  UISuspend();
  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());

  UIResume();
  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, ResumeSuspendFromSystem) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  SystemSuspend(true);
  EXPECT_TRUE(IsControllable());
  EXPECT_FALSE(IsActive());

  SystemResume();
  EXPECT_TRUE(IsControllable());
  EXPECT_TRUE(IsActive());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, OneShotTakesGainFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  EXPECT_CALL(
      *mock_audio_focus_delegate(),
      RequestAudioFocus(content::AudioFocusManager::AudioFocusType::Gain))
      .Times(1);
  EXPECT_CALL(*mock_audio_focus_delegate(),
              RequestAudioFocus(::testing::Ne(
                  content::AudioFocusManager::AudioFocusType::Gain)))
      .Times(0);
  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Transient);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, RemovingOneShotDropsFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  EXPECT_CALL(*mock_audio_focus_delegate(), AbandonAudioFocus());
  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);
  RemovePlayer(player_observer.get(), 0);
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       RemovingOneShotWhileStillHavingOtherPlayersKeepsFocus) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  EXPECT_CALL(*mock_audio_focus_delegate(), AbandonAudioFocus())
      .Times(1);  // Called in TearDown
  StartNewPlayer(player_observer.get(), media::MediaContentType::OneShot);
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  RemovePlayer(player_observer.get(), 0);
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ActualPlaybackStateWhilePlayerPaused) {
  EnsureMediaSessionService();
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>(
      shell()->web_contents()->GetMainFrame());

  ::testing::Sequence s;
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, true))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, true))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, true))
      .InSequence(s);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  OnPlayerPaused(player_observer.get(), 0);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::PLAYING);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::PAUSED);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::NONE);

  // Verify before test exists. Otherwise the sequence will expire and cause
  // weird problems.
  ::testing::Mock::VerifyAndClear(mock_media_session_observer());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ActualPlaybackStateWhilePlayerPlaying) {
  EnsureMediaSessionService();
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>(
      shell()->web_contents()->GetMainFrame());
  ::testing::Sequence s;
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::PLAYING);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::PAUSED);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::NONE);

  // Verify before test exists. Otherwise the sequence will expire and cause
  // weird problems.
  ::testing::Mock::VerifyAndClear(mock_media_session_observer());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       ActualPlaybackStateWhilePlayerRemoved) {
  EnsureMediaSessionService();
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>(
      shell()->web_contents()->GetMainFrame());

  ::testing::Sequence s;
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false))
      .InSequence(s);
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, _))
      .InSequence(s);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  RemovePlayer(player_observer.get(), 0);

  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::PLAYING);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::PAUSED);
  SetPlaybackState(blink::mojom::MediaSessionPlaybackState::NONE);

  // Verify before test exists. Otherwise the sequence will expire and cause
  // weird problems.
  ::testing::Mock::VerifyAndClear(mock_media_session_observer());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_Suspended_SystemTransient) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemSuspend(true);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.Suspended"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(0));  // System Transient
  EXPECT_EQ(0, samples->GetCount(1));  // System Permanent
  EXPECT_EQ(0, samples->GetCount(2));  // UI
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_Suspended_SystemPermantent) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  SystemSuspend(false);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.Suspended"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(0, samples->GetCount(0));  // System Transient
  EXPECT_EQ(1, samples->GetCount(1));  // System Permanent
  EXPECT_EQ(0, samples->GetCount(2));  // UI
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, UMA_Suspended_UI) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();

  base::HistogramTester tester;

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  UISuspend();

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.Suspended"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(0, samples->GetCount(0));  // System Transient
  EXPECT_EQ(0, samples->GetCount(1));  // System Permanent
  EXPECT_EQ(1, samples->GetCount(2));  // UI
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, UMA_Suspended_Multiple) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  UISuspend();
  UIResume();

  SystemSuspend(true);
  SystemResume();

  UISuspend();
  UIResume();

  SystemSuspend(false);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.Suspended"));
  EXPECT_EQ(4, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(0));  // System Transient
  EXPECT_EQ(1, samples->GetCount(1));  // System Permanent
  EXPECT_EQ(2, samples->GetCount(2));  // UI
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, UMA_Suspended_Crossing) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  UISuspend();
  SystemSuspend(true);
  SystemSuspend(false);
  UIResume();

  SystemSuspend(true);
  SystemSuspend(true);
  SystemSuspend(false);
  SystemResume();

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.Suspended"));
  EXPECT_EQ(2, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(0));  // System Transient
  EXPECT_EQ(0, samples->GetCount(1));  // System Permanent
  EXPECT_EQ(1, samples->GetCount(2));  // UI
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest, UMA_Suspended_Stop) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  media_session_->Stop(MediaSession::SuspendType::UI);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.Suspended"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(0, samples->GetCount(0));  // System Transient
  EXPECT_EQ(0, samples->GetCount(1));  // System Permanent
  EXPECT_EQ(1, samples->GetCount(2));  // UI
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_NoActivation) {
  base::HistogramTester tester;

  std::unique_ptr<MediaSessionImpl> media_session = CreateDummyMediaSession();
  media_session.reset();

  // A MediaSession that wasn't active doesn't register an active time.
  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
  EXPECT_EQ(0, samples->TotalCount());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_SimpleActivation) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  MediaSessionUmaHelper* media_session_uma_helper = GetMediaSessionUMAHelper();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  media_session_uma_helper->SetClockForTest(&clock);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  media_session_->Stop(MediaSession::SuspendType::UI);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(1000));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_ActivationWithUISuspension) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  MediaSessionUmaHelper* media_session_uma_helper = GetMediaSessionUMAHelper();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  media_session_uma_helper->SetClockForTest(&clock);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  UISuspend();

  clock.Advance(base::TimeDelta::FromMilliseconds(2000));
  UIResume();

  clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  media_session_->Stop(MediaSession::SuspendType::UI);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(2000));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_ActivationWithSystemSuspension) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  MediaSessionUmaHelper* media_session_uma_helper = GetMediaSessionUMAHelper();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  media_session_uma_helper->SetClockForTest(&clock);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  SystemSuspend(true);

  clock.Advance(base::TimeDelta::FromMilliseconds(2000));
  SystemResume();

  clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  media_session_->Stop(MediaSession::SuspendType::UI);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
  EXPECT_EQ(1, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(2000));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_ActivateSuspendedButNotStopped) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  MediaSessionUmaHelper* media_session_uma_helper = GetMediaSessionUMAHelper();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  media_session_uma_helper->SetClockForTest(&clock);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  clock.Advance(base::TimeDelta::FromMilliseconds(500));
  SystemSuspend(true);

  {
    std::unique_ptr<base::HistogramSamples> samples(
        tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
    EXPECT_EQ(0, samples->TotalCount());
  }

  SystemResume();
  clock.Advance(base::TimeDelta::FromMilliseconds(5000));
  UISuspend();

  {
    std::unique_ptr<base::HistogramSamples> samples(
        tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
    EXPECT_EQ(0, samples->TotalCount());
  }
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_ActivateSuspendStopTwice) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  MediaSessionUmaHelper* media_session_uma_helper = GetMediaSessionUMAHelper();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  media_session_uma_helper->SetClockForTest(&clock);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  clock.Advance(base::TimeDelta::FromMilliseconds(500));
  SystemSuspend(true);
  media_session_->Stop(MediaSession::SuspendType::UI);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  clock.Advance(base::TimeDelta::FromMilliseconds(5000));
  SystemResume();
  media_session_->Stop(MediaSession::SuspendType::UI);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
  EXPECT_EQ(2, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(500));
  EXPECT_EQ(1, samples->GetCount(5000));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       UMA_ActiveTime_MultipleActivations) {
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>();
  base::HistogramTester tester;

  MediaSessionUmaHelper* media_session_uma_helper = GetMediaSessionUMAHelper();
  base::SimpleTestTickClock clock;
  clock.SetNowTicks(base::TimeTicks::Now());
  media_session_uma_helper->SetClockForTest(&clock);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  clock.Advance(base::TimeDelta::FromMilliseconds(10000));
  RemovePlayer(player_observer.get(), 0);

  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);
  clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  media_session_->Stop(MediaSession::SuspendType::UI);

  std::unique_ptr<base::HistogramSamples> samples(
      tester.GetHistogramSamplesSinceCreation("Media.Session.ActiveTime"));
  EXPECT_EQ(2, samples->TotalCount());
  EXPECT_EQ(1, samples->GetCount(1000));
  EXPECT_EQ(1, samples->GetCount(10000));
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       AddingObserverNotifiesCurrentInformation_EmptyInfo) {
  media_session_->RemoveObserver(mock_media_session_observer());
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(false, true));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionMetadataChanged(Eq(base::nullopt)));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionActionsChanged(
                  Eq(std::set<blink::mojom::MediaSessionAction>())));
  media_session_->AddObserver(mock_media_session_observer());
}

IN_PROC_BROWSER_TEST_F(MediaSessionImplBrowserTest,
                       AddingObserverNotifiesCurrentInformation_WithInfo) {
  // Set up the service and information.
  EnsureMediaSessionService();

  content::MediaMetadata metadata;
  metadata.title = base::ASCIIToUTF16("title");
  metadata.artist = base::ASCIIToUTF16("artist");
  metadata.album = base::ASCIIToUTF16("album");
  mock_media_session_service_->SetMetadata(metadata);

  mock_media_session_service_->EnableAction(
      blink::mojom::MediaSessionAction::PLAY);
  std::set<blink::mojom::MediaSessionAction> expectedActions =
      mock_media_session_service_->actions();

  // Make sure the service is routed,
  auto player_observer = std::make_unique<MockMediaSessionPlayerObserver>(
      shell()->web_contents()->GetMainFrame());
  StartNewPlayer(player_observer.get(), media::MediaContentType::Persistent);

  // Check if the expectations are met when the observer is newly added.
  media_session_->RemoveObserver(mock_media_session_observer());
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionStateChanged(true, false));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionMetadataChanged(Eq(metadata)));
  EXPECT_CALL(*mock_media_session_observer(),
              MediaSessionActionsChanged(Eq(expectedActions)));
  media_session_->AddObserver(mock_media_session_observer());
}

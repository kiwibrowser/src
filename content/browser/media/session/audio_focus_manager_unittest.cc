// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/session/audio_focus_manager.h"

#include <memory>

#include "base/command_line.h"
#include "base/run_loop.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/browser/media/session/media_session_player_observer.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/test_web_contents.h"
#include "media/base/media_content_type.h"
#include "media/base/media_switches.h"

namespace content {

namespace {

class MockMediaSessionPlayerObserver : public MediaSessionPlayerObserver {
 public:
  void OnSuspend(int player_id) override {}
  void OnResume(int player_id) override {}
  void OnSeekForward(int player_id, base::TimeDelta seek_time) override {}
  void OnSeekBackward(int player_id, base::TimeDelta seek_time) override {}
  void OnSetVolumeMultiplier(
      int player_id, double volume_multiplier) override {}
  RenderFrameHost* render_frame_host() const override { return nullptr; }
};

}  // anonymous namespace

using AudioFocusType = AudioFocusManager::AudioFocusType;
using SuspendType = MediaSession::SuspendType;

class AudioFocusManagerTest : public testing::Test {
 public:
  AudioFocusManagerTest() = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kEnableAudioFocus);
    rph_factory_.reset(new MockRenderProcessHostFactory());
    RenderProcessHostImpl::set_render_process_host_factory_for_testing(
        rph_factory_.get());
    browser_context_.reset(new TestBrowserContext());
    pepper_observer_.reset(new MockMediaSessionPlayerObserver());
  }

  void TearDown() override {
    // Run pending tasks.
    base::RunLoop().RunUntilIdle();

    browser_context_.reset();
    RenderProcessHostImpl::set_render_process_host_factory_for_testing(nullptr);
    rph_factory_.reset();
  }

  MediaSessionImpl* GetAudioFocusedSession() const {
    const auto& audio_focus_stack =
        AudioFocusManager::GetInstance()->audio_focus_stack_;
    for (auto iter = audio_focus_stack.rbegin();
         iter != audio_focus_stack.rend(); ++iter) {
      if ((*iter)->audio_focus_type() ==
          AudioFocusManager::AudioFocusType::Gain)
        return (*iter);
    }
    return nullptr;
  }

  int GetTransientMaybeDuckCount() const {
    int count = 0;
    const auto& audio_focus_stack =
        AudioFocusManager::GetInstance()->audio_focus_stack_;
    for (auto iter = audio_focus_stack.rbegin();
         iter != audio_focus_stack.rend(); ++iter) {
      if ((*iter)->audio_focus_type() ==
          AudioFocusManager::AudioFocusType::GainTransientMayDuck) {
          ++count;
      } else {
        break;
      }
    }

    return count;
  }

  double IsSessionDucking(MediaSessionImpl* session) {
    return session->is_ducking_;  // Quack! Quack!
  }

  void RequestAudioFocus(MediaSessionImpl* session,
                         AudioFocusManager::AudioFocusType audio_focus_type) {
    session->RequestSystemAudioFocus(audio_focus_type);
  }

  void AbandonAudioFocus(MediaSessionImpl* session) {
    session->AbandonSystemAudioFocusIfNeeded();
  }

  std::unique_ptr<WebContents> CreateWebContents() {
    return TestWebContents::Create(browser_context_.get(),
        SiteInstance::SiteInstance::Create(browser_context_.get()));
  }

  std::unique_ptr<MediaSessionPlayerObserver> pepper_observer_;

 private:
  TestBrowserThreadBundle test_browser_thread_bundle_;

  std::unique_ptr<MockRenderProcessHostFactory> rph_factory_;
  std::unique_ptr<TestBrowserContext> browser_context_;
};

TEST_F(AudioFocusManagerTest, InstanceAvailableAndSame) {
  AudioFocusManager* audio_focus_manager = AudioFocusManager::GetInstance();
  ASSERT_TRUE(!!audio_focus_manager);
  ASSERT_EQ(audio_focus_manager, AudioFocusManager::GetInstance());
}

TEST_F(AudioFocusManagerTest, RequestAudioFocusGain_ReplaceFocusedEntry) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  std::unique_ptr<WebContents> web_contents_3(CreateWebContents());
  MediaSessionImpl* media_session_3 =
      MediaSessionImpl::Get(web_contents_3.get());

  ASSERT_EQ(nullptr, GetAudioFocusedSession());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session_1, GetAudioFocusedSession());

  RequestAudioFocus(media_session_2, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session_2, GetAudioFocusedSession());

  RequestAudioFocus(media_session_3, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session_3, GetAudioFocusedSession());
}

TEST_F(AudioFocusManagerTest, RequestAudioFocusGain_Duplicate) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  ASSERT_EQ(nullptr, GetAudioFocusedSession());

  RequestAudioFocus(media_session, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session, GetAudioFocusedSession());

  RequestAudioFocus(media_session, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session, GetAudioFocusedSession());
}

TEST_F(AudioFocusManagerTest, RequestAudioFocusGain_FromTransient) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  RequestAudioFocus(
      media_session, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(nullptr, GetAudioFocusedSession());
  ASSERT_EQ(1, GetTransientMaybeDuckCount());

  RequestAudioFocus(media_session, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session, GetAudioFocusedSession());
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
}

TEST_F(AudioFocusManagerTest, RequestAudioFocusTransient_FromGain) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  RequestAudioFocus(media_session, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session, GetAudioFocusedSession());
  ASSERT_EQ(0, GetTransientMaybeDuckCount());

  RequestAudioFocus(
      media_session, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(nullptr, GetAudioFocusedSession());
  ASSERT_EQ(1, GetTransientMaybeDuckCount());
  ASSERT_FALSE(IsSessionDucking(media_session));
}

TEST_F(AudioFocusManagerTest, RequestAudioFocusTransient_FromGainWhileDucking) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
  ASSERT_FALSE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(1, GetTransientMaybeDuckCount());
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_1, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(2, GetTransientMaybeDuckCount());
  ASSERT_FALSE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, AbandonAudioFocus_RemovesFocusedEntry) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  RequestAudioFocus(media_session, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session, GetAudioFocusedSession());

  AbandonAudioFocus(media_session);
  ASSERT_EQ(nullptr, GetAudioFocusedSession());
}

TEST_F(AudioFocusManagerTest, AbandonAudioFocus_NoAssociatedEntry) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  AbandonAudioFocus(media_session);
  ASSERT_EQ(nullptr, GetAudioFocusedSession());
}

TEST_F(AudioFocusManagerTest, AbandonAudioFocus_RemovesTransientEntry) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  RequestAudioFocus(
      media_session, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(1, GetTransientMaybeDuckCount());

  AbandonAudioFocus(media_session);
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
}

TEST_F(AudioFocusManagerTest, AbandonAudioFocus_WhileDuckingThenResume) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
  ASSERT_FALSE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(1, GetTransientMaybeDuckCount());
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  AbandonAudioFocus(media_session_1);
  ASSERT_EQ(1, GetTransientMaybeDuckCount());

  AbandonAudioFocus(media_session_2);
  ASSERT_EQ(0, GetTransientMaybeDuckCount());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_FALSE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, AbandonAudioFocus_StopsDucking) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
  ASSERT_FALSE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(1, GetTransientMaybeDuckCount());
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  AbandonAudioFocus(media_session_2);
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
  ASSERT_FALSE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, DuckWhilePlaying) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_FALSE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_TRUE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, GainSuspendsTransient) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_TRUE(media_session_2->IsSuspended());
}

TEST_F(AudioFocusManagerTest, DuckWithMultipleTransients) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  std::unique_ptr<WebContents> web_contents_3(CreateWebContents());
  MediaSessionImpl* media_session_3 =
      MediaSessionImpl::Get(web_contents_3.get());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_FALSE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_3, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  AbandonAudioFocus(media_session_2);
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  AbandonAudioFocus(media_session_3);
  ASSERT_FALSE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, WebContentsDestroyed_ReleasesFocus) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  RequestAudioFocus(media_session, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_EQ(media_session, GetAudioFocusedSession());

  web_contents.reset();
  ASSERT_EQ(nullptr, GetAudioFocusedSession());
}

TEST_F(AudioFocusManagerTest, WebContentsDestroyed_ReleasesTransients) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  RequestAudioFocus(
      media_session, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_EQ(1, GetTransientMaybeDuckCount());

  web_contents.reset();
  ASSERT_EQ(0, GetTransientMaybeDuckCount());
}

TEST_F(AudioFocusManagerTest, WebContentsDestroyed_StopsDucking) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  RequestAudioFocus(media_session_1, AudioFocusManager::AudioFocusType::Gain);
  ASSERT_FALSE(IsSessionDucking(media_session_1));

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::GainTransientMayDuck);
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  web_contents_2.reset();
  ASSERT_FALSE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, PepperRequestsGainFocus) {
  std::unique_ptr<WebContents> web_contents(CreateWebContents());
  MediaSessionImpl* media_session = MediaSessionImpl::Get(web_contents.get());

  media_session->AddPlayer(
      pepper_observer_.get(), 0, media::MediaContentType::Pepper);
  ASSERT_EQ(media_session, GetAudioFocusedSession());

  media_session->RemovePlayer(pepper_observer_.get(), 0);
  ASSERT_EQ(nullptr, GetAudioFocusedSession());
}

TEST_F(AudioFocusManagerTest, GainDucksPepper) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  media_session_1->AddPlayer(
      pepper_observer_.get(), 0, media::MediaContentType::Pepper);

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::Gain);

  ASSERT_EQ(media_session_2, GetAudioFocusedSession());
  ASSERT_TRUE(media_session_1->IsActive());
  ASSERT_TRUE(IsSessionDucking(media_session_1));
}

TEST_F(AudioFocusManagerTest, AbandoningGainFocusRevokesTopMostPepperSession) {
  std::unique_ptr<WebContents> web_contents_1(CreateWebContents());
  MediaSessionImpl* media_session_1 =
      MediaSessionImpl::Get(web_contents_1.get());

  std::unique_ptr<WebContents> web_contents_2(CreateWebContents());
  MediaSessionImpl* media_session_2 =
      MediaSessionImpl::Get(web_contents_2.get());

  std::unique_ptr<WebContents> web_contents_3(CreateWebContents());
  MediaSessionImpl* media_session_3 =
      MediaSessionImpl::Get(web_contents_3.get());

  media_session_1->AddPlayer(
      pepper_observer_.get(), 0, media::MediaContentType::Pepper);

  RequestAudioFocus(
      media_session_2, AudioFocusManager::AudioFocusType::Gain);
  RequestAudioFocus(
      media_session_3, AudioFocusManager::AudioFocusType::Gain);

  ASSERT_EQ(media_session_3, GetAudioFocusedSession());
  ASSERT_TRUE(media_session_2->IsSuspended());
  ASSERT_TRUE(media_session_1->IsActive());
  ASSERT_TRUE(IsSessionDucking(media_session_1));

  AbandonAudioFocus(media_session_3);
  ASSERT_EQ(media_session_1, GetAudioFocusedSession());
}

}  // namespace content

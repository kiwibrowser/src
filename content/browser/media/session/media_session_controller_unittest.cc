// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "content/browser/media/media_web_contents_observer.h"
#include "content/browser/media/session/media_session_controller.h"
#include "content/browser/media/session/media_session_impl.h"
#include "content/common/media/media_player_delegate_messages.h"
#include "content/test/test_render_view_host.h"
#include "content/test/test_web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class MediaSessionControllerTest : public RenderViewHostImplTestHarness {
 public:
  void SetUp() override {
    RenderViewHostImplTestHarness::SetUp();
    id_ = WebContentsObserver::MediaPlayerId(contents()->GetMainFrame(), 0);
    controller_ = CreateController();
  }

  void TearDown() override {
    // Destruct the controller prior to any other teardown to avoid out of order
    // destruction relative to the MediaSession instance.
    controller_.reset();
    RenderViewHostImplTestHarness::TearDown();
  }

 protected:
  std::unique_ptr<MediaSessionController> CreateController() {
    return std::unique_ptr<MediaSessionController>(new MediaSessionController(
        id_, contents()->media_web_contents_observer()));
  }

  MediaSessionImpl* media_session() {
    return MediaSessionImpl::Get(contents());
  }

  IPC::TestSink& test_sink() { return main_test_rfh()->GetProcess()->sink(); }

  void Suspend() {
    controller_->OnSuspend(controller_->get_player_id_for_testing());
  }

  void Resume() {
    controller_->OnResume(controller_->get_player_id_for_testing());
  }

  void SeekForward(base::TimeDelta seek_time) {
    controller_->OnSeekForward(controller_->get_player_id_for_testing(),
                               seek_time);
  }

  void SeekBackward(base::TimeDelta seek_time) {
    controller_->OnSeekBackward(controller_->get_player_id_for_testing(),
                                seek_time);
  }

  void SetVolumeMultiplier(double multiplier) {
    controller_->OnSetVolumeMultiplier(controller_->get_player_id_for_testing(),
                                       multiplier);
  }

  template <typename T>
  bool ReceivedMessagePlayPause() {
    const IPC::Message* msg = test_sink().GetUniqueMessageMatching(T::ID);
    if (!msg)
      return false;

    std::tuple<int> result;
    if (!T::Read(msg, &result))
      return false;

    EXPECT_EQ(id_.second, std::get<0>(result));
    test_sink().ClearMessages();
    return id_.second == std::get<0>(result);
  }

  template <typename T>
  bool ReceivedMessageSeek(base::TimeDelta expected_seek_time) {
    const IPC::Message* msg = test_sink().GetUniqueMessageMatching(T::ID);
    if (!msg)
      return false;

    std::tuple<int, base::TimeDelta> result;
    if (!T::Read(msg, &result))
      return false;

    EXPECT_EQ(id_.second, std::get<0>(result));
    if (id_.second != std::get<0>(result))
      return false;

    EXPECT_EQ(expected_seek_time, std::get<1>(result));
    test_sink().ClearMessages();
    return expected_seek_time == std::get<1>(result);
  }

  template <typename T>
  bool ReceivedMessageVolumeMultiplierUpdate(double expected_multiplier) {
    const IPC::Message* msg = test_sink().GetUniqueMessageMatching(T::ID);
    if (!msg)
      return false;

    std::tuple<int, double> result;
    if (!T::Read(msg, &result))
      return false;

    EXPECT_EQ(id_.second, std::get<0>(result));
    if (id_.second != std::get<0>(result))
      return false;

    EXPECT_EQ(expected_multiplier, std::get<1>(result));
    test_sink().ClearMessages();
    return expected_multiplier == std::get<1>(result);
  }

  WebContentsObserver::MediaPlayerId id_;
  std::unique_ptr<MediaSessionController> controller_;
};

TEST_F(MediaSessionControllerTest, NoAudioNoSession) {
  ASSERT_TRUE(controller_->Initialize(false, false,
                                      media::MediaContentType::Persistent));
  EXPECT_FALSE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());
}

TEST_F(MediaSessionControllerTest, IsRemoteNoSession) {
  ASSERT_TRUE(
      controller_->Initialize(true, true, media::MediaContentType::Persistent));
  EXPECT_FALSE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());
}

TEST_F(MediaSessionControllerTest, TransientNoControllableSession) {
  ASSERT_TRUE(
      controller_->Initialize(true, false, media::MediaContentType::Transient));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());
}

TEST_F(MediaSessionControllerTest, BasicControls) {
  ASSERT_TRUE(controller_->Initialize(true, false,
                                      media::MediaContentType::Persistent));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());

  // Verify suspend notifies the renderer and maintains its session.
  Suspend();
  EXPECT_TRUE(ReceivedMessagePlayPause<MediaPlayerDelegateMsg_Pause>());

  // Likewise verify the resume behavior.
  Resume();
  EXPECT_TRUE(ReceivedMessagePlayPause<MediaPlayerDelegateMsg_Play>());

  // ...as well as the seek behavior.
  const base::TimeDelta kTestSeekForwardTime = base::TimeDelta::FromSeconds(1);
  SeekForward(kTestSeekForwardTime);
  EXPECT_TRUE(ReceivedMessageSeek<MediaPlayerDelegateMsg_SeekForward>(
      kTestSeekForwardTime));
  const base::TimeDelta kTestSeekBackwardTime = base::TimeDelta::FromSeconds(2);
  SeekBackward(kTestSeekBackwardTime);
  EXPECT_TRUE(ReceivedMessageSeek<MediaPlayerDelegateMsg_SeekBackward>(
      kTestSeekBackwardTime));

  // Verify destruction of the controller removes its session.
  controller_.reset();
  EXPECT_FALSE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());
}

TEST_F(MediaSessionControllerTest, VolumeMultiplier) {
  ASSERT_TRUE(controller_->Initialize(true, false,
                                      media::MediaContentType::Persistent));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());

  // Upon creation of the MediaSession the default multiplier will be sent.
  EXPECT_TRUE(ReceivedMessageVolumeMultiplierUpdate<
              MediaPlayerDelegateMsg_UpdateVolumeMultiplier>(1.0));

  // Verify a different volume multiplier is sent.
  const double kTestMultiplier = 0.5;
  SetVolumeMultiplier(kTestMultiplier);
  EXPECT_TRUE(ReceivedMessageVolumeMultiplierUpdate<
              MediaPlayerDelegateMsg_UpdateVolumeMultiplier>(kTestMultiplier));
}

TEST_F(MediaSessionControllerTest, ControllerSidePause) {
  ASSERT_TRUE(controller_->Initialize(true, false,
                                      media::MediaContentType::Persistent));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());

  // Verify pause behavior.
  controller_->OnPlaybackPaused();
  EXPECT_FALSE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());

  // Verify the next Initialize() call restores the session.
  ASSERT_TRUE(controller_->Initialize(true, false,
                                      media::MediaContentType::Persistent));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());
}

TEST_F(MediaSessionControllerTest, Reinitialize) {
  ASSERT_TRUE(controller_->Initialize(false, false,
                                      media::MediaContentType::Persistent));
  EXPECT_FALSE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());

  // Create a transient type session.
  ASSERT_TRUE(
      controller_->Initialize(true, false, media::MediaContentType::Transient));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());
  const int current_player_id = controller_->get_player_id_for_testing();

  // Reinitialize the session as a content type.
  ASSERT_TRUE(controller_->Initialize(true, false,
                                      media::MediaContentType::Persistent));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());
  // Player id should not change when there's an active session.
  EXPECT_EQ(current_player_id, controller_->get_player_id_for_testing());

  // Verify suspend notifies the renderer and maintains its session.
  Suspend();
  EXPECT_TRUE(ReceivedMessagePlayPause<MediaPlayerDelegateMsg_Pause>());

  // Likewise verify the resume behavior.
  Resume();
  EXPECT_TRUE(ReceivedMessagePlayPause<MediaPlayerDelegateMsg_Play>());

  // Attempt to switch to no audio player, which should do nothing.
  // TODO(dalecurtis): Delete this test once we're no longer using WMPA and
  // the BrowserMediaPlayerManagers.  Tracked by http://crbug.com/580626
  ASSERT_TRUE(controller_->Initialize(false, false,
                                      media::MediaContentType::Persistent));
  EXPECT_TRUE(media_session()->IsActive());
  EXPECT_TRUE(media_session()->IsControllable());
  EXPECT_EQ(current_player_id, controller_->get_player_id_for_testing());

  // Switch to a remote player, which should release the session.
  ASSERT_TRUE(
      controller_->Initialize(true, true, media::MediaContentType::Persistent));
  EXPECT_FALSE(media_session()->IsActive());
  EXPECT_FALSE(media_session()->IsControllable());
  EXPECT_EQ(current_player_id, controller_->get_player_id_for_testing());
}

}  // namespace content

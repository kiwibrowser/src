// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "media/base/android/media_player_bridge.h"
#include "media/base/android/media_player_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

class MockMediaPlayerManager : public MediaPlayerManager {
 public:
  MOCK_METHOD0(GetMediaResourceGetter, MediaResourceGetter*());
  MOCK_METHOD0(GetMediaUrlInterceptor, MediaUrlInterceptor*());
  MOCK_METHOD3(OnTimeUpdate,
               void(int player_id,
                    base::TimeDelta current_timestamp,
                    base::TimeTicks current_time_ticks));
  MOCK_METHOD5(OnMediaMetadataChanged,
               void(int player_id,
                    base::TimeDelta duration,
                    int width,
                    int height,
                    bool success));
  MOCK_METHOD1(OnPlaybackComplete, void(int player_id));
  MOCK_METHOD1(OnMediaInterrupted, void(int player_id));
  MOCK_METHOD2(OnBufferingUpdate, void(int player_id, int percentage));
  MOCK_METHOD2(OnSeekComplete,
               void(int player_id, const base::TimeDelta& current_time));
  MOCK_METHOD2(OnError, void(int player_id, int error));
  MOCK_METHOD3(OnVideoSizeChanged, void(int player_id, int width, int height));
  MOCK_METHOD2(OnAudibleStateChanged, void(int player_id, bool is_audible_now));
  MOCK_METHOD1(OnWaitingForDecryptionKey, void(int player_id));
  MOCK_METHOD0(GetFullscreenPlayer, MediaPlayerAndroid*());
  MOCK_METHOD1(GetPlayer, MediaPlayerAndroid*(int player_id));
  MOCK_METHOD3(RequestPlay,
               bool(int player_id, base::TimeDelta duration, bool has_audio));

  void OnMediaResourcesRequested(int player_id) {}
};

}  // anonymous namespace

class MediaPlayerBridgeTest : public testing::Test {
 public:
  MediaPlayerBridgeTest()
      : bridge_(0,
                GURL(),
                GURL(),
                "",
                false,
                &manager_,
                base::Bind(&MockMediaPlayerManager::OnMediaResourcesRequested,
                           base::Unretained(&manager_)),
                GURL(),
                false) {}

  void SetCanSeekForward(bool can_seek_forward) {
    bridge_.can_seek_forward_ = can_seek_forward;
  }

  void SetCanSeekBackward(bool can_seek_backward) {
    bridge_.can_seek_backward_ = can_seek_backward;
  }

  bool SeekInternal(const base::TimeDelta& current_time, base::TimeDelta time) {
    return bridge_.SeekInternal(current_time, time);
  }

 private:
  // A message loop needs to be instantiated in order for the test to run
  // properly.
  base::MessageLoop message_loop_;
  MockMediaPlayerManager manager_;
  MediaPlayerBridge bridge_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerBridgeTest);
};

TEST_F(MediaPlayerBridgeTest, PreventForwardSeekWhenItIsNotPossible) {
  // Simulate the Java MediaPlayerBridge reporting that forward seeks are not
  // possible
  SetCanSeekForward(false);
  SetCanSeekBackward(true);

  // If this assertion fails, seeks will be allowed which will result in a
  // crash because j_media_player_bridge_ cannot be properly instantiated
  // during this test.
  ASSERT_FALSE(
      SeekInternal(base::TimeDelta(), base::TimeDelta::FromSeconds(10)));
}

TEST_F(MediaPlayerBridgeTest, PreventBackwardSeekWhenItIsNotPossible) {
  // Simulate the Java MediaPlayerBridge reporting that backward seeks are not
  // possible
  SetCanSeekForward(true);
  SetCanSeekBackward(false);

  // If this assertion fails, seeks will be allowed which will result in a
  // crash because j_media_player_bridge_ cannot be properly instantiated
  // during this test.
  ASSERT_FALSE(
      SeekInternal(base::TimeDelta::FromSeconds(10), base::TimeDelta()));
}

}  // namespace media

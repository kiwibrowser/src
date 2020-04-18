// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media_router/providers/cast/cast_media_source.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

TEST(CastMediaSourceTest, FromCastURL) {
  MediaSource::Id source_id(
      "cast:ABCDEFAB?capabilities=video_out,audio_out"
      "&broadcastNamespace=namespace"
      "&broadcastMessage=message");
  std::unique_ptr<CastMediaSource> source = CastMediaSource::From(source_id);
  ASSERT_TRUE(source);
  EXPECT_EQ(source_id, source->source_id());
  ASSERT_EQ(1u, source->app_infos().size());
  const CastAppInfo& app_info = source->app_infos()[0];
  EXPECT_EQ("ABCDEFAB", app_info.app_id);
  EXPECT_EQ(cast_channel::CastDeviceCapability::VIDEO_OUT |
                cast_channel::CastDeviceCapability::AUDIO_OUT,
            app_info.required_capabilities);
  const auto& broadcast_request = source->broadcast_request();
  ASSERT_TRUE(broadcast_request);
  EXPECT_EQ("namespace", broadcast_request->broadcast_namespace);
  EXPECT_EQ("message", broadcast_request->message);
}

TEST(CastMediaSourceTest, FromLegacyCastURL) {
  MediaSource::Id source_id(
      "https://google.com/cast#__castAppId__=ABCDEFAB(video_out,audio_out)"
      "/__castBroadcastNamespace__=namespace"
      "/__castBroadcastMessage__=message");
  std::unique_ptr<CastMediaSource> source = CastMediaSource::From(source_id);
  ASSERT_TRUE(source);
  EXPECT_EQ(source_id, source->source_id());
  ASSERT_EQ(1u, source->app_infos().size());
  const CastAppInfo& app_info = source->app_infos()[0];
  EXPECT_EQ("ABCDEFAB", app_info.app_id);
  EXPECT_EQ(cast_channel::CastDeviceCapability::VIDEO_OUT |
                cast_channel::CastDeviceCapability::AUDIO_OUT,
            app_info.required_capabilities);
  const auto& broadcast_request = source->broadcast_request();
  ASSERT_TRUE(broadcast_request);
  EXPECT_EQ("namespace", broadcast_request->broadcast_namespace);
  EXPECT_EQ("message", broadcast_request->message);
}

TEST(CastMediaSourceTest, FromPresentationURL) {
  MediaSource::Id source_id("https://google.com");
  std::unique_ptr<CastMediaSource> source = CastMediaSource::From(source_id);
  ASSERT_TRUE(source);
  EXPECT_EQ(source_id, source->source_id());
  ASSERT_EQ(2u, source->app_infos().size());
  EXPECT_EQ("0F5096E8", source->app_infos()[0].app_id);
  EXPECT_EQ("85CDB22F", source->app_infos()[1].app_id);
}

TEST(CastMediaSourceTest, FromMirroringURN) {
  MediaSource::Id source_id("urn:x-org.chromium.media:source:tab:5");
  std::unique_ptr<CastMediaSource> source = CastMediaSource::From(source_id);
  ASSERT_TRUE(source);
  EXPECT_EQ(source_id, source->source_id());
  ASSERT_EQ(2u, source->app_infos().size());
  EXPECT_EQ("0F5096E8", source->app_infos()[0].app_id);
  EXPECT_EQ("85CDB22F", source->app_infos()[1].app_id);
}

TEST(CastMediaSourceTest, FromInvalidSource) {
  EXPECT_FALSE(CastMediaSource::From("invalid:source"));
  EXPECT_FALSE(CastMediaSource::From("file:///foo.mp4"));
  EXPECT_FALSE(CastMediaSource::From(""));
  EXPECT_FALSE(CastMediaSource::From("cast:"));

  // Missing app ID.
  EXPECT_FALSE(CastMediaSource::From("cast:?param=foo"));
  EXPECT_FALSE(CastMediaSource::From(
      "https://google.com/cast#__castAppId__=/param=foo"));
}

}  // namespace media_router

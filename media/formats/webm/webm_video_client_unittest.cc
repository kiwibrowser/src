// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/webm/webm_video_client.h"

#include "media/base/media_util.h"
#include "media/base/mock_media_log.h"
#include "media/base/video_decoder_config.h"
#include "media/formats/webm/webm_constants.h"

namespace media {

namespace {
const gfx::Size kCodedSize(321, 243);
}

static const struct CodecTestParams {
  VideoCodecProfile profile;
  const std::vector<uint8_t> codec_private;
} kCodecTestParams[] = {
    {VP9PROFILE_PROFILE0, {}},
    {VP9PROFILE_PROFILE2,
     // Valid VP9 Profile 2 example, extracted out of a sample file at
     // https://www.webmproject.org/vp9/levels/#test-bitstreams
     {0x01, 0x01, 0x02, 0x02, 0x01, 0x0a, 0x3, 0x1, 0xa, 0x4, 0x1, 0x1}},
    // Invalid VP9 CodecPrivate: too short.
    {VP9PROFILE_PROFILE0, {0x01, 0x01}},
    // Invalid VP9 CodecPrivate: wrong field id.
    {VP9PROFILE_PROFILE0, {0x77, 0x01, 0x02}},
    // Invalid VP9 CodecPrivate: wrong field length.
    {VP9PROFILE_PROFILE0, {0x01, 0x75, 0x02}},
    // Invalid VP9 CodecPrivate: wrong Profile (can't be > 3).
    {VP9PROFILE_PROFILE0, {0x01, 0x01, 0x34}}};

class WebMVideoClientTest : public testing::TestWithParam<CodecTestParams> {
 public:
  WebMVideoClientTest() : webm_video_client_(&media_log_) {
    // Simulate configuring width and height in the |webm_video_client_|.
    webm_video_client_.OnUInt(kWebMIdPixelWidth, kCodedSize.width());
    webm_video_client_.OnUInt(kWebMIdPixelHeight, kCodedSize.height());
  }

  testing::StrictMock<MockMediaLog> media_log_;
  WebMVideoClient webm_video_client_;

  DISALLOW_COPY_AND_ASSIGN(WebMVideoClientTest);
};

TEST_P(WebMVideoClientTest, InitializeConfigVP9Profiles) {
  const std::string kCodecId = "V_VP9";
  const VideoCodecProfile profile = GetParam().profile;
  const std::vector<uint8_t> codec_private = GetParam().codec_private;

  VideoDecoderConfig config;
  EXPECT_TRUE(webm_video_client_.InitializeConfig(kCodecId, codec_private,
                                                  EncryptionScheme(), &config));

  VideoDecoderConfig expected_config(kCodecVP9, profile, PIXEL_FORMAT_I420,
                                     COLOR_SPACE_HD_REC709, VIDEO_ROTATION_0,
                                     kCodedSize, gfx::Rect(kCodedSize),
                                     kCodedSize, codec_private, Unencrypted());

  EXPECT_TRUE(config.Matches(expected_config))
      << "Config (" << config.AsHumanReadableString()
      << ") does not match expected ("
      << expected_config.AsHumanReadableString() << ")";
}

INSTANTIATE_TEST_CASE_P(/* No prefix. */,
                        WebMVideoClientTest,
                        ::testing::ValuesIn(kCodecTestParams));

}  // namespace media

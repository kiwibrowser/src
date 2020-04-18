/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <memory>
#include <string>

#include "modules/rtp_rtcp/include/rtp_rtcp.h"
#include "modules/rtp_rtcp/mocks/mock_rtp_rtcp.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "test/field_trial.h"
#include "test/gmock.h"
#include "test/gtest.h"
#include "video/payload_router.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Unused;

namespace webrtc {
namespace {
const int8_t kPayloadType = 96;
const uint32_t kSsrc1 = 12345;
const uint32_t kSsrc2 = 23456;
const uint32_t kSsrc3 = 34567;
const int16_t kPictureId = 123;
const int16_t kTl0PicIdx = 20;
const uint8_t kTemporalIdx = 1;
const int16_t kInitialPictureId1 = 222;
const int16_t kInitialPictureId2 = 44;
const int16_t kInitialTl0PicIdx1 = 99;
const int16_t kInitialTl0PicIdx2 = 199;
}  // namespace

TEST(PayloadRouterTest, SendOnOneModule) {
  NiceMock<MockRtpRtcp> rtp;
  std::vector<RtpRtcp*> modules(1, &rtp);

  uint8_t payload = 'a';
  EncodedImage encoded_image;
  encoded_image._timeStamp = 1;
  encoded_image.capture_time_ms_ = 2;
  encoded_image._frameType = kVideoFrameKey;
  encoded_image._buffer = &payload;
  encoded_image._length = 1;

  PayloadRouter payload_router(modules, {kSsrc1}, kPayloadType, {});

  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _, _))
      .Times(0);
  EXPECT_NE(
      EncodedImageCallback::Result::OK,
      payload_router.OnEncodedImage(encoded_image, nullptr, nullptr).error);

  payload_router.SetActive(true);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _, _))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));
  EXPECT_EQ(
      EncodedImageCallback::Result::OK,
      payload_router.OnEncodedImage(encoded_image, nullptr, nullptr).error);

  payload_router.SetActive(false);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _, _))
      .Times(0);
  EXPECT_NE(
      EncodedImageCallback::Result::OK,
      payload_router.OnEncodedImage(encoded_image, nullptr, nullptr).error);

  payload_router.SetActive(true);
  EXPECT_CALL(rtp, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                    encoded_image._timeStamp,
                                    encoded_image.capture_time_ms_, &payload,
                                    encoded_image._length, nullptr, _, _))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));
  EXPECT_EQ(
      EncodedImageCallback::Result::OK,
      payload_router.OnEncodedImage(encoded_image, nullptr, nullptr).error);
}

TEST(PayloadRouterTest, SendSimulcastSetActive) {
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  std::vector<RtpRtcp*> modules = {&rtp_1, &rtp_2};

  uint8_t payload = 'a';
  EncodedImage encoded_image;
  encoded_image._timeStamp = 1;
  encoded_image.capture_time_ms_ = 2;
  encoded_image._frameType = kVideoFrameKey;
  encoded_image._buffer = &payload;
  encoded_image._length = 1;

  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2}, kPayloadType, {});

  CodecSpecificInfo codec_info_1;
  memset(&codec_info_1, 0, sizeof(CodecSpecificInfo));
  codec_info_1.codecType = kVideoCodecVP8;
  codec_info_1.codecSpecific.VP8.simulcastIdx = 0;

  payload_router.SetActive(true);
  EXPECT_CALL(rtp_1, Sending()).WillOnce(Return(true));
  EXPECT_CALL(rtp_1, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                      encoded_image._timeStamp,
                                      encoded_image.capture_time_ms_, &payload,
                                      encoded_image._length, nullptr, _, _))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(rtp_2, SendOutgoingData(_, _, _, _, _, _, _, _, _)).Times(0);
  EXPECT_EQ(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_1, nullptr)
                .error);

  CodecSpecificInfo codec_info_2;
  memset(&codec_info_2, 0, sizeof(CodecSpecificInfo));
  codec_info_2.codecType = kVideoCodecVP8;
  codec_info_2.codecSpecific.VP8.simulcastIdx = 1;

  EXPECT_CALL(rtp_2, Sending()).WillOnce(Return(true));
  EXPECT_CALL(rtp_2, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                      encoded_image._timeStamp,
                                      encoded_image.capture_time_ms_, &payload,
                                      encoded_image._length, nullptr, _, _))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_CALL(rtp_1, SendOutgoingData(_, _, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_EQ(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_2, nullptr)
                .error);

  // Inactive.
  payload_router.SetActive(false);
  EXPECT_CALL(rtp_1, SendOutgoingData(_, _, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_CALL(rtp_2, SendOutgoingData(_, _, _, _, _, _, _, _, _))
      .Times(0);
  EXPECT_NE(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_1, nullptr)
                .error);
  EXPECT_NE(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_2, nullptr)
                .error);
}

// Tests how setting individual rtp modules to active affects the overall
// behavior of the payload router. First sets one module to active and checks
// that outgoing data can be sent on this module, and checks that no data can be
// sent if both modules are inactive.
TEST(PayloadRouterTest, SendSimulcastSetActiveModules) {
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  std::vector<RtpRtcp*> modules = {&rtp_1, &rtp_2};

  uint8_t payload = 'a';
  EncodedImage encoded_image;
  encoded_image._timeStamp = 1;
  encoded_image.capture_time_ms_ = 2;
  encoded_image._frameType = kVideoFrameKey;
  encoded_image._buffer = &payload;
  encoded_image._length = 1;
  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2}, kPayloadType, {});
  CodecSpecificInfo codec_info_1;
  memset(&codec_info_1, 0, sizeof(CodecSpecificInfo));
  codec_info_1.codecType = kVideoCodecVP8;
  codec_info_1.codecSpecific.VP8.simulcastIdx = 0;
  CodecSpecificInfo codec_info_2;
  memset(&codec_info_2, 0, sizeof(CodecSpecificInfo));
  codec_info_2.codecType = kVideoCodecVP8;
  codec_info_2.codecSpecific.VP8.simulcastIdx = 1;

  // Only setting one stream to active will still set the payload router to
  // active and allow sending data on the active stream.
  std::vector<bool> active_modules({true, false});
  payload_router.SetActiveModules(active_modules);

  EXPECT_CALL(rtp_1, Sending()).WillOnce(Return(true));
  EXPECT_CALL(rtp_1, SendOutgoingData(encoded_image._frameType, kPayloadType,
                                      encoded_image._timeStamp,
                                      encoded_image.capture_time_ms_, &payload,
                                      encoded_image._length, nullptr, _, _))
      .Times(1)
      .WillOnce(Return(true));
  EXPECT_EQ(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_1, nullptr)
                .error);

  // Setting both streams to inactive will turn the payload router to inactive.
  active_modules = {false, false};
  payload_router.SetActiveModules(active_modules);
  // An incoming encoded image will not ask the module to send outgoing data
  // because the payload router is inactive.
  EXPECT_CALL(rtp_1, SendOutgoingData(_, _, _, _, _, _, _, _, _)).Times(0);
  EXPECT_CALL(rtp_1, Sending()).Times(0);
  EXPECT_CALL(rtp_2, SendOutgoingData(_, _, _, _, _, _, _, _, _)).Times(0);
  EXPECT_CALL(rtp_2, Sending()).Times(0);
  EXPECT_NE(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_1, nullptr)
                .error);
  EXPECT_NE(EncodedImageCallback::Result::OK,
            payload_router.OnEncodedImage(encoded_image, &codec_info_2, nullptr)
                .error);
}

TEST(PayloadRouterTest, SimulcastTargetBitrate) {
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  std::vector<RtpRtcp*> modules = {&rtp_1, &rtp_2};

  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2}, kPayloadType, {});
  payload_router.SetActive(true);

  VideoBitrateAllocation bitrate;
  bitrate.SetBitrate(0, 0, 10000);
  bitrate.SetBitrate(0, 1, 20000);
  bitrate.SetBitrate(1, 0, 40000);
  bitrate.SetBitrate(1, 1, 80000);

  VideoBitrateAllocation layer0_bitrate;
  layer0_bitrate.SetBitrate(0, 0, 10000);
  layer0_bitrate.SetBitrate(0, 1, 20000);

  VideoBitrateAllocation layer1_bitrate;
  layer1_bitrate.SetBitrate(0, 0, 40000);
  layer1_bitrate.SetBitrate(0, 1, 80000);

  EXPECT_CALL(rtp_1, SetVideoBitrateAllocation(layer0_bitrate)).Times(1);
  EXPECT_CALL(rtp_2, SetVideoBitrateAllocation(layer1_bitrate)).Times(1);

  payload_router.OnBitrateAllocationUpdated(bitrate);
}

// If the middle of three streams is inactive the first and last streams should
// be asked to send the TargetBitrate message.
TEST(PayloadRouterTest, SimulcastTargetBitrateWithInactiveStream) {
  // Set up three active rtp modules.
  NiceMock<MockRtpRtcp> rtp_1;
  NiceMock<MockRtpRtcp> rtp_2;
  NiceMock<MockRtpRtcp> rtp_3;
  std::vector<RtpRtcp*> modules = {&rtp_1, &rtp_2, &rtp_3};
  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2, kSsrc3}, kPayloadType,
                               {});
  payload_router.SetActive(true);

  // Create bitrate allocation with bitrate only for the first and third stream.
  VideoBitrateAllocation bitrate;
  bitrate.SetBitrate(0, 0, 10000);
  bitrate.SetBitrate(0, 1, 20000);
  bitrate.SetBitrate(2, 0, 40000);
  bitrate.SetBitrate(2, 1, 80000);

  VideoBitrateAllocation layer0_bitrate;
  layer0_bitrate.SetBitrate(0, 0, 10000);
  layer0_bitrate.SetBitrate(0, 1, 20000);

  VideoBitrateAllocation layer2_bitrate;
  layer2_bitrate.SetBitrate(0, 0, 40000);
  layer2_bitrate.SetBitrate(0, 1, 80000);

  // Expect the first and third rtp module to be asked to send a TargetBitrate
  // message. (No target bitrate with 0bps sent from the second one.)
  EXPECT_CALL(rtp_1, SetVideoBitrateAllocation(layer0_bitrate)).Times(1);
  EXPECT_CALL(rtp_2, SetVideoBitrateAllocation(_)).Times(0);
  EXPECT_CALL(rtp_3, SetVideoBitrateAllocation(layer2_bitrate)).Times(1);

  payload_router.OnBitrateAllocationUpdated(bitrate);
}

TEST(PayloadRouterTest, SvcTargetBitrate) {
  NiceMock<MockRtpRtcp> rtp_1;
  std::vector<RtpRtcp*> modules = {&rtp_1};
  PayloadRouter payload_router(modules, {kSsrc1}, kPayloadType, {});
  payload_router.SetActive(true);

  VideoBitrateAllocation bitrate;
  bitrate.SetBitrate(0, 0, 10000);
  bitrate.SetBitrate(0, 1, 20000);
  bitrate.SetBitrate(1, 0, 40000);
  bitrate.SetBitrate(1, 1, 80000);

  EXPECT_CALL(rtp_1, SetVideoBitrateAllocation(bitrate)).Times(1);

  payload_router.OnBitrateAllocationUpdated(bitrate);
}

TEST(PayloadRouterTest, InfoMappedToRtpVideoHeader_Vp8) {
  NiceMock<MockRtpRtcp> rtp1;
  NiceMock<MockRtpRtcp> rtp2;
  std::vector<RtpRtcp*> modules = {&rtp1, &rtp2};
  RtpPayloadState state2;
  state2.picture_id = kPictureId;
  state2.tl0_pic_idx = kTl0PicIdx;
  std::map<uint32_t, RtpPayloadState> states = {{kSsrc2, state2}};

  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2}, kPayloadType, states);
  payload_router.SetActive(true);

  EncodedImage encoded_image;
  encoded_image.rotation_ = kVideoRotation_90;
  encoded_image.content_type_ = VideoContentType::SCREENSHARE;

  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecVP8;
  codec_info.codecSpecific.VP8.simulcastIdx = 1;
  codec_info.codecSpecific.VP8.temporalIdx = kTemporalIdx;
  codec_info.codecSpecific.VP8.keyIdx = kNoKeyIdx;
  codec_info.codecSpecific.VP8.layerSync = true;
  codec_info.codecSpecific.VP8.nonReference = true;

  EXPECT_CALL(rtp2, Sending()).WillOnce(Return(true));
  EXPECT_CALL(rtp2, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kVideoRotation_90, header->rotation);
        EXPECT_EQ(VideoContentType::SCREENSHARE, header->content_type);
        EXPECT_EQ(1, header->simulcastIdx);
        EXPECT_EQ(kRtpVideoVp8, header->codec);
        EXPECT_EQ(kPictureId + 1, header->codecHeader.VP8.pictureId);
        EXPECT_EQ(kTemporalIdx, header->codecHeader.VP8.temporalIdx);
        EXPECT_EQ(kTl0PicIdx, header->codecHeader.VP8.tl0PicIdx);
        EXPECT_EQ(kNoKeyIdx, header->codecHeader.VP8.keyIdx);
        EXPECT_TRUE(header->codecHeader.VP8.layerSync);
        EXPECT_TRUE(header->codecHeader.VP8.nonReference);
        return true;
      }));

  EXPECT_EQ(
      EncodedImageCallback::Result::OK,
      payload_router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);
}

TEST(PayloadRouterTest, InfoMappedToRtpVideoHeader_Vp9) {
  RtpPayloadState state;
  state.picture_id = kPictureId;
  state.tl0_pic_idx = kTl0PicIdx;
  std::map<uint32_t, RtpPayloadState> states = {{kSsrc1, state}};

  NiceMock<MockRtpRtcp> rtp;
  std::vector<RtpRtcp*> modules = {&rtp};
  PayloadRouter router(modules, {kSsrc1}, kPayloadType, states);
  router.SetActive(true);

  EncodedImage encoded_image;
  encoded_image.rotation_ = kVideoRotation_90;
  encoded_image.content_type_ = VideoContentType::SCREENSHARE;

  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecVP9;
  codec_info.codecSpecific.VP9.num_spatial_layers = 3;
  codec_info.codecSpecific.VP9.first_frame_in_picture = true;
  codec_info.codecSpecific.VP9.spatial_idx = 0;
  codec_info.codecSpecific.VP9.temporal_idx = 2;
  codec_info.codecSpecific.VP9.end_of_picture = false;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(
          Invoke([&codec_info](Unused, Unused, Unused, Unused, Unused, Unused,
                               Unused, const RTPVideoHeader* header, Unused) {
            EXPECT_EQ(kVideoRotation_90, header->rotation);
            EXPECT_EQ(VideoContentType::SCREENSHARE, header->content_type);
            EXPECT_EQ(kRtpVideoVp9, header->codec);
            EXPECT_EQ(kPictureId + 1, header->codecHeader.VP9.picture_id);
            EXPECT_EQ(kTl0PicIdx, header->codecHeader.VP9.tl0_pic_idx);
            EXPECT_EQ(header->codecHeader.VP9.temporal_idx,
                      codec_info.codecSpecific.VP9.temporal_idx);
            EXPECT_EQ(header->codecHeader.VP9.spatial_idx,
                      codec_info.codecSpecific.VP9.spatial_idx);
            EXPECT_EQ(header->codecHeader.VP9.num_spatial_layers,
                      codec_info.codecSpecific.VP9.num_spatial_layers);
            EXPECT_EQ(header->codecHeader.VP9.end_of_picture,
                      codec_info.codecSpecific.VP9.end_of_picture);
            return true;
          }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // Next spatial layer.
  codec_info.codecSpecific.VP9.first_frame_in_picture = false;
  codec_info.codecSpecific.VP9.spatial_idx += 1;
  codec_info.codecSpecific.VP9.end_of_picture = true;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(
          Invoke([&codec_info](Unused, Unused, Unused, Unused, Unused, Unused,
                               Unused, const RTPVideoHeader* header, Unused) {
            EXPECT_EQ(kVideoRotation_90, header->rotation);
            EXPECT_EQ(VideoContentType::SCREENSHARE, header->content_type);
            EXPECT_EQ(kRtpVideoVp9, header->codec);
            EXPECT_EQ(kPictureId + 1, header->codecHeader.VP9.picture_id);
            EXPECT_EQ(kTl0PicIdx, header->codecHeader.VP9.tl0_pic_idx);
            EXPECT_EQ(header->codecHeader.VP9.temporal_idx,
                      codec_info.codecSpecific.VP9.temporal_idx);
            EXPECT_EQ(header->codecHeader.VP9.spatial_idx,
                      codec_info.codecSpecific.VP9.spatial_idx);
            EXPECT_EQ(header->codecHeader.VP9.num_spatial_layers,
                      codec_info.codecSpecific.VP9.num_spatial_layers);
            EXPECT_EQ(header->codecHeader.VP9.end_of_picture,
                      codec_info.codecSpecific.VP9.end_of_picture);
            return true;
          }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);
}

TEST(PayloadRouterTest, InfoMappedToRtpVideoHeader_H264) {
  NiceMock<MockRtpRtcp> rtp1;
  std::vector<RtpRtcp*> modules = {&rtp1};
  PayloadRouter payload_router(modules, {kSsrc1}, kPayloadType, {});
  payload_router.SetActive(true);

  EncodedImage encoded_image;
  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecH264;
  codec_info.codecSpecific.H264.packetization_mode =
      H264PacketizationMode::SingleNalUnit;

  EXPECT_CALL(rtp1, Sending()).WillOnce(Return(true));
  EXPECT_CALL(rtp1, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(0, header->simulcastIdx);
        EXPECT_EQ(kRtpVideoH264, header->codec);
        EXPECT_EQ(H264PacketizationMode::SingleNalUnit,
                  header->codecHeader.H264.packetization_mode);
        return true;
      }));

  EXPECT_EQ(
      EncodedImageCallback::Result::OK,
      payload_router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);
}

TEST(PayloadRouterTest, CreateWithNoPreviousStates) {
  NiceMock<MockRtpRtcp> rtp1;
  NiceMock<MockRtpRtcp> rtp2;
  std::vector<RtpRtcp*> modules = {&rtp1, &rtp2};
  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2}, kPayloadType, {});
  payload_router.SetActive(true);

  std::map<uint32_t, RtpPayloadState> initial_states =
      payload_router.GetRtpPayloadStates();
  EXPECT_EQ(2u, initial_states.size());
  EXPECT_NE(initial_states.find(kSsrc1), initial_states.end());
  EXPECT_NE(initial_states.find(kSsrc2), initial_states.end());
}

TEST(PayloadRouterTest, CreateWithPreviousStates) {
  RtpPayloadState state1;
  state1.picture_id = kInitialPictureId1;
  state1.tl0_pic_idx = kInitialTl0PicIdx1;
  RtpPayloadState state2;
  state2.picture_id = kInitialPictureId2;
  state2.tl0_pic_idx = kInitialTl0PicIdx2;
  std::map<uint32_t, RtpPayloadState> states = {{kSsrc1, state1},
                                                {kSsrc2, state2}};

  NiceMock<MockRtpRtcp> rtp1;
  NiceMock<MockRtpRtcp> rtp2;
  std::vector<RtpRtcp*> modules = {&rtp1, &rtp2};
  PayloadRouter payload_router(modules, {kSsrc1, kSsrc2}, kPayloadType, states);
  payload_router.SetActive(true);

  std::map<uint32_t, RtpPayloadState> initial_states =
      payload_router.GetRtpPayloadStates();
  EXPECT_EQ(2u, initial_states.size());
  EXPECT_EQ(kInitialPictureId1, initial_states[kSsrc1].picture_id);
  EXPECT_EQ(kInitialTl0PicIdx1, initial_states[kSsrc1].tl0_pic_idx);
  EXPECT_EQ(kInitialPictureId2, initial_states[kSsrc2].picture_id);
  EXPECT_EQ(kInitialTl0PicIdx2, initial_states[kSsrc2].tl0_pic_idx);
}

TEST(PayloadRouterTest, PictureIdIsSetForVp8) {
  RtpPayloadState state1;
  state1.picture_id = kInitialPictureId1;
  state1.tl0_pic_idx = kInitialTl0PicIdx1;
  RtpPayloadState state2;
  state2.picture_id = kInitialPictureId2;
  state2.tl0_pic_idx = kInitialTl0PicIdx2;
  std::map<uint32_t, RtpPayloadState> states = {{kSsrc1, state1},
                                                {kSsrc2, state2}};

  NiceMock<MockRtpRtcp> rtp1;
  NiceMock<MockRtpRtcp> rtp2;
  std::vector<RtpRtcp*> modules = {&rtp1, &rtp2};
  PayloadRouter router(modules, {kSsrc1, kSsrc2}, kPayloadType, states);
  router.SetActive(true);

  EncodedImage encoded_image;
  // Modules are sending for this test.
  // OnEncodedImage, simulcastIdx: 0.
  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecVP8;
  codec_info.codecSpecific.VP8.simulcastIdx = 0;

  EXPECT_CALL(rtp1, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp8, header->codec);
        EXPECT_EQ(kInitialPictureId1 + 1, header->codecHeader.VP8.pictureId);
        return true;
      }));
  EXPECT_CALL(rtp1, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // OnEncodedImage, simulcastIdx: 1.
  codec_info.codecSpecific.VP8.simulcastIdx = 1;

  EXPECT_CALL(rtp2, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp8, header->codec);
        EXPECT_EQ(kInitialPictureId2 + 1, header->codecHeader.VP8.pictureId);
        return true;
      }));
  EXPECT_CALL(rtp2, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // State should hold latest used picture id and tl0_pic_idx.
  states = router.GetRtpPayloadStates();
  EXPECT_EQ(2u, states.size());
  EXPECT_EQ(kInitialPictureId1 + 1, states[kSsrc1].picture_id);
  EXPECT_EQ(kInitialTl0PicIdx1 + 1, states[kSsrc1].tl0_pic_idx);
  EXPECT_EQ(kInitialPictureId2 + 1, states[kSsrc2].picture_id);
  EXPECT_EQ(kInitialTl0PicIdx2 + 1, states[kSsrc2].tl0_pic_idx);
}

TEST(PayloadRouterTest, PictureIdWraps) {
  RtpPayloadState state1;
  state1.picture_id = kMaxTwoBytePictureId;
  state1.tl0_pic_idx = kInitialTl0PicIdx1;

  NiceMock<MockRtpRtcp> rtp;
  std::vector<RtpRtcp*> modules = {&rtp};
  PayloadRouter router(modules, {kSsrc1}, kPayloadType, {{kSsrc1, state1}});
  router.SetActive(true);

  EncodedImage encoded_image;
  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecVP8;
  codec_info.codecSpecific.VP8.temporalIdx = kNoTemporalIdx;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp8, header->codec);
        EXPECT_EQ(0, header->codecHeader.VP8.pictureId);
        return true;
      }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // State should hold latest used picture id and tl0_pic_idx.
  std::map<uint32_t, RtpPayloadState> states = router.GetRtpPayloadStates();
  EXPECT_EQ(1u, states.size());
  EXPECT_EQ(0, states[kSsrc1].picture_id);  // Wrapped.
  EXPECT_EQ(kInitialTl0PicIdx1, states[kSsrc1].tl0_pic_idx);
}

TEST(PayloadRouterTest, Tl0PicIdxUpdatedForVp8) {
  RtpPayloadState state;
  state.picture_id = kInitialPictureId1;
  state.tl0_pic_idx = kInitialTl0PicIdx1;
  std::map<uint32_t, RtpPayloadState> states = {{kSsrc1, state}};

  NiceMock<MockRtpRtcp> rtp;
  std::vector<RtpRtcp*> modules = {&rtp};
  PayloadRouter router(modules, {kSsrc1}, kPayloadType, states);
  router.SetActive(true);

  EncodedImage encoded_image;
  // Modules are sending for this test.
  // OnEncodedImage, temporalIdx: 1.
  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecVP8;
  codec_info.codecSpecific.VP8.temporalIdx = 1;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp8, header->codec);
        EXPECT_EQ(kInitialPictureId1 + 1, header->codecHeader.VP8.pictureId);
        EXPECT_EQ(kInitialTl0PicIdx1, header->codecHeader.VP8.tl0PicIdx);
        return true;
      }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // OnEncodedImage, temporalIdx: 0.
  codec_info.codecSpecific.VP8.temporalIdx = 0;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp8, header->codec);
        EXPECT_EQ(kInitialPictureId1 + 2, header->codecHeader.VP8.pictureId);
        EXPECT_EQ(kInitialTl0PicIdx1 + 1, header->codecHeader.VP8.tl0PicIdx);
        return true;
      }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // State should hold latest used picture id and tl0_pic_idx.
  states = router.GetRtpPayloadStates();
  EXPECT_EQ(1u, states.size());
  EXPECT_EQ(kInitialPictureId1 + 2, states[kSsrc1].picture_id);
  EXPECT_EQ(kInitialTl0PicIdx1 + 1, states[kSsrc1].tl0_pic_idx);
}

TEST(PayloadRouterTest, Tl0PicIdxUpdatedForVp9) {
  RtpPayloadState state;
  state.picture_id = kInitialPictureId1;
  state.tl0_pic_idx = kInitialTl0PicIdx1;
  std::map<uint32_t, RtpPayloadState> states = {{kSsrc1, state}};

  NiceMock<MockRtpRtcp> rtp;
  std::vector<RtpRtcp*> modules = {&rtp};
  PayloadRouter router(modules, {kSsrc1}, kPayloadType, states);
  router.SetActive(true);

  EncodedImage encoded_image;
  // Modules are sending for this test.
  // OnEncodedImage, temporalIdx: 1.
  CodecSpecificInfo codec_info;
  memset(&codec_info, 0, sizeof(CodecSpecificInfo));
  codec_info.codecType = kVideoCodecVP9;
  codec_info.codecSpecific.VP9.temporal_idx = 1;
  codec_info.codecSpecific.VP9.first_frame_in_picture = true;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp9, header->codec);
        EXPECT_EQ(kInitialPictureId1 + 1, header->codecHeader.VP9.picture_id);
        EXPECT_EQ(kInitialTl0PicIdx1, header->codecHeader.VP9.tl0_pic_idx);
        return true;
      }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // OnEncodedImage, temporalIdx: 0.
  codec_info.codecSpecific.VP9.temporal_idx = 0;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp9, header->codec);
        EXPECT_EQ(kInitialPictureId1 + 2, header->codecHeader.VP9.picture_id);
        EXPECT_EQ(kInitialTl0PicIdx1 + 1, header->codecHeader.VP9.tl0_pic_idx);
        return true;
      }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // OnEncodedImage, first_frame_in_picture = false
  codec_info.codecSpecific.VP9.first_frame_in_picture = false;

  EXPECT_CALL(rtp, SendOutgoingData(_, _, _, _, _, _, nullptr, _, _))
      .WillOnce(Invoke([](Unused, Unused, Unused, Unused, Unused, Unused,
                          Unused, const RTPVideoHeader* header, Unused) {
        EXPECT_EQ(kRtpVideoVp9, header->codec);
        EXPECT_EQ(kInitialPictureId1 + 2, header->codecHeader.VP9.picture_id);
        EXPECT_EQ(kInitialTl0PicIdx1 + 1, header->codecHeader.VP9.tl0_pic_idx);
        return true;
      }));
  EXPECT_CALL(rtp, Sending()).WillOnce(Return(true));

  EXPECT_EQ(EncodedImageCallback::Result::OK,
            router.OnEncodedImage(encoded_image, &codec_info, nullptr).error);

  // State should hold latest used picture id and tl0_pic_idx.
  states = router.GetRtpPayloadStates();
  EXPECT_EQ(1u, states.size());
  EXPECT_EQ(kInitialPictureId1 + 2, states[kSsrc1].picture_id);
  EXPECT_EQ(kInitialTl0PicIdx1 + 1, states[kSsrc1].tl0_pic_idx);
}

}  // namespace webrtc

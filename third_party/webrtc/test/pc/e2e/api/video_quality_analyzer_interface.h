/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_API_VIDEO_QUALITY_ANALYZER_INTERFACE_H_
#define TEST_PC_E2E_API_VIDEO_QUALITY_ANALYZER_INTERFACE_H_

#include <memory>
#include <string>

#include "absl/types/optional.h"
#include "api/video/encoded_image.h"
#include "api/video/video_frame.h"
#include "api/video_codecs/video_encoder.h"

namespace webrtc {

// Base interface for video quality analyzer for peer connection level end-2-end
// tests. Interface has only one abstract method, which have to return frame id.
// Other methods have empty implementation by default, so user can override only
// required parts.
//
// VideoQualityAnalyzerInterface will be injected into WebRTC pipeline on both
// sides of the call. Here is video data flow in WebRTC pipeline
//
// Alice:
//  ___________       ________       _________
// |           |     |        |     |         |
// |   Frame   |-(A)→| WebRTC |-(B)→| Video   |-(C)┐
// | Generator |     | Stack  |     | Decoder |    |
//  ¯¯¯¯¯¯¯¯¯¯¯       ¯¯¯¯¯¯¯¯       ¯¯¯¯¯¯¯¯¯     |
//                                               __↓________
//                                              | Transport |
//                                              |     &     |
//                                              |  Network  |
//                                               ¯¯|¯¯¯¯¯¯¯¯
// Bob:                                            |
//  _______       ________       _________         |
// |       |     |        |     |         |        |
// | Video |←(F)-| WebRTC |←(E)-| Video   |←(D)----┘
// | Sink  |     | Stack  |     | Decoder |
//  ¯¯¯¯¯¯¯       ¯¯¯¯¯¯¯¯       ¯¯¯¯¯¯¯¯¯
// The analyzer will be injected in all points from A to F.
class VideoQualityAnalyzerInterface {
 public:
  virtual ~VideoQualityAnalyzerInterface() = default;

  // Will be called by framework before test. |threads_count| is number of
  // threads that analyzer can use for heavy calculations. Analyzer can perform
  // simple calculations on the calling thread in each method, but should
  // remember, that it is the same thread, that is used in video pipeline.
  virtual void Start(int max_threads_count) {}

  // Will be called when frame was generated from the input stream.
  // Returns frame id, that will be set by framework to the frame.
  virtual uint16_t OnFrameCaptured(const std::string& stream_label,
                                   const VideoFrame& frame) = 0;
  // Will be called before calling the encoder.
  virtual void OnFramePreEncode(const VideoFrame& frame) {}
  // Will be called for each EncodedImage received from encoder. Single
  // VideoFrame can produce multiple EncodedImages. Each encoded image will
  // have id from VideoFrame.
  virtual void OnFrameEncoded(uint16_t frame_id,
                              const EncodedImage& encoded_image) {}
  // Will be called for each frame dropped by encoder.
  virtual void OnFrameDropped(EncodedImageCallback::DropReason reason) {}
  // Will be called before calling the decoder.
  virtual void OnFrameReceived(uint16_t frame_id,
                               const EncodedImage& encoded_image) {}
  // Will be called after decoding the frame. |decode_time_ms| is a decode
  // time provided by decoder itself. If decoder doesn’t produce such
  // information can be omitted.
  virtual void OnFrameDecoded(const VideoFrame& frame,
                              absl::optional<int32_t> decode_time_ms,
                              absl::optional<uint8_t> qp) {}
  // Will be called when frame will be obtained from PeerConnection stack.
  virtual void OnFrameRendered(const VideoFrame& frame) {}
  // Will be called if encoder return not WEBRTC_VIDEO_CODEC_OK.
  // All available codes are listed in
  // modules/video_coding/include/video_error_codes.h
  virtual void OnEncoderError(const VideoFrame& frame, int32_t error_code) {}
  // Will be called if decoder return not WEBRTC_VIDEO_CODEC_OK.
  // All available codes are listed in
  // modules/video_coding/include/video_error_codes.h
  virtual void OnDecoderError(uint16_t frame_id, int32_t error_code) {}

  // Tells analyzer that analysis complete and it should calculate final
  // statistics.
  virtual void Stop() {}
};

}  // namespace webrtc

#endif  // TEST_PC_E2E_API_VIDEO_QUALITY_ANALYZER_INTERFACE_H_

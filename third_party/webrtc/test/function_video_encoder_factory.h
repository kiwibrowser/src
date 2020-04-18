/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_FUNCTION_VIDEO_ENCODER_FACTORY_H_
#define TEST_FUNCTION_VIDEO_ENCODER_FACTORY_H_

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "api/video_codecs/video_encoder_factory.h"

namespace webrtc {
namespace test {

// An encoder factory producing encoders by calling a supplied create
// function.
class FunctionVideoEncoderFactory final : public VideoEncoderFactory {
 public:
  explicit FunctionVideoEncoderFactory(
      std::function<std::unique_ptr<VideoEncoder>()> create)
      : create_(std::move(create)) {
    codec_info_.is_hardware_accelerated = false;
    codec_info_.has_internal_source = false;
  }

  // Unused by tests.
  std::vector<SdpVideoFormat> GetSupportedFormats() const override {
    RTC_NOTREACHED();
    return {};
  }

  CodecInfo QueryVideoEncoder(
      const SdpVideoFormat& /* format */) const override {
    return codec_info_;
  }

  std::unique_ptr<VideoEncoder> CreateVideoEncoder(
      const SdpVideoFormat& /* format */) override {
    return create_();
  }

 private:
  const std::function<std::unique_ptr<VideoEncoder>()> create_;
  CodecInfo codec_info_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_FUNCTION_VIDEO_ENCODER_FACTORY_H_

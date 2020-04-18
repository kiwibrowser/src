/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_ANALYZER_VIDEO_VIDEO_QUALITY_ANALYZER_INJECTION_HELPER_H_
#define TEST_PC_E2E_ANALYZER_VIDEO_VIDEO_QUALITY_ANALYZER_INJECTION_HELPER_H_

#include <memory>
#include <string>

#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "test/frame_generator.h"
#include "test/pc/e2e/analyzer/video/encoded_image_id_injector.h"
#include "test/pc/e2e/analyzer/video/id_generator.h"
#include "test/pc/e2e/api/video_quality_analyzer_interface.h"
#include "test/testsupport/video_frame_writer.h"

namespace webrtc {
namespace test {

// Provides factory methods for components, that will be used to inject
// VideoQualityAnalyzerInterface into PeerConnection pipeline.
class VideoQualityAnalyzerInjectionHelper {
 public:
  VideoQualityAnalyzerInjectionHelper(
      std::unique_ptr<VideoQualityAnalyzerInterface> analyzer,
      EncodedImageIdInjector* injector,
      EncodedImageIdExtractor* extractor);
  ~VideoQualityAnalyzerInjectionHelper();

  // Wraps video encoder factory to give video quality analyzer access to frames
  // before encoding and encoded images after.
  std::unique_ptr<VideoEncoderFactory> WrapVideoEncoderFactory(
      std::unique_ptr<VideoEncoderFactory> delegate) const;
  // Wraps video decoder factory to give video quality analyzer access to
  // received encoded images and frames, that were decoded from them.
  std::unique_ptr<VideoDecoderFactory> WrapVideoDecoderFactory(
      std::unique_ptr<VideoDecoderFactory> delegate) const;

  // Wraps frame generator, so video quality analyzer will gain access to the
  // captured frames. If |writer| in not nullptr, will dump captured frames
  // with provided writer.
  std::unique_ptr<FrameGenerator> WrapFrameGenerator(
      std::string stream_label,
      std::unique_ptr<FrameGenerator> delegate,
      VideoFrameWriter* writer) const;
  // Creates sink, that will allow video quality analyzer to get access to the
  // rendered frames. If |writer| in not nullptr, will dump rendered frames
  // with provided writer.
  std::unique_ptr<rtc::VideoSinkInterface<VideoFrame>> CreateVideoSink(
      VideoFrameWriter* writer) const;

  // Stops VideoQualityAnalyzerInterface to populate final data and metrics.
  void Stop();

 private:
  std::unique_ptr<VideoQualityAnalyzerInterface> analyzer_;
  EncodedImageIdInjector* injector_;
  EncodedImageIdExtractor* extractor_;

  std::unique_ptr<IdGenerator<int>> encoding_entities_id_generator_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_PC_E2E_ANALYZER_VIDEO_VIDEO_QUALITY_ANALYZER_INJECTION_HELPER_H_

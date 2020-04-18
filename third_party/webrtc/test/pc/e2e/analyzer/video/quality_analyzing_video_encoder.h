/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef TEST_PC_E2E_ANALYZER_VIDEO_QUALITY_ANALYZING_VIDEO_ENCODER_H_
#define TEST_PC_E2E_ANALYZER_VIDEO_QUALITY_ANALYZING_VIDEO_ENCODER_H_

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "api/video/video_frame.h"
#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_codec.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/critical_section.h"
#include "test/pc/e2e/analyzer/video/encoded_image_id_injector.h"
#include "test/pc/e2e/analyzer/video/id_generator.h"
#include "test/pc/e2e/api/video_quality_analyzer_interface.h"

namespace webrtc {
namespace test {

// QualityAnalyzingVideoEncoder is used to wrap origin video encoder and inject
// VideoQualityAnalyzerInterface before and after encoder.
//
// QualityAnalyzingVideoEncoder propagates all calls to the origin encoder.
// It registers its own EncodedImageCallback in the origin encoder and will
// store user specified callback inside itself.
//
// When Encode(...) will be invoked, quality encoder first calls video quality
// analyzer with original frame, then encodes frame with original encoder.
//
// When origin encoder encodes the image it will call quality encoder's special
// callback, where video analyzer will be called again and then frame id will be
// injected into EncodedImage with passed EncodedImageIdInjector. Then new
// EncodedImage will be passed to origin callback, provided by user.
//
// Quality encoder registers its own callback in origin encoder at the same
// time, when user registers his callback in quality encoder.
class QualityAnalyzingVideoEncoder : public VideoEncoder,
                                     public EncodedImageCallback {
 public:
  // Creates analyzing encoder. |id| is unique coding entity id, that will
  // be used to distinguish all encoders and decoders inside
  // EncodedImageIdInjector and EncodedImageIdExtracor.
  QualityAnalyzingVideoEncoder(int id,
                               std::unique_ptr<VideoEncoder> delegate,
                               EncodedImageIdInjector* injector,
                               VideoQualityAnalyzerInterface* analyzer);
  ~QualityAnalyzingVideoEncoder() override;

  // Methods of VideoEncoder interface.
  int32_t InitEncode(const VideoCodec* codec_settings,
                     int32_t number_of_cores,
                     size_t max_payload_size) override;
  int32_t RegisterEncodeCompleteCallback(
      EncodedImageCallback* callback) override;
  int32_t Release() override;
  int32_t Encode(const VideoFrame& frame,
                 const CodecSpecificInfo* codec_specific_info,
                 const std::vector<FrameType>* frame_types) override;
  int32_t SetRates(uint32_t bitrate, uint32_t framerate) override;
  int32_t SetRateAllocation(const VideoBitrateAllocation& allocation,
                            uint32_t framerate) override;
  EncoderInfo GetEncoderInfo() const override;

  // Methods of EncodedImageCallback interface.
  EncodedImageCallback::Result OnEncodedImage(
      const EncodedImage& encoded_image,
      const CodecSpecificInfo* codec_specific_info,
      const RTPFragmentationHeader* fragmentation) override;
  void OnDroppedFrame(DropReason reason) override;

 private:
  const int id_;
  std::unique_ptr<VideoEncoder> delegate_;
  EncodedImageIdInjector* const injector_;
  VideoQualityAnalyzerInterface* const analyzer_;

  // VideoEncoder interface assumes async delivery of encoded images.
  // This lock is used to protect shared state, that have to be propagated
  // from received VideoFrame to resulted EncodedImage.
  rtc::CriticalSection lock_;

  EncodedImageCallback* delegate_callback_ RTC_GUARDED_BY(lock_);
  std::list<std::pair<uint32_t, uint16_t>> timestamp_to_frame_id_list_
      RTC_GUARDED_BY(lock_);
};

// Produces QualityAnalyzingVideoEncoder, which hold decoders, produced by
// specified factory as delegates. Forwards all other calls to specified
// factory.
class QualityAnalyzingVideoEncoderFactory : public VideoEncoderFactory {
 public:
  QualityAnalyzingVideoEncoderFactory(
      std::unique_ptr<VideoEncoderFactory> delegate,
      IdGenerator<int>* id_generator,
      EncodedImageIdInjector* injector,
      VideoQualityAnalyzerInterface* analyzer);
  ~QualityAnalyzingVideoEncoderFactory() override;

  // Methods of VideoEncoderFactory interface.
  std::vector<SdpVideoFormat> GetSupportedFormats() const override;
  VideoEncoderFactory::CodecInfo QueryVideoEncoder(
      const SdpVideoFormat& format) const override;
  std::unique_ptr<VideoEncoder> CreateVideoEncoder(
      const SdpVideoFormat& format) override;

 private:
  std::unique_ptr<VideoEncoderFactory> delegate_;
  IdGenerator<int>* const id_generator_;
  EncodedImageIdInjector* const injector_;
  VideoQualityAnalyzerInterface* const analyzer_;
};

}  // namespace test
}  // namespace webrtc

#endif  // TEST_PC_E2E_ANALYZER_VIDEO_QUALITY_ANALYZING_VIDEO_ENCODER_H_

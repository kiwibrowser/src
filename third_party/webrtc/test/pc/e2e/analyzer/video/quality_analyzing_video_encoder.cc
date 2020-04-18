/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "test/pc/e2e/analyzer/video/quality_analyzing_video_encoder.h"

#include <utility>

#include "absl/memory/memory.h"
#include "modules/video_coding/include/video_error_codes.h"
#include "rtc_base/logging.h"

namespace webrtc {
namespace test {
namespace {

constexpr size_t kMaxFrameInPipelineCount = 1000;

}  // namespace

QualityAnalyzingVideoEncoder::QualityAnalyzingVideoEncoder(
    int id,
    std::unique_ptr<VideoEncoder> delegate,
    EncodedImageIdInjector* injector,
    VideoQualityAnalyzerInterface* analyzer)
    : id_(id),
      delegate_(std::move(delegate)),
      injector_(injector),
      analyzer_(analyzer) {}
QualityAnalyzingVideoEncoder::~QualityAnalyzingVideoEncoder() = default;

int32_t QualityAnalyzingVideoEncoder::InitEncode(
    const VideoCodec* codec_settings,
    int32_t number_of_cores,
    size_t max_payload_size) {
  return delegate_->InitEncode(codec_settings, number_of_cores,
                               max_payload_size);
}

int32_t QualityAnalyzingVideoEncoder::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  // We need to get a lock here because delegate_callback can be hypothetically
  // accessed from different thread (encoder one) concurrently.
  rtc::CritScope crit(&lock_);
  delegate_callback_ = callback;
  return delegate_->RegisterEncodeCompleteCallback(this);
}

int32_t QualityAnalyzingVideoEncoder::Release() {
  rtc::CritScope crit(&lock_);
  delegate_callback_ = nullptr;
  return delegate_->Release();
}

int32_t QualityAnalyzingVideoEncoder::Encode(
    const VideoFrame& frame,
    const CodecSpecificInfo* codec_specific_info,
    const std::vector<FrameType>* frame_types) {
  {
    rtc::CritScope crit(&lock_);
    // Store id to be able to retrieve it in analyzing callback.
    timestamp_to_frame_id_list_.push_back({frame.timestamp(), frame.id()});
    // If this list is growing, it means that we are not receiving new encoded
    // images from encoder. So it should be a bug in setup on in the encoder.
    RTC_DCHECK_LT(timestamp_to_frame_id_list_.size(), kMaxFrameInPipelineCount);
  }
  analyzer_->OnFramePreEncode(frame);
  int32_t result = delegate_->Encode(frame, codec_specific_info, frame_types);
  if (result != WEBRTC_VIDEO_CODEC_OK) {
    // If origin encoder failed, then cleanup data for this frame.
    {
      rtc::CritScope crit(&lock_);
      // The timestamp-frame_id pair can be not the last one, so we need to
      // find it first and then remove. We will search from the end, because
      // usually it will be the last or close to the last one.
      auto it = timestamp_to_frame_id_list_.end();
      while (it != timestamp_to_frame_id_list_.begin()) {
        --it;
        if (it->first == frame.timestamp()) {
          timestamp_to_frame_id_list_.erase(it);
          break;
        }
      }
    }
    analyzer_->OnEncoderError(frame, result);
  }
  return result;
}

int32_t QualityAnalyzingVideoEncoder::SetRates(uint32_t bitrate,
                                               uint32_t framerate) {
  return delegate_->SetRates(bitrate, framerate);
}

int32_t QualityAnalyzingVideoEncoder::SetRateAllocation(
    const VideoBitrateAllocation& allocation,
    uint32_t framerate) {
  return delegate_->SetRateAllocation(allocation, framerate);
}

VideoEncoder::EncoderInfo QualityAnalyzingVideoEncoder::GetEncoderInfo() const {
  return delegate_->GetEncoderInfo();
}

// It is assumed, that encoded callback will be always invoked with encoded
// images that correspond to the frames if the same sequence, that frames
// arrived. In other words, assume we have frames F1, F2 and F3 and they have
// corresponding encoded images I1, I2 and I3. In such case if we will call
// encode first with F1, then with F2 and then with F3, then encoder callback
// will be called first with all spatial layers for F1 (I1), then F2 (I2) and
// then F3 (I3).
//
// Basing on it we will use a list of timestamp-frame_id pairs like this:
//  1. If current encoded image timestamp is equals to timestamp in the front
//     pair - pick frame id from that pair
//  2. If current encoded image timestamp isn't equals to timestamp in the front
//     pair - remove the front pair and got to the step 1.
EncodedImageCallback::Result QualityAnalyzingVideoEncoder::OnEncodedImage(
    const EncodedImage& encoded_image,
    const CodecSpecificInfo* codec_specific_info,
    const RTPFragmentationHeader* fragmentation) {
  uint16_t frame_id;
  {
    rtc::CritScope crit(&lock_);
    std::pair<uint32_t, uint16_t> timestamp_frame_id;
    while (!timestamp_to_frame_id_list_.empty()) {
      timestamp_frame_id = timestamp_to_frame_id_list_.front();
      if (timestamp_frame_id.first == encoded_image.Timestamp()) {
        break;
      }
      timestamp_to_frame_id_list_.pop_front();
    }

    // After the loop the first element should point to current |encoded_image|
    // frame id. We don't remove it from the list, because there may be
    // multiple spatial layers for this frame, so encoder can produce more
    // encoded images with this timestamp. The first element will be removed
    // when the next frame would be encoded and EncodedImageCallback would be
    // called with the next timestamp.

    if (timestamp_to_frame_id_list_.empty()) {
      // Ensure, that we have info about this frame. It can happen that for some
      // reasons encoder response, that he failed to decode, when we were
      // posting frame to it, but then call the callback for this frame.
      RTC_LOG(LS_ERROR) << "QualityAnalyzingVideoEncoder::OnEncodedImage: No "
                           "frame id for encoded_image.Timestamp()="
                        << encoded_image.Timestamp();
      return EncodedImageCallback::Result(
          EncodedImageCallback::Result::Error::OK);
    }
    frame_id = timestamp_frame_id.second;
  }

  analyzer_->OnFrameEncoded(frame_id, encoded_image);

  // Image id injector injects frame id into provided EncodedImage and returns
  // the image with a) modified original buffer (in such case the current owner
  // of the buffer will be responsible for deleting it) or b) a new buffer (in
  // such case injector will be responsible for deleting it).
  const EncodedImage& image = injector_->InjectId(frame_id, encoded_image, id_);
  {
    rtc::CritScope crit(&lock_);
    RTC_DCHECK(delegate_callback_);
    return delegate_callback_->OnEncodedImage(image, codec_specific_info,
                                              fragmentation);
  }
}

void QualityAnalyzingVideoEncoder::OnDroppedFrame(
    EncodedImageCallback::DropReason reason) {
  rtc::CritScope crit(&lock_);
  analyzer_->OnFrameDropped(reason);
  RTC_DCHECK(delegate_callback_);
  delegate_callback_->OnDroppedFrame(reason);
}

QualityAnalyzingVideoEncoderFactory::QualityAnalyzingVideoEncoderFactory(
    std::unique_ptr<VideoEncoderFactory> delegate,
    IdGenerator<int>* id_generator,
    EncodedImageIdInjector* injector,
    VideoQualityAnalyzerInterface* analyzer)
    : delegate_(std::move(delegate)),
      id_generator_(id_generator),
      injector_(injector),
      analyzer_(analyzer) {}
QualityAnalyzingVideoEncoderFactory::~QualityAnalyzingVideoEncoderFactory() =
    default;

std::vector<SdpVideoFormat>
QualityAnalyzingVideoEncoderFactory::GetSupportedFormats() const {
  return delegate_->GetSupportedFormats();
}

VideoEncoderFactory::CodecInfo
QualityAnalyzingVideoEncoderFactory::QueryVideoEncoder(
    const SdpVideoFormat& format) const {
  return delegate_->QueryVideoEncoder(format);
}

std::unique_ptr<VideoEncoder>
QualityAnalyzingVideoEncoderFactory::CreateVideoEncoder(
    const SdpVideoFormat& format) {
  return absl::make_unique<QualityAnalyzingVideoEncoder>(
      id_generator_->GetNextId(), delegate_->CreateVideoEncoder(format),
      injector_, analyzer_);
}

}  // namespace test
}  // namespace webrtc

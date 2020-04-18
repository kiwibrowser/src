// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/webrtc_dummy_video_encoder.h"

#include <algorithm>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/stl_util.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "remoting/protocol/video_channel_state_observer.h"

#if defined(USE_H264_ENCODER)
#include "media/video/h264_parser.h"
#endif

namespace remoting {
namespace protocol {

#if defined(USE_H264_ENCODER)
namespace {

// Populates struct webrtc::RTPFragmentationHeader for H264 codec.
// Each entry specifies the offset and length (excluding start code) of a NALU.
// Returns true if successful.
bool GetRTPFragmentationHeaderH264(webrtc::RTPFragmentationHeader* header,
                                   const uint8_t* data, uint32_t length) {
  std::vector<media::H264NALU> nalu_vector;
  if (!media::H264Parser::ParseNALUs(data, length, &nalu_vector)) {
    // H264Parser::ParseNALUs() has logged the errors already.
    return false;
  }

  // TODO(zijiehe): Find a right place to share the following logic between
  // //content and //remoting.
  header->VerifyAndAllocateFragmentationHeader(nalu_vector.size());
  for (size_t i = 0; i < nalu_vector.size(); ++i) {
    header->fragmentationOffset[i] = nalu_vector[i].data - data;
    header->fragmentationLength[i] = nalu_vector[i].size;
    header->fragmentationPlType[i] = 0;
    header->fragmentationTimeDiff[i] = 0;
  }
  return true;
}

}  // namespace
#endif

WebrtcDummyVideoEncoder::WebrtcDummyVideoEncoder(
    scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
    base::WeakPtr<VideoChannelStateObserver> video_channel_state_observer)
    : main_task_runner_(main_task_runner),
      state_(kUninitialized),
      video_channel_state_observer_(video_channel_state_observer) {
  // Initialize randomly to avoid replay attacks.
  base::RandBytes(&picture_id_, sizeof(picture_id_));
  picture_id_ &= 0x7fff;
}

WebrtcDummyVideoEncoder::~WebrtcDummyVideoEncoder() = default;

int32_t WebrtcDummyVideoEncoder::InitEncode(
    const webrtc::VideoCodec* codec_settings,
    int32_t number_of_cores,
    size_t max_payload_size) {
  DCHECK(codec_settings);
  base::AutoLock lock(lock_);
  int stream_count = codec_settings->numberOfSimulcastStreams;
  // Validate request is to support a single stream.
  if (stream_count > 1) {
    for (int i = 0; i < stream_count; ++i) {
      if (codec_settings->simulcastStream[i].maxBitrate != 0) {
        LOG(ERROR) << "Simulcast unsupported";
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
      }
    }
  }
  state_ = kInitialized;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcDummyVideoEncoder::RegisterEncodeCompleteCallback(
    webrtc::EncodedImageCallback* callback) {
  base::AutoLock lock(lock_);
  encoded_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcDummyVideoEncoder::Release() {
  base::AutoLock lock(lock_);
  encoded_callback_ = nullptr;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcDummyVideoEncoder::Encode(
    const webrtc::VideoFrame& frame,
    const webrtc::CodecSpecificInfo* codec_specific_info,
    const std::vector<webrtc::FrameType>* frame_types) {
  // WebrtcDummyVideoCapturer doesn't generate any video frames, so Encode() can
  // be called only from VCMGenericEncoder::RequestFrame() to request a key
  // frame.
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VideoChannelStateObserver::OnKeyFrameRequested,
                            video_channel_state_observer_));
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcDummyVideoEncoder::SetChannelParameters(uint32_t packet_loss,
                                                      int64_t rtt) {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VideoChannelStateObserver::OnChannelParameters,
                            video_channel_state_observer_, packet_loss,
                            base::TimeDelta::FromMilliseconds(rtt)));
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t WebrtcDummyVideoEncoder::SetRates(uint32_t bitrate,
                                          uint32_t framerate) {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VideoChannelStateObserver::OnTargetBitrateChanged,
                            video_channel_state_observer_, bitrate));
  // framerate is not expected to be valid given we never report captured
  // frames.
  return WEBRTC_VIDEO_CODEC_OK;
}

webrtc::EncodedImageCallback::Result WebrtcDummyVideoEncoder::SendEncodedFrame(
    const WebrtcVideoEncoder::EncodedFrame& frame,
    base::TimeTicks capture_time,
    base::TimeTicks encode_started_time,
    base::TimeTicks encode_finished_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  uint8_t* buffer = const_cast<uint8_t*>(
      reinterpret_cast<const uint8_t*>(base::data(frame.data)));
  size_t buffer_size = frame.data.size();
  base::AutoLock lock(lock_);
  if (state_ == kUninitialized) {
    LOG(ERROR) << "encoder interface uninitialized";
    return webrtc::EncodedImageCallback::Result(
        webrtc::EncodedImageCallback::Result::ERROR_SEND_FAILED);
  }

  webrtc::EncodedImage encoded_image(buffer, buffer_size, buffer_size);
  encoded_image._encodedWidth = frame.size.width();
  encoded_image._encodedHeight = frame.size.height();
  encoded_image._completeFrame = true;
  encoded_image._frameType =
      frame.key_frame ? webrtc::kVideoFrameKey : webrtc::kVideoFrameDelta;
  int64_t capture_time_ms = (capture_time - base::TimeTicks()).InMilliseconds();
  int64_t encode_started_time_ms =
      (encode_started_time - base::TimeTicks()).InMilliseconds();
  int64_t encode_finished_time_ms =
      (encode_finished_time - base::TimeTicks()).InMilliseconds();
  encoded_image.capture_time_ms_ = capture_time_ms;
  encoded_image._timeStamp = static_cast<uint32_t>(capture_time_ms * 90);
  encoded_image.playout_delay_.min_ms = 0;
  encoded_image.playout_delay_.max_ms = 0;
  encoded_image.timing_.encode_start_ms = encode_started_time_ms;
  encoded_image.timing_.encode_finish_ms = encode_finished_time_ms;
  encoded_image.content_type_ = webrtc::VideoContentType::SCREENSHARE;

  webrtc::CodecSpecificInfo codec_specific_info;
  memset(&codec_specific_info, 0, sizeof(codec_specific_info));
  codec_specific_info.codecType = frame.codec;

  if (frame.codec == webrtc::kVideoCodecVP8) {
    webrtc::CodecSpecificInfoVP8* vp8_info =
        &codec_specific_info.codecSpecific.VP8;
    vp8_info->simulcastIdx = 0;
    vp8_info->temporalIdx = webrtc::kNoTemporalIdx;
    vp8_info->tl0PicIdx = webrtc::kNoTl0PicIdx;
    vp8_info->pictureId = picture_id_;
    picture_id_ = (picture_id_ + 1) & 0x7fff;
  } else if (frame.codec == webrtc::kVideoCodecVP9) {
    webrtc::CodecSpecificInfoVP9* vp9_info =
        &codec_specific_info.codecSpecific.VP9;
    vp9_info->inter_pic_predicted = !frame.key_frame;
    vp9_info->ss_data_available = frame.key_frame;
    vp9_info->spatial_layer_resolution_present = frame.key_frame;
    if (frame.key_frame) {
      vp9_info->width[0] = frame.size.width();
      vp9_info->height[0] = frame.size.height();
    }
    vp9_info->num_spatial_layers = 1;
    vp9_info->gof_idx = webrtc::kNoGofIdx;
    vp9_info->temporal_idx = webrtc::kNoTemporalIdx;
    vp9_info->spatial_idx = webrtc::kNoSpatialIdx;
    vp9_info->tl0_pic_idx = webrtc::kNoTl0PicIdx;
    vp9_info->picture_id = picture_id_;
    picture_id_ = (picture_id_ + 1) & 0x7fff;
  } else if (frame.codec == webrtc::kVideoCodecH264) {
#if defined(USE_H264_ENCODER)
    webrtc::CodecSpecificInfoH264* h264_info =
        &codec_specific_info.codecSpecific.H264;
    h264_info->packetization_mode =
        webrtc::H264PacketizationMode::NonInterleaved;
#else
    NOTREACHED();
#endif
  } else {
    NOTREACHED();
  }

  webrtc::RTPFragmentationHeader header;
  if (frame.codec == webrtc::kVideoCodecH264) {
#if defined(USE_H264_ENCODER)
    if (!GetRTPFragmentationHeaderH264(&header, buffer, buffer_size)) {
      return webrtc::EncodedImageCallback::Result(
          webrtc::EncodedImageCallback::Result::ERROR_SEND_FAILED);
    }
#else
    NOTREACHED();
#endif
  } else {
    header.VerifyAndAllocateFragmentationHeader(1);
    header.fragmentationOffset[0] = 0;
    header.fragmentationLength[0] = buffer_size;
    header.fragmentationPlType[0] = 0;
    header.fragmentationTimeDiff[0] = 0;
  }

  DCHECK(encoded_callback_);
  return encoded_callback_->OnEncodedImage(encoded_image, &codec_specific_info,
                                           &header);
}

WebrtcDummyVideoEncoderFactory::WebrtcDummyVideoEncoderFactory()
    : main_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
  // TODO(isheriff): These do not really affect anything internally
  // in webrtc.
  codecs_.push_back(cricket::VideoCodec("VP8"));
  codecs_.push_back(cricket::VideoCodec("VP9"));
  codecs_.push_back(cricket::VideoCodec("H264"));
}

WebrtcDummyVideoEncoderFactory::~WebrtcDummyVideoEncoderFactory() {
  DCHECK(encoders_.empty());
}

webrtc::VideoEncoder* WebrtcDummyVideoEncoderFactory::CreateVideoEncoder(
    const cricket::VideoCodec& codec) {
  webrtc::VideoCodecType type = webrtc::PayloadStringToCodecType(codec.name);
  WebrtcDummyVideoEncoder* encoder = new WebrtcDummyVideoEncoder(
      main_task_runner_, video_channel_state_observer_);
  base::AutoLock lock(lock_);
  encoders_.push_back(base::WrapUnique(encoder));
  if (encoder_created_callback_) {
    main_task_runner_->PostTask(FROM_HERE,
                                base::Bind(encoder_created_callback_, type));
  }
  return encoder;
}

const std::vector<cricket::VideoCodec>&
WebrtcDummyVideoEncoderFactory::supported_codecs() const {
  return codecs_;
}

bool WebrtcDummyVideoEncoderFactory::EncoderTypeHasInternalSource(
    webrtc::VideoCodecType type) const {
  // Returns true to directly provide encoded frames to webrtc.
  return true;
}

void WebrtcDummyVideoEncoderFactory::DestroyVideoEncoder(
    webrtc::VideoEncoder* encoder) {
  base::AutoLock lock(lock_);
  if (encoder == nullptr) {
    LOG(ERROR) << "Attempting to destroy null encoder";
    return;
  }
  for (auto pos = encoders_.begin(); pos != encoders_.end(); ++pos) {
    if ((*pos).get() == encoder) {
      encoders_.erase(pos);
      return;
    }
  }
  NOTREACHED() << "Asked to remove encoder not owned by factory.";
}

webrtc::EncodedImageCallback::Result
WebrtcDummyVideoEncoderFactory::SendEncodedFrame(
    const WebrtcVideoEncoder::EncodedFrame& frame,
    base::TimeTicks capture_time,
    base::TimeTicks encode_started_time,
    base::TimeTicks encode_finished_time) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  base::AutoLock lock(lock_);
  if (encoders_.size() != 1) {
    LOG(ERROR) << "Unexpected number of encoders " << encoders_.size();
    return webrtc::EncodedImageCallback::Result(
        webrtc::EncodedImageCallback::Result::ERROR_SEND_FAILED);
  }
  return encoders_.front()->SendEncodedFrame(
      frame, capture_time, encode_started_time, encode_finished_time);
}

void WebrtcDummyVideoEncoderFactory::RegisterEncoderSelectedCallback(
    const base::Callback<void(webrtc::VideoCodecType)>& callback) {
  encoder_created_callback_ = callback;
}

void WebrtcDummyVideoEncoderFactory::SetVideoChannelStateObserver(
    base::WeakPtr<VideoChannelStateObserver> video_channel_state_observer) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(encoders_.empty());
  base::AutoLock lock(lock_);
  video_channel_state_observer_ = video_channel_state_observer;
}

}  // namespace protocol
}  // namespace remoting

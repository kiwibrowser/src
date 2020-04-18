// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_FFMPEG_CDM_VIDEO_DECODER_H_
#define MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_FFMPEG_CDM_VIDEO_DECODER_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "media/cdm/library_cdm/clear_key_cdm/cdm_video_decoder.h"
#include "media/ffmpeg/ffmpeg_deleters.h"

struct AVCodecContext;
struct AVFrame;

namespace media {

class CdmHostProxy;
class FFmpegDecodingLoop;

class FFmpegCdmVideoDecoder : public CdmVideoDecoder {
 public:
  explicit FFmpegCdmVideoDecoder(CdmHostProxy* cdm_host_proxy);
  ~FFmpegCdmVideoDecoder() override;

  // CdmVideoDecoder implementation.
  bool Initialize(const cdm::VideoDecoderConfig_2& config) override;
  void Deinitialize() override;
  void Reset() override;
  cdm::Status DecodeFrame(const uint8_t* compressed_frame,
                          int32_t compressed_frame_size,
                          int64_t timestamp,
                          cdm::VideoFrame* decoded_frame) override;
  bool is_initialized() const override;

  // Returns true when |format| and |data_size| specify a supported video
  // output configuration.
  static bool IsValidOutputConfig(cdm::VideoFormat format,
                                  const cdm::Size& data_size);

 private:
  bool OnNewFrame(AVFrame* frame);

  // Allocates storage, then copies video frame stored in |frame| to
  // |cdm_video_frame|. Returns true when allocation and copy succeed.
  bool CopyAvFrameTo(AVFrame* frame, cdm::VideoFrame* cdm_video_frame);

  void ReleaseFFmpegResources();

  // FFmpeg structures owned by this object.
  std::unique_ptr<AVCodecContext, ScopedPtrAVFreeContext> codec_context_;
  std::unique_ptr<FFmpegDecodingLoop> decoding_loop_;

  bool is_initialized_ = false;

  CdmHostProxy* const cdm_host_proxy_ = nullptr;

  base::circular_deque<std::unique_ptr<AVFrame, ScopedPtrAVFreeFrame>>
      pending_frames_;

  DISALLOW_COPY_AND_ASSIGN(FFmpegCdmVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_CDM_LIBRARY_CDM_CLEAR_KEY_CDM_FFMPEG_CDM_VIDEO_DECODER_H_

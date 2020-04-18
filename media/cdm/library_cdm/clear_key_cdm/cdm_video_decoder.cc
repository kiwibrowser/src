// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/library_cdm/clear_key_cdm/cdm_video_decoder.h"

#if defined(CLEAR_KEY_CDM_USE_FFMPEG_DECODER)
#include "media/cdm/library_cdm/clear_key_cdm/ffmpeg_cdm_video_decoder.h"
#endif

#if defined(CLEAR_KEY_CDM_USE_LIBVPX_DECODER)
#include "media/cdm/library_cdm/clear_key_cdm/libvpx_cdm_video_decoder.h"
#endif

namespace media {

std::unique_ptr<CdmVideoDecoder> CreateVideoDecoder(
    CdmHostProxy* cdm_host_proxy,
    const cdm::VideoDecoderConfig_2& config) {
  std::unique_ptr<CdmVideoDecoder> video_decoder;

#if defined(CLEAR_KEY_CDM_USE_LIBVPX_DECODER)
  if (config.codec == cdm::kCodecVp8 || config.codec == cdm::kCodecVp9) {
    video_decoder.reset(new LibvpxCdmVideoDecoder(cdm_host_proxy));

    if (!video_decoder->Initialize(config))
      video_decoder.reset();

    return video_decoder;
  }
#endif

#if defined(CLEAR_KEY_CDM_USE_FFMPEG_DECODER)
  video_decoder.reset(new FFmpegCdmVideoDecoder(cdm_host_proxy));

  if (!video_decoder->Initialize(config))
    video_decoder.reset();
#endif

  return video_decoder;
}

}  // namespace media

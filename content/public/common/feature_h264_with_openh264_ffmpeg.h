// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_FEATURE_H264_WITH_OPENH264_FFMPEG_H_
#define CONTENT_PUBLIC_COMMON_FEATURE_H264_WITH_OPENH264_FFMPEG_H_

#include "base/feature_list.h"
#include "content/public/common/buildflags.h"
#include "media/media_buildflags.h"

namespace content {

#if BUILDFLAG(RTC_USE_H264) && BUILDFLAG(ENABLE_FFMPEG_VIDEO_DECODERS)

// Run-time feature for the |rtc_use_h264| encoder/decoder.
extern const base::Feature kWebRtcH264WithOpenH264FFmpeg;

#endif  // BUILDFLAG(RTC_USE_H264) && BUILDFLAG(ENABLE_FFMPEG_VIDEO_DECODERS)

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_FEATURE_H264_WITH_OPENH264_FFMPEG_H_
